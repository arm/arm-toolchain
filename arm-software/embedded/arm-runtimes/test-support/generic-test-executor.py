#!/usr/bin/env python3

#
# Copyright (c) 2022-2025, Arm Limited and affiliates.
#
# Part of the Arm Toolchain project, under the Apache License v2.0 with LLVM Exceptions.
# See https://llvm.org/LICENSE.txt for license information.
# SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
#

# This is a generic executor script to execute tests with the supported models, such as QEMU or FVPs.

from run_qemu import run_qemu
from run_fvp import run_fvp
import argparse
import os
import pathlib
import sys


def run(args):
    # Some tests expect argv[0] to be literally "program-name", not
    # the actual program name.
    argv = ["program-name"] + args.arguments

    if args.qemu_command:
        return run_qemu(
            args.qemu_command,
            args.qemu_machine,
            args.qemu_cpu,
            args.qemu_params.split(":") if args.qemu_params else [],
            args.image,
            argv,
            None,
            pathlib.Path.cwd(),
            args.verbose,
        )
    else:
        return run_fvp(
            args.fvp_install_dir,
            args.fvp_config_dir,
            args.fvp_model,
            args.fvp_config,
            args.image,
            argv,
            None,
            pathlib.Path.cwd(),
            args.verbose,
            args.tarmac,
        )


def main():
    parser = argparse.ArgumentParser(
        description="Run a single test using either qemu or an FVP"
    )
    # Top-level required argument
    parser.add_argument(
        "--test_executor",
        required=True,
        choices=["fvp", "qemu"],
        help="Executor type: fvp or qemu",
    )

    # Common arguments
    parser.add_argument("--image", help="image file to execute")
    parser.add_argument(
        "--verbose",
        action="store_true",
        help="Print verbose output. This may affect test result, as the output "
        "will be added to the output of the test.",
    )

    # FVP arguments
    parser.add_argument(
        "--fvp_install_dir", help="Directory in which FVP models are installed"
    )
    parser.add_argument(
        "--fvp_config_dir", help="Directory in which FVP models are installed"
    )
    parser.add_argument(
        "--fvp_model",
        help="model name for FVP",
    )
    parser.add_argument(
        "--fvp_config",
        action="append",
        help="FVP config file(s) to use",
    )
    parser.add_argument(
        "--tarmac",
        help="file to wrote tarmac trace to (FVP only)",
    )

    # QEMU arguments
    parser.add_argument(
        "--qemu_machine",
        help="name of the machine to pass to QEMU",
    )
    parser.add_argument(
        "--qemu_command",
        help="name of the qemu binary used to execute. Binary must be either present in PATH or pass along with absolute path.",
    )
    parser.add_argument(
        "--qemu_cpu", required=False, help="name of the cpu to pass to QEMU"
    )
    parser.add_argument(
        "--qemu_params",
        type=str,
        help='list of arguments to pass to qemu, separated with ":"',
    )
    parser.add_argument(
        "arguments",
        nargs=argparse.REMAINDER,
        default=[],
        help="optional arguments for the image",
    )
    args = parser.parse_args()
    print("args: " + str(args))

    # Conditional validation
    if args.test_executor == "fvp":
        required = [
            "fvp_install_dir",
            "fvp_config_dir",
            "fvp_model",
            "fvp_config",
            "image",
        ]
    elif args.test_executor == "qemu":
        required = ["qemu_machine", "qemu_command", "qemu_cpu", "image"]

    # Check missing required args
    missing = [arg for arg in required if getattr(args, arg) is None]
    if missing:
        parser.error(
            f"For test_executor='{args.test_executor}', the following arguments are required: {', '.join('--' + m for m in missing)}"
        )

    ret_code = run(args)
    sys.exit(ret_code)


if __name__ == "__main__":
    main()
