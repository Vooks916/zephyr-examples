# Overview

This is a sample program that uses the temperature sensor on the Curiosity Nano Explorer.

# Description

This program displays the ambient temperature in Celsius on the OLED display. The displayed temperature is refreshed every second.

# Support

This program runs on the SAM D21 Curiosity Nano and SAM E51 Curiosity Nano boards.

# Build and Flash Instructions

## Board Information

| Board                  | Board Name   | Pack         |
|------------------------|--------------|--------------|
| SAM D21 Curiosity Nano | samd21_cnano | atsamd21g17d |
| SAM E51 Curiosity Nano | same51_cnano | atsame51j20a |

## Instructions

1. Install the PyOCD pack for the board (replace `<pack>` with the pack for the board):

```
pyocd pack install <pack>
```

2. Build the program (replace `<board_name>` and `<program_path>` with the board name and path to the program):

```
west build -b <board_name> <program_path>
```

3. Flash the program:

```
west flash
```
