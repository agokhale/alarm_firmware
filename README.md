# Alarm board firmware ![](https://github.com/cibomahto/alarm-module/workflows/.github/workflows/main.yml/badge.svg)

Firmware for the [Ventilator alarm module](https://github.com/ventilator-project/alarm_module)

# Toolchain Setup

From a bare Ubuntu install:

    sudo apt update
    sudo apt upgrade
    sudo apt install build-essential git

Optional tools:

    sudo apt install vim dos2unix

For serial debuggets, add the user to dialout:

    sudo usermod -a -G dialout matt

## Getting a fixed compiler version

Using the '9-2019-q4-major' version from:
https://developer.arm.com/tools-and-software/open-source-software/developer-tools/gnu-toolchain/gnu-rm/downloads

    cd ~
    wget https://armkeil.blob.core.windows.net/developer/Files/downloads/gnu-rm/9-2019q4/gcc-arm-none-eabi-9-2019-q4-major-x86_64-linux.tar.bz2
    tar -xf gcc-arm-none-eabi-9-2019-q4-major-x86_64-linux.tar.bz2
    export PATH=~/gcc-arm-none-eabi-9-2019-q4-major/bin:$PATH

## Set up and build the firmware

    git clone https://github.com/ventilator-project/alarm_firmware
    cd alarm_firmware/firmware
    make

## Load and debug with black magic device

Add yourself to the dialout group, then restart:

    sudo usermod -a -G dialout $1

To load the firmware:

    make flash-bm

For a debugging session, launch gdb:

    cd ~/alarm_firmware/firmware
    arm-none-eabi-gdb

And from gdb:

    target extended-remote /dev/ttyACM0
    monitor swdp_scan
    attach 1
    file build/alarm_firmware.elf
    load
    

# Project generation

The project was generated using STMCube (version ?):

    https://www.st.com/en/development-tools/stm32cubeprog.html

And uses the STM32 HAL variant


## Style guide TODO

Similar to WebKit: https://webkit.org/code-style-guidelines/

Except:

    use _ instead of CamelCase

Use Doxygen format for comments

## Static analysis TODO

Cppcheck is free and easy to set up:

    sudo apt install cppcheck

then run it on the project files:

    cppcheck --platform=unix32 --template=gcc --enable=warning,style main/ components/ --inconclusive > /dev/null

## Linting TODO

Use clang-format:

    sudo apt install clang-format

Then run:

    clang-format -i -style webkit main/*.h main/*.c

