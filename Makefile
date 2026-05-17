SHELL := /bin/bash

MRUBY_DIR := mruby
MRUBYC_DIR := mrubyc
MRUBYC_SRC_DIR := $(MRUBYC_DIR)/src
MRUBYC_HAL_DIR := $(MRUBYC_DIR)/hal/emscripten
MRUBY_VERSION_H := $(MRUBY_DIR)/include/mruby/version.h
EMSDK_DIR := emsdk
EMSDK_VERSION ?= 5.0.7
HOMEBREW_RUBY_BINDIR ?= /opt/homebrew/opt/ruby/bin
SRC_DIR := src
PUBLIC_DIR := public_html
BUILD_DIR := build

CC := emcc
RUBY ?= $(HOMEBREW_RUBY_BINDIR)/ruby
PYTHON ?= python3
PORT ?= 8000

MRUBYC_CFLAGS := -O3 \
	-flto \
	-Wall \
	-I$(MRUBYC_SRC_DIR) \
	-I$(MRUBYC_HAL_DIR) \
	-DMRBC_SCHEDULER_EXIT=1 \
	-DMRBC_USE_FLOAT=2 \
	-DMRBC_USE_MATH=1 \
	-DMAX_VM_COUNT=5 \
	-DMRBC_MEMORY_SIZE=131072

MRUBYC_EMFLAGS := -s WASM=1 \
	-s STRICT=1 \
	-s EXPORTED_RUNTIME_METHODS='["ccall","HEAPU8"]' \
	-s EXPORTED_FUNCTIONS='["_mrbc_wasm_init","_mrbc_wasm_run","_malloc","_free"]' \
	-s INITIAL_HEAP=16777216 \
	-s STACK_SIZE=1048576 \
	-s MALLOC=dlmalloc \
	-s ABORTING_MALLOC=1 \
	-s ASYNCIFY=1 \
	-s ASYNCIFY_STACK_SIZE=65536 \
	-s MODULARIZE=1 \
	-s EXPORT_ES6=1 \
	-s EXPORT_NAME='createMrubycModule' \
	-s ASSERTIONS=1 \
	-s STACK_OVERFLOW_CHECK=1 \
	-s CHECK_NULL_WRITES=1 \
	-s FILESYSTEM=0 \
	-s ENVIRONMENT='web' \
	-s EXIT_RUNTIME=0 \
	-s DYNAMIC_EXECUTION=0 \
	-s TEXTDECODER=2 \
	-s INCOMING_MODULE_JS_API='["locateFile","print","printErr"]' \
	--no-entry

MRUBYC_SRCS := \
	$(MRUBYC_SRC_DIR)/alloc.c \
	$(MRUBYC_SRC_DIR)/c_array.c \
	$(MRUBYC_SRC_DIR)/c_hash.c \
	$(MRUBYC_SRC_DIR)/c_math.c \
	$(MRUBYC_SRC_DIR)/c_numeric.c \
	$(MRUBYC_SRC_DIR)/c_object.c \
	$(MRUBYC_SRC_DIR)/c_proc.c \
	$(MRUBYC_SRC_DIR)/c_range.c \
	$(MRUBYC_SRC_DIR)/c_string.c \
	$(MRUBYC_SRC_DIR)/class.c \
	$(MRUBYC_SRC_DIR)/console.c \
	$(MRUBYC_SRC_DIR)/error.c \
	$(MRUBYC_SRC_DIR)/global.c \
	$(MRUBYC_SRC_DIR)/keyvalue.c \
	$(MRUBYC_SRC_DIR)/load.c \
	$(MRUBYC_SRC_DIR)/mrblib.c \
	$(MRUBYC_SRC_DIR)/rrt0.c \
	$(MRUBYC_SRC_DIR)/symbol.c \
	$(MRUBYC_SRC_DIR)/value.c \
	$(MRUBYC_SRC_DIR)/vm.c \
	$(MRUBYC_HAL_DIR)/hal.c \
	$(SRC_DIR)/main.c

MRUBYC_OUTPUT_DIR := $(PUBLIC_DIR)/mrubyc
MRUBYC_OUTPUT_JS := $(MRUBYC_OUTPUT_DIR)/mrubyc.js
MRUBYC_OUTPUT_WASM := $(MRUBYC_OUTPUT_DIR)/mrubyc.wasm
MRBC_OUTPUT_JS := $(PUBLIC_DIR)/mrbc/mrbc.js
MRBC_OUTPUT_WASM := $(PUBLIC_DIR)/mrbc/mrbc.wasm

.PHONY: all emsdk-install mrbc mrubyc mrubyc-autogen check-emcc check-ruby4 check-mruby4 serve clean clean-mrbc clean-mrubyc help

all: mrbc mrubyc

emsdk-install:
	$(EMSDK_DIR)/emsdk install $(EMSDK_VERSION)
	$(EMSDK_DIR)/emsdk activate $(EMSDK_VERSION)

check-emcc:
	@command -v $(CC) >/dev/null 2>&1 || { echo "emcc was not found. Run: source emsdk/emsdk_env.sh" >&2; exit 1; }

check-ruby4:
	@$(RUBY) -e 'exit RUBY_VERSION.start_with?("4.") ? 0 : 1' || { echo "Ruby 4.x is required. Set RUBY=/path/to/ruby or install Homebrew Ruby." >&2; exit 1; }
	@PATH="$(HOMEBREW_RUBY_BINDIR):$(PATH)" ruby -e 'exit RUBY_VERSION.start_with?("4.") ? 0 : 1' || { echo "Ruby 4.x must be first in PATH for mruby/c generator scripts." >&2; exit 1; }

check-mruby4:
	@grep -q '^#define MRUBY_RELEASE_MAJOR 4$$' $(MRUBY_VERSION_H) || { echo "mruby 4.x is required." >&2; exit 1; }
	@grep -q '^#define MRUBY_RUBY_VERSION "4\.0"$$' $(MRUBY_VERSION_H) || { echo "Ruby 4.0 compatibility is required." >&2; exit 1; }

mrbc: check-emcc check-ruby4 check-mruby4 $(MRBC_OUTPUT_JS)

$(MRBC_OUTPUT_JS): mruby_build_config.rb Makefile
	PATH="$(HOMEBREW_RUBY_BINDIR):$(PATH)" $(MAKE) -C $(MRUBY_DIR)
	cd $(MRUBY_DIR) && PATH="$(HOMEBREW_RUBY_BINDIR):$(PATH)" $(RUBY) ./minirake MRUBY_CONFIG=../mruby_build_config.rb

mrubyc: check-emcc check-ruby4 mrubyc-autogen $(MRUBYC_OUTPUT_JS)

mrubyc-autogen:
	PATH="$(HOMEBREW_RUBY_BINDIR):$(PATH)" $(MAKE) -C $(MRUBYC_DIR) autogen

$(MRUBYC_OUTPUT_DIR):
	mkdir -p $@

$(MRUBYC_OUTPUT_JS): $(MRUBYC_SRCS) Makefile | $(MRUBYC_OUTPUT_DIR)
	$(CC) $(MRUBYC_CFLAGS) $(MRUBYC_EMFLAGS) $(MRUBYC_SRCS) -o $@

serve:
	$(PYTHON) -m http.server $(PORT) --directory $(PUBLIC_DIR)

clean: clean-mrbc clean-mrubyc

clean-mrbc:
	rm -rf $(BUILD_DIR)/mrbc $(PUBLIC_DIR)/mrbc

clean-mrubyc:
	rm -f $(MRUBYC_OUTPUT_JS) $(MRUBYC_OUTPUT_WASM)

help:
	@echo "Targets:"
	@echo "  make          Build mrbc.js and mrubyc.js"
	@echo "  make emsdk-install"
	@echo "                Install and activate Emscripten $(EMSDK_VERSION)"
	@echo "  make check-ruby4"
	@echo "                Verify the Ruby 4.x executable used for generators"
	@echo "  make mrbc     Build the mruby bytecode compiler for the browser"
	@echo "  make mrubyc   Build the mruby/c VM for the browser"
	@echo "  make serve    Serve public_html on http://localhost:$(PORT)"
	@echo "  make clean    Remove generated WebAssembly artifacts"
