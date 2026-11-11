cmake_minimum_required(VERSION 3.28.0)

find_package(Zephyr)
project(i2c_eeprom_app)

target_sources(app PRIVATE src/main.c)
