# Bare-metal semihosting sample

This sample shows how to use semihosting with
[QEMU Arm System emulator](https://www.qemu.org/docs/master/system/target-arm.html)
targeting the
[micro:bit board model](https://www.qemu.org/2019/05/22/microbit/)
to build C++ programs with `newlib`.

A recent version of QEMU is required to run the sample, because of the
dependency on details of semihosting implementation in QEMU.
