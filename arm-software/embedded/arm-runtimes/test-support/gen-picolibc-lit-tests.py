#!/usr/bin/env python3

# SPDX-FileCopyrightText: Copyright 2025 Arm Limited and/or its affiliates <open-source-office@arm.com>

"""Helper script to generate a lit test suite from a meson project's tests.

The lit testing infratructure has a number of features that it would
be useful to apply to meson test suite, such as filtering tests,
setting particular tests to be expected to fail, and controlling
whether or not testing should continue on a failure.
Since meson can run tests individually and provide a list of all
tests, it is possible to run the same tests through lit, with each
individual test simply invoking meson.
This allows a project built using meson to share the same test
infrastructure as the other LLVM projects."""

import argparse
import os
import subprocess


def main():
    arg_parser = argparse.ArgumentParser(
        prog="meson_to_lit",
        description="A script that generates a set of lit tests for a meson test suite.",
    )
    arg_parser.add_argument(
        "--meson",
        required=True,
        help="Path to meson.",
    )
    arg_parser.add_argument(
        "--build",
        required=True,
        help="Path to meson build directory.",
    )
    arg_parser.add_argument(
        "--output",
        required=True,
        help="Path to write tests to.",
    )
    arg_parser.add_argument(
        "--name",
        required=True,
        help="Name to give the lit suite.",
    )
    arg_parser.add_argument(
        "--timeout-multiplier", type=float, help="Timeout multiplier (float)."
    )

    args = arg_parser.parse_args()

    # Ensure the output location exists.
    os.makedirs(os.path.join(args.output, "tests"), exist_ok=True)

    # Get the test list from meson.
    p = subprocess.run(
        [args.meson, "test", "--list"],
        cwd=args.build,
        capture_output=True,
        check=True,
        text=True,
    )

    for line in p.stdout.splitlines():
        # Meson lists tests in the format of
        # [SUBPROJECT]:[PATH] / [TESTNAME]
        # e.g. picolibc:semihost / semihost-argv
        # The testnames should be unique, so only the name is needed.
        subproj, full_testname = line.split(":", maxsplit=1)
        testname = full_testname.split(" / ")[-1]
        with open(
            os.path.join(args.output, "tests", testname + ".test"),
            "w",
            encoding="utf-8",
        ) as f:
            # Invoke meson to run the test.
            # Set --logbase so that each has a unique log name.
            cmd = f"# RUN: {args.meson} test -C {args.build} {testname} --logbase {testname} --no-rebuild"
            if args.timeout_multiplier and args.timeout_multiplier != 1:
                cmd += f" -t {args.timeout_multiplier}"
            f.write(f"{cmd}\n")

    # Simple lit config to run the tests.
    cfg_txt = """import lit.formats
import lit.llvm
import os

lit.llvm.initialize(lit_config, config)

config.name = "%CONFIG_NAME%"
config.suffixes = [".test"]
config.test_format = lit.formats.ShTest(not lit.llvm.llvm_config.use_lit_shell)
config.test_source_root = os.path.join(os.path.dirname(__file__), "tests")
"""
    with open(os.path.join(args.output, "lit.cfg.py"), "w", encoding="utf-8") as f:
        f.write(cfg_txt.replace("%CONFIG_NAME%", args.name))


if __name__ == "__main__":
    main()
