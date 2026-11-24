/* SPDX-License-Identifier: BSD-3-Clause */

#include <stdint.h>
#include <mrubyc.h>
#include <zephyr/sys/printk.h>

#include "led.h"
#include "blink_mrb.h"
#include "heartbeat_mrb.h"

static _Alignas(8) uint8_t pool[32 * 1024];

int main(void)
{
	mrbc_init(pool, sizeof(pool));
	register_led_class(NULL);

	if (mrbc_create_task(blink_mrb, NULL) == NULL ||
	    mrbc_create_task(heartbeat_mrb, NULL) == NULL) {
		printk("Unable to load the Ruby tasks\n");
		return 1;
	}

	int result = mrbc_run();

	printk("Scheduler returned %d\n", result);
	return result == 0 ? 0 : 1;
}
