/* SPDX-License-Identifier: BSD-3-Clause */

#ifndef LED_H
#define LED_H

#include <mrubyc.h>

/* Registers the Led class. Call after mrbc_init() and before mrbc_create_task(). */
void register_led_class(mrbc_vm *vm);

#endif
