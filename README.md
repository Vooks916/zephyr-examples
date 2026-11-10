# zephyr-examples

This repository contains sample Zephyr applications that run on the Curiosity Nano development boards. Each application demonstrates the use of a specific peripheral on the Curiosity Nano Explorer.

## Building and Flashing

### Supported Board Information

|Board|Board Name|PyOCD Pack|
|---|---|---|
|SAM D21 Curiosity Nano|samd21_cnano|atsamd21g17d|
|SAM E51 Curiosity Nano|same51_cnano|atsame51j20a|

### Instructions

1. Install the PyOCD pack:

```
pyocd pack install <pyocd_pack>
```

2. Build the application:

```
west build -b <board_name> <program_path>
```

3. Flash the application:

```
west flash
```
