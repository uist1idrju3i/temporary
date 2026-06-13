# EV35L43A mruby/c LED0 Blink

This repository is a minimal mruby/c sample for the Microchip EV35L43A / AVR128DB48 Curiosity Nano. It boots the AVR target, starts the mruby/c VM, and blinks the on-board LED0 from Ruby code.

## What this sample demonstrates

- Running mruby/c directly on an AVR128DB48 target.
- Embedding Ruby bytecode in a C firmware image.
- Exposing a small C hardware abstraction as a Ruby `LED0` class.
- Building the firmware without an MPLAB X `nbproject` directory.

## Hardware

| Item | Value |
| --- | --- |
| Board | Microchip EV35L43A / AVR128DB48 Curiosity Nano |
| MCU | AVR128DB48 |
| Clock source | 16.00 MHz external high-frequency crystal |
| Crystal pins | PA0 / PA1 |
| LED | LED0 on PB3 |
| LED polarity | Active-low |

`main.c` switches the main clock to the mounted 16.00 MHz external crystal. The Makefile sets `F_CPU=16000000UL` to match that clock source.

## Repository layout

| Path | Description |
| --- | --- |
| `main.c` | AVR entry point, clock setup, LED0 GPIO handling, mruby/c initialization, and Ruby class binding. |
| `led0_blink.rb` | Ruby blink loop. |
| `led0_blink_bytecode.c` | C bytecode generated from `led0_blink.rb` by `mrbc`. |
| `Makefile` | XC8 build rules and Ruby bytecode generation helper. |
| `mrubyc/` | mruby/c submodule used by the firmware build. |

Generated build artifacts are written under `build/`.

## Requirements

- macOS development environment.
- Homebrew Ruby 4.0 and Homebrew `mrbc` 4.0 available under `/opt/homebrew`.
- Microchip MPLAB X and XC8 for AVR. This project was tested with XC8 v3.10.
- Microchip AVR-Dx DFP. The default path targets AVR-Dx DFP 2.7.321.
- Initialized `mrubyc` submodule.

If needed, initialize the submodule first:

```sh
git submodule update --init --recursive
```

## Quick start

Check the toolchain:

```sh
make toolcheck
```

Build the firmware:

```sh
make
```

Program the generated HEX file with MPLAB X or MPLAB IPE:

```text
build/ev35l43a_mrubyc_blink.hex
```

## Toolchain paths

The default paths match the local environment used when this sample was created:

| Variable | Default |
| --- | --- |
| `RUBY_PATH_PREFIX` | `/opt/homebrew/opt/ruby/bin:/opt/homebrew/bin` |
| `MP_CC` | `/Applications/microchip/xc8/v3.10/bin/xc8-cc` |
| `MP_CC_DIR` | `/Applications/microchip/xc8/v3.10/bin` |
| `DFP_DIR` | `/Applications/microchip/mplabx/v6.30/packs/Microchip/AVR-Dx_DFP/2.7.321` |

Override them on the `make` command line if your installation uses different paths:

```sh
make \
  RUBY_PATH_PREFIX=/path/to/ruby/bin:/path/to/mrbc/bin \
  MP_CC=/path/to/xc8-cc \
  MP_CC_DIR=/path/to/xc8/bin \
  DFP_DIR=/path/to/AVR-Dx_DFP/version
```

## Regenerate Ruby bytecode

The firmware embeds Ruby bytecode as a C array named `mrbbuf`. Regenerate it after editing `led0_blink.rb`:

```sh
make bytecode
```

Equivalent direct command:

```sh
PATH="/opt/homebrew/opt/ruby/bin:/opt/homebrew/bin:$PATH" \
  mrbc --remove-lv -Bmrbbuf -oled0_blink_bytecode.c led0_blink.rb
```

## Build outputs

`make` creates these primary outputs:

| Output | Description |
| --- | --- |
| `build/ev35l43a_mrubyc_blink.elf` | Linked firmware image. |
| `build/ev35l43a_mrubyc_blink.hex` | Intel HEX file for programming. |
| `build/ev35l43a_mrubyc_blink.map` | Linker map file. |
| `build/memoryfile.xml` | XC8 memory summary. |

The current optimized build reports the following memory usage:

| Memory | Used | Total | Free |
| --- | ---: | ---: | ---: |
| Program | 73,093 bytes | 131,072 bytes | 57,979 bytes |
| Data | 9,493 bytes | 16,384 bytes | 6,891 bytes |

Remove generated files with:

```sh
make clean
```

## Ruby API

The C firmware exposes a Ruby class named `LED0`.

| Ruby method | Behavior |
| --- | --- |
| `LED0.on` | Turns LED0 on. |
| `LED0.off` | Turns LED0 off. |
| `LED0.toggle` | Toggles LED0. |
| `LED0.on?` | Returns `true` when LED0 is currently on. |

The Ruby code uses intuitive LED semantics. The active-low PB3 handling is hidden inside `main.c`.

The bundled Ruby program is intentionally small:

```ruby
while true
  LED0.on
  sleep_ms 500
  LED0.off
  sleep_ms 500
end
```

## Build configuration notes

Important Makefile defaults:

| Setting | Value |
| --- | --- |
| `DEVICE` | `AVR128DB48` |
| `F_CPU` | `16000000UL` |
| `MRBC_MEMORY_SIZE` | `8192` |
| `MRBC_TICK_UNIT` | `MRBC_TICK_UNIT_10_MS` |
| `MRBC_NO_TIMER` | Defined |
| `MRBC_USE_FLOAT` | `0` |
| `MRBC_USE_MATH` | `0` |
| `MRBC_USE_STRING` | `0` |
| `MRBC_SYMBOL_SEARCH_LINEAR` | Defined |
| `MRBC_INSTANCE_DESTRUCTOR` | `0` |
| `MRBC_NO_STDIO` | Defined |

This first blink sample defines `MRBC_NO_TIMER` and uses the AVR mruby/c HAL idle delay path. No MCC timer configuration is required.

## Size optimization notes

The Makefile is tuned for this minimal LED blink firmware. The current build keeps the mruby/c feature set intentionally small:

- `MRBC_USE_FLOAT=0` disables the Ruby `Float` class.
- `MRBC_USE_MATH=0` disables the Ruby `Math` module.
- `MRBC_USE_STRING=0` disables the Ruby `String` class.
- `MRBC_SYMBOL_SEARCH_LINEAR` uses a smaller dynamic symbol table layout than the default tree search mode.
- `MRBC_INSTANCE_DESTRUCTOR=0` removes user-defined instance destructor support.
- `MRBC_NO_STDIO` removes Ruby-level standard I/O methods.

The XC8 compile and link options were checked against the local toolchain help and Microchip documentation before adoption. The current size-related flags are:

| Flag | Purpose |
| --- | --- |
| `-O1` | Baseline optimization level that works with the installed XC8 license. |
| `-mcall-prologues` | Extracts function register save/restore sequences into shared subroutines. This reduced program memory by 2,300 bytes in this project, with a possible execution-time cost. |
| `-ffunction-sections` / `-fdata-sections` | Places functions and data into individual sections so unused sections can be removed. |
| `-Wl,--gc-sections` | Removes unused input sections at link time. |
| `-mconst-data-in-progmem` | Keeps read-only data in program memory instead of copying it to RAM. |
| `-mconst-data-in-config-mapped-progmem` | Places read-only data in the AVR program-memory mapping supported by XC8. |

The following options were investigated but not adopted for this build:

| Option | Result |
| --- | --- |
| `-Os` | Supported by XC8, but ignored by the current local license mode because size optimization requires MPLAB XC8 PRO. |
| `-mpa` / procedural abstraction | Also requires MPLAB XC8 PRO in the current local environment. |
| `-mrelax` | Failed to link this project with the current linker script and read-only data placement. |
| `-O2` / `-O3` | Increased program memory usage for this firmware. |
| `-maccumulate-args` | Increased program memory usage for this firmware. |
| `-mreorder-stack-vars` | No measurable memory improvement. |
| `-fcacheconst` | No measurable memory improvement. |
| `-msmart-io=1` / `-msmart-io=2` | No measurable memory improvement. |
| `-fno-jump-tables` | Increased program memory usage for this firmware. |
