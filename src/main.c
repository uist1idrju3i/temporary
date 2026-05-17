#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <emscripten.h>
#include "mrubyc.h"

#if !defined(MRBC_MEMORY_SIZE)
#define MRBC_MEMORY_SIZE 131072
#endif

#define MIN_BYTECODE_SIZE 8
#define MAX_BYTECODE_SIZE (1024 * 1024)

static uint8_t memory_pool[MRBC_MEMORY_SIZE];
static int initialized = 0;

static void write_text(int fd, const char *message)
{
  hal_write(fd, message, (int)strlen(message));
  hal_flush(fd);
}

EMSCRIPTEN_KEEPALIVE
void mrbc_wasm_init(void)
{
  if (!initialized) {
    mrbc_init(memory_pool, MRBC_MEMORY_SIZE);
    initialized = 1;
  }
}

static void reset_vm(void)
{
  if (initialized) {
    mrbc_cleanup();
    initialized = 0;
  }
  mrbc_wasm_init();
}

EMSCRIPTEN_KEEPALIVE
int mrbc_wasm_run(const uint8_t *bytecode, int size)
{
  char buffer[128];

  if (bytecode == NULL) {
    write_text(2, "Bytecode pointer is NULL.\n");
    return -1;
  }

  if (size < MIN_BYTECODE_SIZE) {
    snprintf(buffer, sizeof(buffer), "Bytecode is too small: %d bytes.\n", size);
    write_text(2, buffer);
    return -2;
  }

  if (size > MAX_BYTECODE_SIZE) {
    snprintf(buffer, sizeof(buffer), "Bytecode is too large: %d bytes.\n", size);
    write_text(2, buffer);
    return -3;
  }

  if (bytecode[0] != 'R' || bytecode[1] != 'I' || bytecode[2] != 'T' || bytecode[3] != 'E') {
    write_text(2, "Invalid mruby bytecode.\n");
    return -4;
  }

  if (!initialized) {
    mrbc_wasm_init();
  }

  if (mrbc_create_task(bytecode, NULL) == NULL) {
    write_text(2, "Failed to create mruby/c task.\n");
    reset_vm();
    return -5;
  }

  int ret = mrbc_run();
  hal_flush(1);
  hal_flush(2);
  reset_vm();

  return ret == 1 ? 0 : ret;
}
