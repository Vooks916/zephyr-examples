# zephyr-examples

This repository contains example Zephyr applications that run on the Curiosity Nano development boards. Each application demonstrates the use of a specific peripheral on the Curiosity Nano Explorer.

## Required Hardware

* SAM D21 Curiosity Nano (SAMD21G17D) 
* SAM E51 Curiosity Nano (SAME51J20A)
* Curiosity Nano Explorer development board

## Setup

### 1. Install Zephyr

It is assumed Zephyr has been installed according to the [Getting Started Guide](https://docs.zephyrproject.org/latest/develop/getting_started/index.html).

Ensure that Python 3.12, West, the Zephyr SDK, and the ARM toolchain are installed before continuing.

Replace `~/zephyrproject` in the following commands with the path to the workspace.

### 2. Activate Python’s Virtual Environment:

To run commands like `west`, you need to activate the virtual environment every time you start a new terminal session before working with Zephyr. Once activated, your shell will be prefixed with `(.venv)`.

**Linux/macOS:**
```
cd ~/zephyrproject
source ~/zephyrproject/.venv/bin/activate
```

**Windows (Powershell):**
```
cd ~/zephyrproject
.\.venv\Scripts\Activate.ps1
```

### 3. Add a Remote for Our Zephyr Fork:

```
cd ~/zephyrproject/zephyr
git remote add capstone https://github.com/Vooks916/zephyr
```

### 4. Fetch and Checkout the Demo Branch of Our Fork:

In this branch, we have added the board support needed to run the example applications.

```
git fetch capstone demo
git checkout capstone/demo
```

### 5. Update the hal_atmel Module:

This updates the HAL module that has additional hardware support.

```
west update hal_atmel
```

### 6. Clone the Example Applications Repo

```
cd ~/zephyrproject
git clone https://github.com/Vooks916/zephyr-examples.git
```

### 7. Install PyOCD

```
python -m pip install -U pyocd
```

### 8. Install the PyOCD Pack:

Install the device pack for each development board.

**SAM D21 Curiosity Nano:**
```
pyocd pack install atsamd21g17d
```

**SAM E51 Curiosity Nano:**
```
pyocd pack install atsame51j20a
```

## Supported Board Information

|Board|Board Name|PyOCD Pack|
|---|---|---|
|SAM D21 Curiosity Nano|samd21_cnano|atsamd21g17d|
|SAM E51 Curiosity Nano|same51_cnano|atsame51j20a|

## Build and Flash Instructions

### 1. Setup Hardware

Connect the Curiosity Nano development board to the Curiosity Nano Explorer development board.

Connect a USB cable between the debug USB on the board and the pc.

### 2. Build the Application:

In the examples repo, build the specified example application.

```
cd ~/zephyrproject/zephyr-examples
west build -p always -b <board_name> <program_path>
```

### 3. Flash the Application:

```
west flash
```
