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
#include "mrubyc.h"

#include "led0_blink_bytecode.c"

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
  _PROTECTED_WRITE(CLKCTRL.XOSCHFCTRLA,
                   CLKCTRL_ENABLE_bm |
                   CLKCTRL_SELHF_XTAL_gc |
                   CLKCTRL_FRQRANGE_16M_gc |
                   CLKCTRL_CSUTHF_4K_gc);

  while( (CLKCTRL.MCLKSTATUS & CLKCTRL_EXTS_bm) == 0 ) {
  }

  _PROTECTED_WRITE(CLKCTRL.MCLKCTRLA, CLKCTRL_CLKSEL_EXTCLK_gc);
}

/**
 * @brief Initialize the board LED0 GPIO.
 * @details
 * LED0 is controlled through PORTB bit 3 and is treated as active-low by this
 * application. The output latch is set before enabling the output driver so
 * that the LED starts in the off state without a visible low-going glitch.
 */
static void led0_init(void)
{
  PORTB.OUTSET = PIN3_bm;
  PORTB.DIRSET = PIN3_bm;
}

/**
 * @brief Turn LED0 on.
 * @details
 * LED0 is active-low, so clearing the PORTB bit drives the LED control signal
 * low and turns the LED on.
 */
static void led0_on(void)
{
  PORTB.OUTCLR = PIN3_bm;
}

/**
 * @brief Turn LED0 off.
 * @details
 * LED0 is active-low, so setting the PORTB bit drives the LED control signal
 * high and turns the LED off.
 */
static void led0_off(void)
{
  PORTB.OUTSET = PIN3_bm;
}

/**
 * @brief Toggle the current LED0 output state.
 * @details
 * The AVR PORT output-toggle register flips PORTB bit 3 atomically, avoiding a
 * read-modify-write sequence in application code.
 */
static void led0_toggle(void)
{
  PORTB.OUTTGL = PIN3_bm;
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
  return (PORTB.OUT & PIN3_bm) == 0;
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
  define_led0_class();

  if( mrbc_create_task(mrbbuf, 0) != NULL ) {
    mrbc_run();
  }

  led0_off();
  while( 1 ) {
  }
}
