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
| Primary MCU | AVR128DB48 |
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
- Microchip ATmega DFP 3.5.296 when building the optional ATmega128 matrix target.
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

The default Makefile build and the smallest XC8 PRO trial matrix build report:

| Build | Program used | Program free | Data used | Data free |
| --- | ---: | ---: | ---: | ---: |
| Default Makefile baseline | 73,093 bytes | 57,979 bytes | 9,493 bytes | 6,891 bytes |
| Best XC8 PRO matrix result | 53,344 bytes | 77,728 bytes | 9,493 bytes | 6,891 bytes |

The best matrix result was produced in an isolated `build/matrix*` output directory; it is not the default `make` result unless the flags listed in the matrix section are applied.

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

The Makefile is tuned for this minimal LED blink firmware. The build keeps the mruby/c feature set intentionally small:

- `MRBC_USE_FLOAT=0` disables the Ruby `Float` class.
- `MRBC_USE_MATH=0` disables the Ruby `Math` module.
- `MRBC_USE_STRING=0` disables the Ruby `String` class.
- `MRBC_SYMBOL_SEARCH_LINEAR` uses a smaller dynamic symbol table layout than the default tree search mode.
- `MRBC_INSTANCE_DESTRUCTOR=0` removes user-defined instance destructor support.
- `MRBC_NO_STDIO` removes Ruby-level standard I/O methods.

The XC8 compile and link options were checked against the local toolchain help and the installed Microchip documentation before running the matrix:

| Source | Purpose |
| --- | --- |
| `xc8-cc --version` | Confirmed Microchip MPLAB XC8 C Compiler V3.10, build date Aug 13 2025. |
| `xc8-cc --help` | Top-level driver options such as `-O0`, `-O1`, `-O2`, `-O3`, `-Og`, `-Os`, `-fcacheconst`, `--memorysummary`, and Smart-IO flags. |
| `xc8-cc -mcpu=AVR128DB48 -mdfp=<DFP>/xc8 --target-help` | AVR target options such as `-mcall-prologues`, `-mconst-data-in-progmem`, `-mconst-data-in-config-mapped-progmem`, `-mrelax`, `-mshort-calls`, `-mpa-*`, `-mreorder-stack-vars`, and `-msmart-io`. |
| `xc8-cc -mcpu=AVR128DB48 -mdfp=<DFP>/xc8 -Q --help=<category>` | Effective defaults for optimizer, target, common, warning, and parameter categories. |
| `avr-ld --help` | Linker options, especially `--gc-sections`, `--relax`, and linker `-O`. |
| `Readme_XC8_for_AVR.htm` and XC8 AVR user documents | License/optimization notes, LTO notes, and procedural abstraction controls. |

The option inventory was classified before building. Target or semantic-changing options such as `-mint8`, `-mno-data-init`, `-nodevicelib`, and `-mno-fallback` were documented but not used in the firmware matrix because they change ABI, startup, device-library, or license-fallback behavior rather than simply optimizing the same program.

### XC8 PRO trial matrix

The matrix used the same mruby/c feature macros and compared every successful case against the default Makefile baseline:

| Baseline | Program | Data |
| --- | ---: | ---: |
| `-O1 -mcall-prologues -ffunction-sections -fdata-sections ... -Wl,--gc-sections` | 73,093 bytes | 9,493 bytes |

The full local matrix covered 81 build entries. Representative results are:

| Case | Status | Program | Delta | Data | Notes |
| --- | --- | ---: | ---: | ---: | --- |
| Default Makefile baseline | OK | 73,093 | +0 | 9,493 | Current `-O1` baseline. |
| Plain `-O1` without size extras | OK | 85,616 | +12,523 | 9,539 | Shows the value of section GC and prologue sharing. |
| `-O0` | OK | 107,728 | +34,635 | 9,993 | Debug/no optimization is much larger. |
| `-O2` | OK | 74,138 | +1,045 | 9,493 | Larger than `-O1` for this firmware. |
| `-O3` / `-Ofast` | OK | 96,794 | +23,701 | 9,493 | Speed-oriented optimization greatly increases code size. |
| `-Og` | OK | 73,777 | +684 | 9,493 | Debug-oriented optimization is slightly larger than `-O1`. |
| `-Os` | OK | 56,936 | -16,157 | 9,493 | XC8 PRO size optimization is effective. |
| `-O1 -flto` | OK | 71,991 | -1,102 | 9,493 | LTO helps even with the old `-O1` baseline. |
| `-Os -flto` | OK | 54,803 | -18,290 | 9,493 | Best result before linker relaxation. |
| `-Os -flto -Wl,--relax` | OK | 53,344 | -19,749 | 9,493 | Smallest tested result. |
| `-Os` without `-mcall-prologues` | OK | 59,390 | -13,703 | 9,493 | `-mcall-prologues` still saves 2,454 bytes at `-Os`. |
| `-Os -flto -Wl,--relax` without `-mcall-prologues` | OK | 55,948 | -17,145 | 9,493 | Prologue sharing still helps with LTO and linker relaxation. |
| `-O1` without `-ffunction-sections` | OK | 83,080 | +9,987 | 9,539 | Function sections are important for the non-LTO baseline. |
| `-O1` without `-fdata-sections` | OK | 73,318 | +225 | 9,493 | Data sections have a small effect here. |
| `-O1` without `--gc-sections` | OK | 83,096 | +10,003 | 9,539 | Linker garbage collection is important for the non-LTO baseline. |
| `-Os -flto -Wl,--relax` without section splitting | OK | 53,344 | -19,749 | 9,493 | LTO made section splitting neutral in this sample. |
| `-Os -flto -Wl,--relax` without `--gc-sections` | OK | 53,344 | -19,749 | 9,493 | LTO made linker GC neutral in this sample. |
| Const-data placement variants | OK | 53,344 | -19,749 | 9,493 | No size difference for this blink program, but the flags remain appropriate for AVR read-only data placement. |
| `-mpa` / `-mpa-iterations=4` / `-mpa-callcost-shortcall` | OK | 53,344 | -19,749 | 9,493 | Procedural abstraction produced no additional saving beyond `-Os -flto -Wl,--relax`. |
| `-mshort-calls` / driver `-mrelax` | OK | 53,344 | -19,749 | 9,493 | No additional saving in the best LTO configuration. |
| Direct `-Wl,--relax` without LTO | FAIL | - | - | - | Failed in the linker script with a non-constant `.rodata.method_symbols_FalseClass%z2` expression. |
| `-maccumulate-args` with best flags | OK | 53,482 | -19,611 | 9,493 | Increased program size by 138 bytes versus the best result. |
| `-mreorder-stack-vars` with best flags | OK | 53,344 | -19,749 | 9,493 | No measurable size change. |
| `-msmart-io=0` / `-msmart-io=1` / `-msmart-io=2` | OK | 53,344 | -19,749 | 9,493 | No measurable size change for this program. |
| `-fno-jump-tables` with best flags | OK | 54,056 | -19,037 | 9,493 | Increased program size by 712 bytes versus the best result. |
| `-fcacheconst` | OK | 53,344 | -19,749 | 9,493 | XC8 emitted warning 1428: this option is not supported and is ignored. |

The smallest tested PRO trial configuration keeps the existing mruby/c feature macros and uses:

| Area | Flags |
| --- | --- |
| Compiler | `-Os -flto -mcall-prologues -ffunction-sections -fdata-sections -fshort-enums -fno-common -funsigned-char -funsigned-bitfields -Wall -mconst-data-in-progmem -mconst-data-in-config-mapped-progmem` |
| Linker | `-Wl,--gc-sections -Wl,--relax -Wl,--memorysummary,<path>/memoryfile.xml` |

`-ffunction-sections`, `-fdata-sections`, and `--gc-sections` did not change the final `-Os -flto -Wl,--relax` size in this sample, but they remain useful for the non-LTO fallback and are harmless in the tested LTO build. Procedural abstraction (`-mpa`) is available under the PRO trial but is not part of the recommended flag set because it produced no measurable saving for this firmware.

## mruby/c feature × XC8 option matrix

The matrix was run with the same 81 XC8 option cases used in the previous XC8 PRO trial matrix, for 15 mruby/c feature profiles and two MCU targets.

| Item | Value |
| --- | --- |
| Total entries | 2,430 (`2 MCU × 15 profiles × 81 XC8 cases`) |
| Successful entries | 1,116 |
| Failed entries | 1,314 |
| AVR128DB48 DFP | AVR-Dx DFP 2.7.321 |
| ATmega128 DFP | ATmega DFP 3.5.296 |
| AVR128DB48 heap | `MRBC_MEMORY_SIZE=8192` |
| ATmega128 heap | `MRBC_MEMORY_SIZE=512` for feature comparison; minimal profile also fits at 1536 bytes in the preflight check |
| Raw local results | `build/mrubyc_matrix/results.csv` and `build/mrubyc_matrix/results.json` |

### mruby/c feature profiles

| Profile | FLOAT | MATH | STRING | Symbol search | Destructor | stdio | Notes |
| --- | ---: | ---: | ---: | --- | ---: | --- | --- |
| `minimal` | 0 | 0 | 0 | linear | 0 | off | Smallest baseline used by the Makefile. |
| `float1` | 1 | 0 | 0 | linear | 0 | off | Float enabled with `MRBC_USE_FLOAT=1`. |
| `float2` | 2 | 0 | 0 | linear | 0 | off | Float enabled with `MRBC_USE_FLOAT=2`. |
| `math1_float0` | 0 | 1 | 0 | linear | 0 | off | Invalid in this build: Math enabled while Float is disabled. |
| `math1_float2` | 2 | 1 | 0 | linear | 0 | off | Math enabled with double Float support. |
| `string1` | 0 | 0 | 1 | linear | 0 | off | String enabled only. |
| `btree_symbols` | 0 | 0 | 0 | btree | 0 | off | Default btree symbol search instead of linear search. |
| `dtor1` | 0 | 0 | 0 | linear | 1 | off | User-defined instance destructor support enabled. |
| `stdio_on` | 0 | 0 | 0 | linear | 0 | on | Ruby stdio methods enabled by omitting `MRBC_NO_STDIO`. |
| `defaultish` | 2 | 0 | 1 | btree | 1 | on | Close to upstream defaults: Float, String, btree symbols, destructor, stdio. |
| `full` | 2 | 1 | 1 | btree | 1 | on | Defaultish plus Math. |
| `math1_float1` | 1 | 1 | 0 | linear | 0 | off | Math enabled with single-precision Float. |
| `float_string` | 2 | 0 | 1 | linear | 0 | off | Float + String without other extras. |
| `string_stdio` | 0 | 0 | 1 | linear | 0 | on | String + stdio, a natural pair. |
| `practical` | 2 | 0 | 1 | linear | 1 | on | Like `defaultish` but with linear symbol search. |

### AVR128DB48: best successful case per feature profile

| Profile | OK / 81 | Best XC8 case | Program | Δ program vs minimal | Data | Δ data vs minimal |
| --- | ---: | --- | ---: | ---: | ---: | ---: |
| `minimal` | 80 | `os_flto_wl_relax_sections_gc_const_both` | 53,344 | +0 | 9,493 | +0 |
| `float1` | 80 | `os_flto_wl_relax_smart_io_2_sections_gc_const_both` | 60,811 | +7,467 | 9,542 | +49 |
| `float2` | 80 | `os_flto_wl_relax_smart_io_2_sections_gc_const_both` | 60,811 | +7,467 | 9,542 | +49 |
| `math1_float0` | 0 | FAIL | - | - | - | - |
| `math1_float2` | 75 | `os_flto_wl_relax_smart_io_2_sections_gc_const_both` | 78,803 | +25,459 | 9,628 | +135 |
| `string1` | 80 | `os_flto_wl_relax_sections_gc_const_both` | 64,882 | +11,538 | 9,505 | +12 |
| `btree_symbols` | 80 | `os_flto_wl_relax_sections_gc_const_both` | 53,372 | +28 | 10,003 | +510 |
| `dtor1` | 80 | `os_flto_wl_relax_sections_gc_const_both` | 53,388 | +44 | 9,493 | +0 |
| `stdio_on` | 80 | `os_flto_wl_relax_sections_gc_const_both` | 54,780 | +1,436 | 9,493 | +0 |
| `defaultish` | 76 | `os_flto_wl_relax_smart_io_2_sections_gc_const_both` | 79,781 | +26,437 | 10,158 | +665 |
| `full` | 52 | `os_flto_wl_relax_smart_io_2_sections_gc_const_both` | 97,803 | +44,459 | 10,244 | +751 |
| `math1_float1` | 75 | `os_flto_wl_relax_smart_io_2_sections_gc_const_both` | 78,803 | +25,459 | 9,628 | +135 |
| `float_string` | 76 | `os_flto_wl_relax_smart_io_2_sections_gc_const_both` | 75,921 | +22,577 | 9,632 | +139 |
| `string_stdio` | 78 | `os_flto_wl_relax_sections_gc_const_both` | 66,456 | +13,112 | 9,505 | +12 |
| `practical` | 76 | `os_flto_wl_relax_smart_io_2_sections_gc_const_both` | 79,749 | +26,405 | 9,648 | +155 |

### ATmega128: best successful case per feature profile

| Profile | OK / 81 | Best XC8 case | Program | Δ program vs minimal | Data | Δ data vs minimal |
| --- | ---: | --- | ---: | ---: | ---: | ---: |
| `minimal` | 4 | `os_call_sections_gc_no_const` | 66,178 | +0 | 2,846 | +0 |
| `float1` | 4 | `os_call_sections_gc_no_const` | 81,032 | +14,854 | 2,862 | +16 |
| `float2` | 4 | `os_call_sections_gc_no_const` | 81,032 | +14,854 | 2,862 | +16 |
| `math1_float0` | 0 | FAIL | - | - | - | - |
| `math1_float2` | 4 | `os_call_sections_gc_no_const` | 100,876 | +34,698 | 2,886 | +40 |
| `string1` | 4 | `os_call_sections_gc_no_const` | 81,850 | +15,672 | 2,890 | +44 |
| `btree_symbols` | 4 | `os_call_sections_gc_no_const` | 66,196 | +18 | 3,356 | +510 |
| `dtor1` | 4 | `os_call_sections_gc_no_const` | 66,212 | +34 | 2,846 | +0 |
| `stdio_on` | 4 | `os_call_sections_gc_no_const` | 68,352 | +2,174 | 2,846 | +0 |
| `defaultish` | 2 | `os_call_sections_gc_no_const` | 104,438 | +38,260 | 3,416 | +570 |
| `full` | 2 | `os_call_sections_gc_no_const` | 124,282 | +58,104 | 3,440 | +594 |
| `math1_float1` | 4 | `os_call_sections_gc_no_const` | 100,876 | +34,698 | 2,886 | +40 |
| `float_string` | 2 | `os_call_sections_gc_no_const` | 101,888 | +35,710 | 2,906 | +60 |
| `string_stdio` | 4 | `os_call_sections_gc_no_const` | 84,234 | +18,056 | 2,890 | +44 |
| `practical` | 2 | `os_call_sections_gc_no_const` | 104,414 | +38,236 | 2,906 | +60 |

### AVR128DB48: representative optimization modes

Values are `Program / Data` bytes. `Os+LTO+relax+smart-io=2` is included because it was smaller for Float/Math-heavy profiles.

| Profile | O1 default | O0 | Og | O2 | O3 | Ofast | Os | Os+LTO+relax | Os+LTO+relax+smart-io=2 |
| --- | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: |
| `minimal` | 73,093 / 9,493 | 107,728 / 9,993 | 73,777 / 9,493 | 74,138 / 9,493 | 96,794 / 9,493 | 96,794 / 9,493 | 56,936 / 9,493 | 53,344 / 9,493 | 53,344 / 9,493 |
| `float1` | 88,775 / 9,574 | 129,078 / 10,074 | 88,971 / 9,574 | 88,896 / 9,574 | 118,040 / 9,574 | 118,040 / 9,574 | 70,982 / 9,574 | 66,999 / 9,574 | 60,811 / 9,542 |
| `float2` | 88,775 / 9,574 | 129,078 / 10,074 | 88,971 / 9,574 | 88,896 / 9,574 | 118,040 / 9,574 | 118,040 / 9,574 | 70,982 / 9,574 | 66,999 / 9,574 | 60,811 / 9,542 |
| `math1_float0` | FAIL | FAIL | FAIL | FAIL | FAIL | FAIL | FAIL | FAIL | FAIL |
| `math1_float2` | 108,371 / 9,660 | FAIL | 108,543 / 9,660 | 108,512 / 9,660 | FAIL | FAIL | 90,470 / 9,660 | 84,993 / 9,660 | 78,803 / 9,628 |
| `string1` | 89,343 / 9,505 | FAIL | 89,737 / 9,505 | 89,962 / 9,505 | 128,955 / 9,505 | 128,955 / 9,505 | 69,172 / 9,505 | 64,882 / 9,505 | 64,882 / 9,505 |
| `btree_symbols` | 73,267 / 10,003 | 108,080 / 10,503 | 73,969 / 10,003 | 74,292 / 10,003 | 96,990 / 10,003 | 96,990 / 10,003 | 56,976 / 10,003 | 53,372 / 10,003 | 53,372 / 10,003 |
| `dtor1` | 73,125 / 9,493 | 107,794 / 9,993 | 73,807 / 9,493 | 74,172 / 9,493 | 96,828 / 9,493 | 96,828 / 9,493 | 56,970 / 9,493 | 53,388 / 9,493 | 53,388 / 9,493 |
| `stdio_on` | 75,081 / 9,493 | 111,258 / 9,993 | 75,825 / 9,493 | 76,166 / 9,493 | 103,640 / 9,493 | 103,640 / 9,493 | 58,460 / 9,493 | 54,780 / 9,493 | 54,780 / 9,493 |
| `defaultish` | 112,543 / 10,174 | FAIL | 112,297 / 10,174 | 112,532 / 10,174 | FAIL | FAIL | 89,610 / 10,174 | 83,999 / 10,174 | 79,781 / 10,158 |
| `full` | FAIL | FAIL | FAIL | FAIL | FAIL | FAIL | 109,098 / 10,260 | 102,029 / 10,260 | 97,803 / 10,244 |
| `math1_float1` | 108,371 / 9,660 | FAIL | 108,543 / 9,660 | 108,512 / 9,660 | FAIL | FAIL | 90,470 / 9,660 | 84,993 / 9,660 | 78,803 / 9,628 |
| `float_string` | 109,987 / 9,664 | FAIL | 109,721 / 9,664 | 110,070 / 9,664 | FAIL | FAIL | 87,794 / 9,664 | 82,071 / 9,664 | 75,921 / 9,632 |
| `string_stdio` | 91,583 / 9,505 | FAIL | 92,017 / 9,505 | 92,208 / 9,505 | FAIL | FAIL | 70,844 / 9,505 | 66,456 / 9,505 | 66,456 / 9,505 |
| `practical` | 112,369 / 9,664 | FAIL | 112,105 / 9,664 | 112,386 / 9,664 | FAIL | FAIL | 89,570 / 9,664 | 83,963 / 9,664 | 79,749 / 9,648 |

### ATmega128: successful cases from the same 81-case XC8 matrix

The exact same 81 XC8 cases were run. Most AVR-Dx `const_both` and LTO cases fail on ATmega128 because `-mconst-data-in-config-mapped-progmem` is not appropriate for the classic AVR target, LTO triggers `mrblib_bytecode` address-space conflicts, or the 4 KB SRAM budget is exceeded. The successful comparable rows are the previous matrix variants that omit the config-mapped const-data flag.

| Profile | OK / 81 | O1 no_const | O1 const_progmem | Os no_const | Os const_progmem | Best |
| --- | ---: | ---: | ---: | ---: | ---: | --- |
| `minimal` | 4 | 87,883 / 2,846 | 87,883 / 2,846 | 66,178 / 2,846 | 66,178 / 2,846 | `os_call_sections_gc_no_const` (66,178 / 2,846) |
| `float1` | 4 | 104,479 / 2,862 | 104,479 / 2,862 | 81,032 / 2,862 | 81,032 / 2,862 | `os_call_sections_gc_no_const` (81,032 / 2,862) |
| `float2` | 4 | 104,479 / 2,862 | 104,479 / 2,862 | 81,032 / 2,862 | 81,032 / 2,862 | `os_call_sections_gc_no_const` (81,032 / 2,862) |
| `math1_float0` | 0 | FAIL | FAIL | FAIL | FAIL | FAIL |
| `math1_float2` | 4 | 124,443 / 2,886 | 124,443 / 2,886 | 100,876 / 2,886 | 100,876 / 2,886 | `os_call_sections_gc_no_const` (100,876 / 2,886) |
| `string1` | 4 | 108,505 / 2,890 | 108,505 / 2,890 | 81,850 / 2,890 | 81,850 / 2,890 | `os_call_sections_gc_no_const` (81,850 / 2,890) |
| `btree_symbols` | 4 | 88,001 / 3,356 | 88,001 / 3,356 | 66,196 / 3,356 | 66,196 / 3,356 | `os_call_sections_gc_no_const` (66,196 / 3,356) |
| `dtor1` | 4 | 87,915 / 2,846 | 87,915 / 2,846 | 66,212 / 2,846 | 66,212 / 2,846 | `os_call_sections_gc_no_const` (66,212 / 2,846) |
| `stdio_on` | 4 | 90,689 / 2,846 | 90,689 / 2,846 | 68,352 / 2,846 | 68,352 / 2,846 | `os_call_sections_gc_no_const` (68,352 / 2,846) |
| `defaultish` | 2 | FAIL | FAIL | 104,438 / 3,416 | 104,438 / 3,416 | `os_call_sections_gc_no_const` (104,438 / 3,416) |
| `full` | 2 | FAIL | FAIL | 124,282 / 3,440 | 124,282 / 3,440 | `os_call_sections_gc_no_const` (124,282 / 3,440) |
| `math1_float1` | 4 | 124,443 / 2,886 | 124,443 / 2,886 | 100,876 / 2,886 | 100,876 / 2,886 | `os_call_sections_gc_no_const` (100,876 / 2,886) |
| `float_string` | 2 | FAIL | FAIL | 101,888 / 2,906 | 101,888 / 2,906 | `os_call_sections_gc_no_const` (101,888 / 2,906) |
| `string_stdio` | 4 | 111,635 / 2,890 | 111,635 / 2,890 | 84,234 / 2,890 | 84,234 / 2,890 | `os_call_sections_gc_no_const` (84,234 / 2,890) |
| `practical` | 2 | FAIL | FAIL | 104,414 / 2,906 | 104,414 / 2,906 | `os_call_sections_gc_no_const` (104,414 / 2,906) |

### Matrix conclusions

#### Size optimization

- On AVR128DB48, the smallest minimal build remains `-Os -flto -Wl,--relax` at **53,344 / 9,493** bytes.
- `-Os -flto -Wl,--relax -msmart-io=2` is the best flag set for Float- or Math-enabled profiles.
- ATmega128 can build the minimal profile with a 512-byte mruby/c heap: **66,178 / 2,846** bytes (`-Os -mcall-prologues -ffunction-sections -fdata-sections -Wl,--gc-sections`).

#### Float precision

- `MRBC_USE_FLOAT=1` (single) and `MRBC_USE_FLOAT=2` (double) produced identical footprints in this firmware for both MCU targets.
- `math1_float1` and `math1_float2` are also identical (78,803 / 9,628 on AVR128DB48), confirming float precision does not affect Math module size.

#### Feature costs (AVR128DB48 best case, Δ vs minimal)

| Feature | Δ Program | Δ Data | Notes |
| --- | ---: | ---: | --- |
| Float (single or double) | +7,467 | +49 | With Smart-IO=2. |
| Math + Float | +25,459 | +135 | With Smart-IO=2. |
| String | +11,538 | +12 | |
| Float + String | +22,577 | +139 | Sum of individual: +18,905. Extra +3,672 from Float×String interaction code. |
| String + stdio | +13,112 | +12 | Sum of individual: +12,974. Nearly additive (+138 interaction). |
| btree symbol search | +28 | +510 | Program cost negligible; data cost is the symbol table structure. |
| Instance destructor | +44 | +0 | Nearly free. |
| stdio (without String) | +1,436 | +0 | |
| `practical` (Float+String+dtor+stdio, linear) | +26,405 | +155 | |
| `defaultish` (practical + btree) | +26,437 | +665 | vs `practical`: btree adds +32 program, +510 data. |
| `full` (defaultish + Math) | +44,459 | +751 | |

#### Additivity analysis

- **Float + String**: predicted from individual deltas = 7,467 + 11,538 = +18,905 program. Actual = +22,577. The +3,672 byte excess indicates Float×String interaction code (e.g. `Float#to_s`, `String#to_f` type conversion paths).
- **String + stdio**: predicted = 11,538 + 1,436 = +12,974 program. Actual = +13,112. Nearly perfectly additive (+138 byte difference), showing these features share almost no code.
- **`practical` vs `defaultish`**: the only difference is btree→linear symbol search. Switching to linear saves 510 bytes of data with negligible program cost (+32 bytes).
- **`math1_float0`** failed in all cases for both MCUs. `MRBC_USE_MATH=1` effectively requires `MRBC_USE_FLOAT≥1`.

#### ATmega128 constraints

- Only 4 of 81 XC8 cases succeed for most profiles (the `-Os`/`-O1` variants without `-mconst-data-in-config-mapped-progmem` and without LTO).
- Heavy profiles (`float_string`, `defaultish`, `practical`, `full`) only succeed with `-Os` due to 131 KB flash overflow at `-O1`.
- 4 KB SRAM is tight: `btree_symbols` uses 3,356 of 4,096 data bytes; `full` uses 3,440 bytes, leaving only 656 bytes for runtime stack.
- Practical ATmega128 recommendation: linear symbols, no stdio, no String/Float/Math unless required by the application.
