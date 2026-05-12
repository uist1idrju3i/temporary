/* SPDX-License-Identifier: BSD-3-Clause */

#include <zephyr/kernel.h>
#include "mrubyc.h"
#include "test.h"

#define MRBC_MEMORY_SIZE (1024 * 40)
static uint8_t memory_pool[MRBC_MEMORY_SIZE];

int main(void)
{
    mrbc_init(memory_pool, MRBC_MEMORY_SIZE);
    mrbc_hal_init();

    if (mrbc_create_task(test_mrb, NULL) != NULL) {
        mrbc_run();
    }

    mrbc_cleanup();
    return 0;
}
