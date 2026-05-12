# SPDX-License-Identifier: BSD-3-Clause

cmake_minimum_required(VERSION 3.20.0)
find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(test-mrubyc-zephyr-hal)

# Note: mrubyc autogen files are already generated manually
# To regenerate: cd mrubyc && PATH=/opt/homebrew/opt/ruby@3.3/bin:$PATH make autogen

target_sources(app PRIVATE
    src/main.c
    mrubyc/src/alloc.c
    mrubyc/src/c_array.c
    mrubyc/src/c_hash.c
    mrubyc/src/c_math.c
    mrubyc/src/c_numeric.c
    mrubyc/src/c_object.c
    mrubyc/src/c_proc.c
    mrubyc/src/c_range.c
    mrubyc/src/c_string.c
    mrubyc/src/class.c
    mrubyc/src/console.c
    mrubyc/src/error.c
    mrubyc/src/global.c
    mrubyc/src/keyvalue.c
    mrubyc/src/load.c
    mrubyc/src/mrblib.c
    mrubyc/src/rrt0.c
    mrubyc/src/symbol.c
    mrubyc/src/value.c
    mrubyc/src/vm.c
    mrubyc/hal/zephyr/hal.c
)

target_include_directories(app PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/mrubyc/src
    ${CMAKE_CURRENT_SOURCE_DIR}/mrubyc/hal/zephyr
    ${CMAKE_CURRENT_SOURCE_DIR}/src/rb
)
