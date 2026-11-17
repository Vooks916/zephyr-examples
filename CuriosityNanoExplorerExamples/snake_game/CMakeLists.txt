cmake_minimum_required(VERSION 3.28.0)

find_package(Zephyr)
project(snake_game)

target_sources(app PRIVATE src/main.c)
