/* SPDX-License-Identifier: BSD-3-Clause */

#include <stdint.h>
#include <mrubyc.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/sys/util.h>

#include "led.h"

/* led0..led3 exist on the nRF54L15-DK and in boards/qemu_cortex_m3.overlay. */
static const struct gpio_dt_spec leds[] = {
	GPIO_DT_SPEC_GET(DT_ALIAS(led0), gpios),
	GPIO_DT_SPEC_GET(DT_ALIAS(led1), gpios),
	GPIO_DT_SPEC_GET(DT_ALIAS(led2), gpios),
	GPIO_DT_SPEC_GET(DT_ALIAS(led3), gpios),
};

static int led_index(mrbc_value *v)
{
	mrbc_value pin = mrbc_instance_getiv(v, mrbc_str_to_symid("pin"));
	int index = mrbc_integer(pin);

	mrbc_decref(&pin);
	return index;
}

static void led_set_state(mrbc_value *v, int on)
{
	mrbc_value state = mrbc_bool_value(on);

	mrbc_instance_setiv(v, mrbc_str_to_symid("on"), &state);
}

static void c_led_initialize(mrbc_vm *vm, mrbc_value *v, int argc)
{
	if (argc != 1) {
		mrbc_raise(vm, MRBC_CLASS(ArgumentError), "wrong number of arguments");
		return;
	}
	if (mrbc_type(v[1]) != MRBC_TT_INTEGER) {
		mrbc_raise(vm, MRBC_CLASS(TypeError), "index must be an Integer");
		return;
	}

	int index = mrbc_integer(v[1]);

	if (index < 0 || index >= (int)ARRAY_SIZE(leds)) {
		mrbc_raise(vm, MRBC_CLASS(RangeError), "index out of range");
		return;
	}
	if (!gpio_is_ready_dt(&leds[index]) ||
	    gpio_pin_configure_dt(&leds[index], GPIO_OUTPUT_INACTIVE) < 0) {
		mrbc_raise(vm, MRBC_CLASS(IOError), "GPIO is not ready");
		return;
	}

	mrbc_value pin = mrbc_integer_value(index);

	mrbc_instance_setiv(&v[0], mrbc_str_to_symid("pin"), &pin);
	led_set_state(&v[0], 0);
}

static void c_led_on(mrbc_vm *vm, mrbc_value *v, int argc)
{
	(void)vm;
	(void)argc;
	gpio_pin_set_dt(&leds[led_index(&v[0])], 1);
	led_set_state(&v[0], 1);
	SET_NIL_RETURN();
}

static void c_led_off(mrbc_vm *vm, mrbc_value *v, int argc)
{
	(void)vm;
	(void)argc;
	gpio_pin_set_dt(&leds[led_index(&v[0])], 0);
	led_set_state(&v[0], 0);
	SET_NIL_RETURN();
}

static void c_led_on_q(mrbc_vm *vm, mrbc_value *v, int argc)
{
	(void)vm;
	(void)argc;
	mrbc_value state = mrbc_instance_getiv(&v[0], mrbc_str_to_symid("on"));

	SET_RETURN(state);
}

void register_led_class(mrbc_vm *vm)
{
	mrbc_class *cls = mrbc_define_class(vm, "Led", MRBC_CLASS(Object));

	mrbc_define_method(vm, cls, "initialize", c_led_initialize);
	mrbc_define_method(vm, cls, "on", c_led_on);
	mrbc_define_method(vm, cls, "off", c_led_off);
	mrbc_define_method(vm, cls, "on?", c_led_on_q);
}
