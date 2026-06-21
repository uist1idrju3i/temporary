/**
 * @file main.c
 * @brief mruby/c LED0 blink application for the EV35L43A / AVR target.
 * @details
 * This file initializes the MCU clock, configures the active-low LED0 GPIO,
 * exposes LED0 control methods to mruby/c, and runs the bytecode generated
 * from the Ruby application.
 */
#include <stdint.h>
#include <avr/io.h>
#include <avr/interrupt.h>
#include "mrubyc.h"

#include "led0_blink_bytecode.c"

#if !defined(__AVR_AVR128DB48__) && !defined(__AVR_ATmega128__)
#error "Unsupported AVR device. This sample supports AVR128DB48 and ATmega128."
#endif

/**
 * @def MRBC_MEMORY_SIZE
 * @brief Size of the static memory pool passed to the mruby/c runtime.
 * @details
 * The build system may override this value. The fallback value is kept small
 * enough for the AVR target while still allowing the bundled blink script to
 * run.
 */
#if !defined(MRBC_MEMORY_SIZE)
#define MRBC_MEMORY_SIZE (1024 * 8)
#endif

/**
 * @brief Static heap storage used internally by the mruby/c virtual machine.
 * @details
 * mruby/c does not allocate from the C library heap in this application.
 * Instead, the runtime receives this fixed-size buffer during initialization.
 */
static uint8_t memory_pool[MRBC_MEMORY_SIZE];

#if defined(__AVR_AVR128DB48__) && !defined(MRBC_NO_TIMER)
#define AVR128DB_MRBC_TIMER_PRESCALER 64u
#define AVR128DB_MRBC_TIMER_COUNTS ((F_CPU / AVR128DB_MRBC_TIMER_PRESCALER) * MRBC_TICK_UNIT / 1000u)

#if AVR128DB_MRBC_TIMER_COUNTS == 0 || AVR128DB_MRBC_TIMER_COUNTS > 65536u
#error "MRBC_TICK_UNIT cannot be generated with the AVR128DB TCA0 timer settings."
#endif
#endif

#if defined(__AVR_AVR128DB48__)
#define CLOCK_STARTUP_TIMEOUT 65535u

static uint8_t wait_clock_status(uint8_t mask)
{
  uint16_t timeout = CLOCK_STARTUP_TIMEOUT;

  while( (CLKCTRL.MCLKSTATUS & mask) == 0 ) {
    if( timeout == 0 ) return 0;
    timeout--;
  }

  return 1;
}

/**
 * @brief Configure the main clock source for the application.
 * @details
 * The external high-frequency crystal oscillator is enabled, the code waits
 * until the oscillator reports a stable status, and then the protected main
 * clock selector is switched to the external clock source.
 *
 * @note This function must run before timing-sensitive peripheral or runtime
 *       initialization that depends on the final CPU clock frequency.
 */
static void clock_init(void)
{
  _PROTECTED_WRITE(CLKCTRL.MCLKCTRLB, 0);
  _PROTECTED_WRITE(CLKCTRL.OSCHFCTRLA, CLKCTRL_FRQSEL_16M_gc);
  (void)wait_clock_status(CLKCTRL_OSCHFS_bm);

  _PROTECTED_WRITE(CLKCTRL.XOSCHFCTRLA,
                   CLKCTRL_ENABLE_bm |
                   CLKCTRL_SELHF_XTAL_gc |
                   CLKCTRL_FRQRANGE_16M_gc |
                   CLKCTRL_CSUTHF_4K_gc);

  if( wait_clock_status(CLKCTRL_EXTS_bm) ) {
    _PROTECTED_WRITE(CLKCTRL.MCLKCTRLA, CLKCTRL_CLKSEL_EXTCLK_gc);
  }
}
#elif defined(__AVR_ATmega128__)
static void clock_init(void)
{
}
#endif

#if defined(__AVR_AVR128DB48__) && !defined(MRBC_NO_TIMER)
static void timer_init(void)
{
  TCA0.SINGLE.CTRLA = 0;
  TCA0.SINGLE.CTRLB = TCA_SINGLE_WGMODE_NORMAL_gc;
  TCA0.SINGLE.CNT = 0;
  TCA0.SINGLE.PER = (uint16_t)(AVR128DB_MRBC_TIMER_COUNTS - 1u);
  TCA0.SINGLE.INTFLAGS = TCA_SINGLE_OVF_bm;
  TCA0.SINGLE.INTCTRL = TCA_SINGLE_OVF_bm;
  TCA0.SINGLE.CTRLA = TCA_SINGLE_CLKSEL_DIV64_gc | TCA_SINGLE_ENABLE_bm;
}

ISR(TCA0_OVF_vect)
{
  TCA0.SINGLE.INTFLAGS = TCA_SINGLE_OVF_bm;
  mrbc_tick();
}
#else
static void timer_init(void)
{
}
#endif

/**
 * @brief Initialize the board LED0 GPIO.
 * @details
 * LED0 is controlled through PORTB bit 3 and is treated as active-low by this
 * application. The output latch is set before enabling the output driver so
 * that the LED starts in the off state without a visible low-going glitch.
 */
static void led0_init(void)
{
#if defined(__AVR_AVR128DB48__)
  PORTB.OUTSET = PIN3_bm;
  PORTB.DIRSET = PIN3_bm;
#elif defined(__AVR_ATmega128__)
  PORTB |= _BV(PB3);
  DDRB |= _BV(PB3);
#endif
}

/**
 * @brief Turn LED0 on.
 * @details
 * LED0 is active-low, so clearing the PORTB bit drives the LED control signal
 * low and turns the LED on.
 */
static void led0_on(void)
{
#if defined(__AVR_AVR128DB48__)
  PORTB.OUTCLR = PIN3_bm;
#elif defined(__AVR_ATmega128__)
  PORTB &= (uint8_t)~_BV(PB3);
#endif
}

/**
 * @brief Turn LED0 off.
 * @details
 * LED0 is active-low, so setting the PORTB bit drives the LED control signal
 * high and turns the LED off.
 */
static void led0_off(void)
{
#if defined(__AVR_AVR128DB48__)
  PORTB.OUTSET = PIN3_bm;
#elif defined(__AVR_ATmega128__)
  PORTB |= _BV(PB3);
#endif
}

/**
 * @brief Toggle the current LED0 output state.
 * @details
 * The AVR PORT output-toggle register flips PORTB bit 3 atomically, avoiding a
 * read-modify-write sequence in application code.
 */
static void led0_toggle(void)
{
#if defined(__AVR_AVR128DB48__)
  PORTB.OUTTGL = PIN3_bm;
#elif defined(__AVR_ATmega128__)
  PORTB ^= _BV(PB3);
#endif
}

/**
 * @brief Query whether LED0 is currently on.
 * @details
 * Because the LED is active-low, a cleared PORTB bit represents the on state.
 *
 * @retval 1 LED0 is currently on.
 * @retval 0 LED0 is currently off.
 */
static int led0_is_on(void)
{
#if defined(__AVR_AVR128DB48__)
  return (PORTB.OUT & PIN3_bm) == 0;
#elif defined(__AVR_ATmega128__)
  return (PORTB & _BV(PB3)) == 0;
#endif
}

/**
 * @brief mruby/c native method backing `LED0.on`.
 * @details
 * This method turns LED0 on and returns `nil` to Ruby.
 *
 * @param vm mruby/c VM invoking this native method.
 * @param v mruby/c argument and return-value array.
 * @param argc Number of Ruby arguments supplied by the caller.
 */
static void c_led0_on(mrbc_vm *vm, mrbc_value v[], int argc)
{
  (void)vm;
  (void)argc;
  led0_on();
  SET_NIL_RETURN();
}

/**
 * @brief mruby/c native method backing `LED0.off`.
 * @details
 * This method turns LED0 off and returns `nil` to Ruby.
 *
 * @param vm mruby/c VM invoking this native method.
 * @param v mruby/c argument and return-value array.
 * @param argc Number of Ruby arguments supplied by the caller.
 */
static void c_led0_off(mrbc_vm *vm, mrbc_value v[], int argc)
{
  (void)vm;
  (void)argc;
  led0_off();
  SET_NIL_RETURN();
}

/**
 * @brief mruby/c native method backing `LED0.toggle`.
 * @details
 * This method toggles LED0 and returns `nil` to Ruby.
 *
 * @param vm mruby/c VM invoking this native method.
 * @param v mruby/c argument and return-value array.
 * @param argc Number of Ruby arguments supplied by the caller.
 */
static void c_led0_toggle(mrbc_vm *vm, mrbc_value v[], int argc)
{
  (void)vm;
  (void)argc;
  led0_toggle();
  SET_NIL_RETURN();
}

/**
 * @brief mruby/c native method backing `LED0.on?`.
 * @details
 * This method returns a Ruby boolean indicating whether LED0 is currently on.
 *
 * @param vm mruby/c VM invoking this native method.
 * @param v mruby/c argument and return-value array.
 * @param argc Number of Ruby arguments supplied by the caller.
 */
static void c_led0_on_q(mrbc_vm *vm, mrbc_value v[], int argc)
{
  (void)vm;
  (void)argc;
  SET_BOOL_RETURN(led0_is_on());
}

/**
 * @brief Define the Ruby-visible `LED0` class and its native methods.
 * @details
 * The class methods registered here provide a minimal hardware abstraction for
 * the Ruby bytecode. Ruby code can call `LED0.on`, `LED0.off`, `LED0.toggle`,
 * and `LED0.on?` without directly accessing MCU registers.
 */
static void define_led0_class(void)
{
  mrbc_class *led0_cls = mrbc_define_class(0, "LED0", MRBC_CLASS(Object));
  mrbc_define_method(0, led0_cls, "on", c_led0_on);
  mrbc_define_method(0, led0_cls, "off", c_led0_off);
  mrbc_define_method(0, led0_cls, "toggle", c_led0_toggle);
  mrbc_define_method(0, led0_cls, "on?", c_led0_on_q);
}

/**
 * @brief Application entry point.
 * @details
 * The startup sequence configures the system clock and LED hardware, initializes
 * mruby/c with the static memory pool, registers the LED0 bridge class, creates
 * a task from the embedded Ruby bytecode, and starts the mruby/c scheduler.
 * If task creation fails or the scheduler returns, LED0 is turned off and the
 * firmware remains in an idle infinite loop.
 *
 * @return This function does not return during normal operation.
 */
int main(void)
{
  clock_init();
  led0_init();
  mrbc_init(memory_pool, MRBC_MEMORY_SIZE);
  timer_init();
  define_led0_class();

  if( mrbc_create_task(mrbbuf, 0) != NULL ) {
    mrbc_run();
  }

  led0_off();
  while( 1 ) {
  }
}
