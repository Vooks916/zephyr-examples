# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.20.0)

find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(bopit_game)

target_include_directories(app PRIVATE src src/hw src/inputs)

target_sources(app PRIVATE
	src/main.c
	src/game.c
	src/ui.c
	src/ring.c
	src/audio.c
	src/hw/ioexp.c
	src/hw/vcnl4200.c
	src/hw/analog.c
	src/inputs/inputs.c
	src/inputs/detector.c
	src/inputs/input_cover.c
	src/inputs/input_touch.c
	src/inputs/input_twirl.c
	src/inputs/input_twist.c
	src/inputs/input_move.c
	src/inputs/input_warm.c
	src/inputs/input_say.c
)
