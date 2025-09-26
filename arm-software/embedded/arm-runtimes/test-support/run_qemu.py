#!/usr/bin/env python3

# Copyright (c) 2023, Arm Limited and affiliates.

import subprocess
import sys
import re


def get_qemu_major_version(qemu_command):
    output = subprocess.check_output([qemu_command, "--version"], text=True)
    version_match = re.search(r"version (\d+)\.", output)
    if version_match:
        return int(version_match.group(1))
    else:
        raise Exception("Cannot get version of " + qemu_command)


def run_qemu(
    qemu_command,
    qemu_machine,
    qemu_cpu,
    qemu_extra_params,
    image,
    arguments,
    timeout,
    working_directory,
    verbose,
    trace,
):
    """Execute the program using QEMU and return the subprocess return code."""
    qemu_params = ["-M", qemu_machine]
    if qemu_cpu:
        qemu_params += ["-cpu", qemu_cpu]
    qemu_params += qemu_extra_params

    # Setup semihosting with chardev bound to stdio.
    # This is needed to test semihosting functionality in picolibc.
    qemu_params += ["-chardev", "stdio,mux=on,id=stdio0"]
    semihosting_config = ["enable=on", "chardev=stdio0"] + [
        "arg=" + arg.replace(",", ",,") for arg in arguments
    ]
    qemu_params += ["-semihosting-config", ",".join(semihosting_config)]

    # Disable features we don't need and which could slow down the test or
    # interfere with semihosting.
    qemu_params += ["-monitor", "none", "-serial", "none", "-nographic"]

    # Load the image to machine's memory and set the PC.
    # "virt" machine cannot be used with load, as QEMU will try to put
    # device tree blob at start of RAM conflicting with our code
    # https://www.qemu.org/docs/master/system/arm/virt.html#hardware-configuration-information-for-bare-metal-programming
    if qemu_machine == "virt":
        qemu_params += ["-kernel", image]
    else:
        qemu_params += ["-device", f"loader,file={image},cpu-num=0"]

    # Enable tracing: disassembly, CPU state, interrupts and guest errors like
    # invalid instructions.
    if trace:
        qemu_params += ["-d", "in_asm,nochain,cpu,int,guest_errors"]
        qemu_params += ["-D", trace]
        # Enable per instruction tracing depending on EQMU version
        if get_qemu_major_version(qemu_command) >= 9:
            qemu_params += ["-accel", "tcg,one-insn-per-tb=on"]
        else:
            qemu_params += ["-singlestep"]

    command = [qemu_command] + qemu_params

    if verbose:
        print("running: {}".format(" ".join(command)))

    result = subprocess.run(
        command,
        stdout=subprocess.PIPE,
        stderr=sys.stderr,
        timeout=timeout,
        cwd=working_directory,
        check=False,
    )
    sys.stdout.buffer.write(result.stdout)
    return result.returncode
