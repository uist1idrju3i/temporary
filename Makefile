PROJECT_ROOT := .
MRUBYC_DIR := $(PROJECT_ROOT)/mrubyc
BUILD_DIR := build
AUTOGEN_DIR := $(BUILD_DIR)/autogen
TARGET := $(BUILD_DIR)/ev35l43a_mrubyc_blink.elf
HEX := $(BUILD_DIR)/ev35l43a_mrubyc_blink.hex
MAP := $(BUILD_DIR)/ev35l43a_mrubyc_blink.map

DEVICE ?= AVR128DB48
F_CPU ?= 16000000UL
MRBC_MEMORY_SIZE ?= 8192
MRBC_TICK_UNIT ?= MRBC_TICK_UNIT_10_MS
RUBY_PATH_PREFIX ?= /opt/homebrew/opt/ruby/bin:/opt/homebrew/bin
RUBY ?= ruby
MRBC ?= mrbc
MP_CC ?= /Applications/microchip/xc8/v3.10/bin/xc8-cc
MP_CC_DIR ?= /Applications/microchip/xc8/v3.10/bin
DFP_DIR ?= /Applications/microchip/mplabx/v6.30/packs/Microchip/AVR-Dx_DFP/2.7.321

PATH_WITH_HOMEBREW_RUBY = PATH="$(RUBY_PATH_PREFIX):$$PATH"

COMMON_FLAGS := -mcpu=$(DEVICE) -mdfp="$(DFP_DIR)/xc8" -O1 -mcall-prologues -ffunction-sections -fdata-sections -fshort-enums -fno-common -funsigned-char -funsigned-bitfields -Wall -mconst-data-in-progmem -mconst-data-in-config-mapped-progmem
CPPFLAGS := -DF_CPU=$(F_CPU) -DNDEBUG -DMRBC_NO_TIMER -DMRBC_TICK_UNIT=$(MRBC_TICK_UNIT) -DMRBC_MEMORY_SIZE=$(MRBC_MEMORY_SIZE) -DMRBC_USE_FLOAT=0 -DMRBC_USE_MATH=0 -DMRBC_USE_STRING=0 -DMRBC_SYMBOL_SEARCH_LINEAR -DMRBC_INSTANCE_DESTRUCTOR=0 -DMRBC_NO_STDIO -I. -I$(BUILD_DIR) -I$(AUTOGEN_DIR) -I$(MRUBYC_DIR)/src -I$(MRUBYC_DIR)/hal/avr
CFLAGS := $(COMMON_FLAGS) $(CPPFLAGS)
LDFLAGS := -mcpu=$(DEVICE) -mdfp="$(DFP_DIR)/xc8" -Wl,-Map=$(MAP) -Wl,--gc-sections -Wl,--memorysummary,$(BUILD_DIR)/memoryfile.xml
LDLIBS := -Wl,--start-group -Wl,-lm -Wl,--end-group

RUBY_SOURCE := led0_blink.rb
BYTECODE_C := led0_blink_bytecode.c
BYTECODE_SYMBOL := mrbbuf

MRUBYC_SRCS := $(addprefix $(MRUBYC_DIR)/src/, \
  alloc.c c_array.c c_hash.c c_math.c c_numeric.c c_object.c c_proc.c \
  c_range.c c_string.c class.c console.c error.c global.c keyvalue.c \
  load.c mrblib.c rrt0.c symbol.c value.c vm.c)
MRUBYC_OBJS := $(patsubst $(MRUBYC_DIR)/src/%.c,$(BUILD_DIR)/mrubyc/%.o,$(MRUBYC_SRCS))
APP_OBJS := $(BUILD_DIR)/main.o
OBJS := $(APP_OBJS) $(MRUBYC_OBJS)

AUTOGEN_SYMBOL_TABLE := $(AUTOGEN_DIR)/_autogen_builtin_symbol.h
AUTOGEN_CLASS_TABLE := $(AUTOGEN_DIR)/_autogen_builtin_class.h
AUTOGEN_METHOD_TABLE := \
  $(AUTOGEN_DIR)/_autogen_class_array.h \
  $(AUTOGEN_DIR)/_autogen_class_exception.h \
  $(AUTOGEN_DIR)/_autogen_class_float.h \
  $(AUTOGEN_DIR)/_autogen_class_hash.h \
  $(AUTOGEN_DIR)/_autogen_class_integer.h \
  $(AUTOGEN_DIR)/_autogen_module_math.h \
  $(AUTOGEN_DIR)/_autogen_class_object.h \
  $(AUTOGEN_DIR)/_autogen_class_proc.h \
  $(AUTOGEN_DIR)/_autogen_class_range.h \
  $(AUTOGEN_DIR)/_autogen_class_string.h \
  $(AUTOGEN_DIR)/_autogen_class_symbol.h \
  $(AUTOGEN_DIR)/_autogen_class_rrt0.h
AUTOGEN_UNICODE_CASE := $(AUTOGEN_DIR)/_autogen_unicode_case.h
AUTOGEN_FILES := $(AUTOGEN_SYMBOL_TABLE) $(AUTOGEN_CLASS_TABLE) $(AUTOGEN_METHOD_TABLE) $(AUTOGEN_UNICODE_CASE)

METHOD_CLASS_SRCS := \
  $(MRUBYC_DIR)/src/c_array.c \
  $(MRUBYC_DIR)/src/c_hash.c \
  $(MRUBYC_DIR)/src/c_math.c \
  $(MRUBYC_DIR)/src/c_numeric.c \
  $(MRUBYC_DIR)/src/c_object.c \
  $(MRUBYC_DIR)/src/c_proc.c \
  $(MRUBYC_DIR)/src/c_range.c \
  $(MRUBYC_DIR)/src/c_string.c \
  $(MRUBYC_DIR)/src/symbol.c \
  $(MRUBYC_DIR)/src/error.c
SYMBOL_SRCS := $(METHOD_CLASS_SRCS) $(MRUBYC_DIR)/src/rrt0.c

.PHONY: all bytecode clean toolcheck

all: $(HEX)

bytecode: $(BYTECODE_C)

toolcheck:
	$(PATH_WITH_HOMEBREW_RUBY) $(RUBY) -v
	$(PATH_WITH_HOMEBREW_RUBY) $(MRBC) --version
	$(MP_CC) --version

$(BYTECODE_C): $(RUBY_SOURCE)
	$(PATH_WITH_HOMEBREW_RUBY) $(MRBC) --remove-lv -B$(BYTECODE_SYMBOL) -o$@ $<

$(HEX): $(TARGET)
	$(MP_CC_DIR)/avr-objcopy -O ihex "$<" "$@"

$(TARGET): $(OBJS)
	@mkdir -p $(@D)
	$(MP_CC) $(LDFLAGS) -o $@ $(OBJS) $(LDLIBS)

$(BUILD_DIR)/main.o: main.c $(BYTECODE_C) $(AUTOGEN_FILES)
	@mkdir -p $(@D)
	$(MP_CC) -c $(CFLAGS) -o $@ $<

$(BUILD_DIR)/mrubyc/%.o: $(MRUBYC_DIR)/src/%.c $(AUTOGEN_FILES)
	@mkdir -p $(@D)
	$(MP_CC) -c $(CFLAGS) -o $@ $<

$(AUTOGEN_SYMBOL_TABLE): $(SYMBOL_SRCS) $(MRUBYC_DIR)/mrblib/*.rb
	@mkdir -p $(@D)
	$(PATH_WITH_HOMEBREW_RUBY) $(RUBY) $(MRUBYC_DIR)/support/make_symbol_table.rb --path-c $(MRUBYC_DIR)/src --path-rb $(MRUBYC_DIR)/mrblib -o $@

$(AUTOGEN_CLASS_TABLE): $(METHOD_CLASS_SRCS)
	@mkdir -p $(@D)
	$(PATH_WITH_HOMEBREW_RUBY) $(RUBY) $(MRUBYC_DIR)/support/make_class_table.rb -o $@ $(METHOD_CLASS_SRCS)

$(AUTOGEN_DIR)/_autogen_class_array.h: $(MRUBYC_DIR)/src/c_array.c $(AUTOGEN_SYMBOL_TABLE)
	@mkdir -p $(@D)
	$(PATH_WITH_HOMEBREW_RUBY) $(RUBY) $(MRUBYC_DIR)/support/make_method_table.rb --output-dir $(AUTOGEN_DIR) $<

$(AUTOGEN_DIR)/_autogen_class_exception.h: $(MRUBYC_DIR)/src/error.c $(AUTOGEN_SYMBOL_TABLE)
	@mkdir -p $(@D)
	$(PATH_WITH_HOMEBREW_RUBY) $(RUBY) $(MRUBYC_DIR)/support/make_method_table.rb --output-dir $(AUTOGEN_DIR) $<

$(AUTOGEN_DIR)/_autogen_class_float.h $(AUTOGEN_DIR)/_autogen_class_integer.h: $(MRUBYC_DIR)/src/c_numeric.c $(AUTOGEN_SYMBOL_TABLE)
	@mkdir -p $(@D)
	$(PATH_WITH_HOMEBREW_RUBY) $(RUBY) $(MRUBYC_DIR)/support/make_method_table.rb --output-dir $(AUTOGEN_DIR) $<

$(AUTOGEN_DIR)/_autogen_class_hash.h: $(MRUBYC_DIR)/src/c_hash.c $(AUTOGEN_SYMBOL_TABLE)
	@mkdir -p $(@D)
	$(PATH_WITH_HOMEBREW_RUBY) $(RUBY) $(MRUBYC_DIR)/support/make_method_table.rb --output-dir $(AUTOGEN_DIR) $<

$(AUTOGEN_DIR)/_autogen_module_math.h: $(MRUBYC_DIR)/src/c_math.c $(AUTOGEN_SYMBOL_TABLE)
	@mkdir -p $(@D)
	$(PATH_WITH_HOMEBREW_RUBY) $(RUBY) $(MRUBYC_DIR)/support/make_method_table.rb --output-dir $(AUTOGEN_DIR) $<

$(AUTOGEN_DIR)/_autogen_class_object.h: $(MRUBYC_DIR)/src/c_object.c $(AUTOGEN_SYMBOL_TABLE)
	@mkdir -p $(@D)
	$(PATH_WITH_HOMEBREW_RUBY) $(RUBY) $(MRUBYC_DIR)/support/make_method_table.rb --output-dir $(AUTOGEN_DIR) $<

$(AUTOGEN_DIR)/_autogen_class_proc.h: $(MRUBYC_DIR)/src/c_proc.c $(AUTOGEN_SYMBOL_TABLE)
	@mkdir -p $(@D)
	$(PATH_WITH_HOMEBREW_RUBY) $(RUBY) $(MRUBYC_DIR)/support/make_method_table.rb --output-dir $(AUTOGEN_DIR) $<

$(AUTOGEN_DIR)/_autogen_class_range.h: $(MRUBYC_DIR)/src/c_range.c $(AUTOGEN_SYMBOL_TABLE)
	@mkdir -p $(@D)
	$(PATH_WITH_HOMEBREW_RUBY) $(RUBY) $(MRUBYC_DIR)/support/make_method_table.rb --output-dir $(AUTOGEN_DIR) $<

$(AUTOGEN_DIR)/_autogen_class_string.h: $(MRUBYC_DIR)/src/c_string.c $(AUTOGEN_SYMBOL_TABLE)
	@mkdir -p $(@D)
	$(PATH_WITH_HOMEBREW_RUBY) $(RUBY) $(MRUBYC_DIR)/support/make_method_table.rb --output-dir $(AUTOGEN_DIR) $<

$(AUTOGEN_DIR)/_autogen_class_symbol.h: $(MRUBYC_DIR)/src/symbol.c $(AUTOGEN_SYMBOL_TABLE)
	@mkdir -p $(@D)
	$(PATH_WITH_HOMEBREW_RUBY) $(RUBY) $(MRUBYC_DIR)/support/make_method_table.rb --output-dir $(AUTOGEN_DIR) $<

$(AUTOGEN_DIR)/_autogen_class_rrt0.h: $(MRUBYC_DIR)/src/rrt0.c $(AUTOGEN_SYMBOL_TABLE)
	@mkdir -p $(@D)
	$(PATH_WITH_HOMEBREW_RUBY) $(RUBY) $(MRUBYC_DIR)/support/make_method_table.rb --output-dir $(AUTOGEN_DIR) $<

$(AUTOGEN_UNICODE_CASE):
	@mkdir -p $(@D)
	@touch $@

clean:
	rm -rf $(BUILD_DIR) $(BYTECODE_C)
