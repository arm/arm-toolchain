#!/usr/bin/env python3

# SPDX-FileCopyrightText: Copyright 2024-2025 Arm Limited and/or its affiliates <open-source-office@arm.com>

# This is a wrapper script for LLVM's lit, which takes additional options in
# order to prepare a specific environment for running tests.
# The new --xfails-file and --xfails-not-file options are functionally similar
# to the existing --xfails and --xfails-not options, but allow for a test
# list to be loaded from a file instead of on the command line. This is
# preferable when there are large number of tests that need to be specified.

import argparse
import os
import runpy
import sys

parser = argparse.ArgumentParser()
parser.add_argument("--xfails-file")
parser.add_argument("--xfails-not-file")
parser.add_argument("--real-lit-path", required=True)
# Only parse the known args above. The rest should be processed the actual lit.
known_args, unknown_args = parser.parse_known_args()

# If an xfails file is set, read it.
if known_args.xfails_file:
    if os.path.isfile(known_args.xfails_file):
        with open(known_args.xfails_file, "r") as f:
            xfail_lines = f.readlines()
        lit_xfails_list = ";".join([line.strip() for line in xfail_lines])
        # Load the contents of the file into LIT_XFAIL
        if "LIT_XFAIL" in os.environ:
            os.environ["LIT_XFAIL"] = os.environ["LIT_XFAIL"] + ";" + lit_xfails_list
        else:
            os.environ["LIT_XFAIL"] = lit_xfails_list
    else:
        raise FileNotFoundError(
            f"Error: Expected file '{known_args.xfails_file}' does not exist."
        )

# If an xfails not file is set, read it.
if known_args.xfails_not_file:
    if os.path.isfile(known_args.xfails_not_file):
        with open(known_args.xfails_not_file, "r") as f:
            xfail_lines = f.readlines()
        lit_xfails_list = ";".join([line.strip() for line in xfail_lines])
        # Load the contents of the file into LIT_XFAIL_NOT
        if "LIT_XFAIL_NOT" in os.environ:
            os.environ["LIT_XFAIL_NOT"] = (
                os.environ["LIT_XFAIL_NOT"] + ";" + lit_xfails_list
            )
        else:
            os.environ["LIT_XFAIL_NOT"] = lit_xfails_list
    else:
        print("file missing?")
        raise FileNotFoundError(
            f"Error: Expected file '{known_args.xfails_not_file}' does not exist."
        )

# Remove expected custom args from the arg list.
new_argv = [sys.argv[0]] + unknown_args
sys.argv = new_argv

# Run lit with the new arg list.
runpy.run_path(known_args.real_lit_path, run_name="__main__")
