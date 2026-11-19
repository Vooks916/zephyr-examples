# SPDX-License-Identifier: Apache-2.0
cmake_minimum_required(VERSION 3.20.0)

# Out-of-tree binding: <app>/dts/bindings/microchip,mcp4821.yaml.
# Zephyr searches <DTS_ROOT>/dts/bindings; the application directory is a
# DTS root by default. Appending it explicitly (before find_package) is
# harmless (DTS_ROOT is de-duplicated) and documents the dependency.
list(APPEND DTS_ROOT ${CMAKE_CURRENT_SOURCE_DIR})

find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(cnano_explorer_speaker_test)

target_sources(app PRIVATE
  src/main.c
  src/dac_mcp4821.c
)
