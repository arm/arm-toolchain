#!/usr/bin/env python3

# SPDX-FileCopyrightText: Copyright 2024-2025 Arm Limited and/or its affiliates <open-source-office@arm.com>

"""This is a wrapper script for LLVM's lit, which takes additional options in
order to prepare a specific environment for running tests.

The new --xfails-file and --xfails-not-file options are functionally similar
to the existing --xfails and --xfails-not options, but allow for a test
list to be loaded from a file instead of on the command line. This is
preferable when there are large number of tests that need to be specified."""

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


def load_env_var(env_var_name, input_filename):
    """Load the contents of a file into an environment variable."""
    if input_filename:
        if os.path.isfile(input_filename):
            with open(input_filename, "r", encoding="utf-8") as f:
                xfail_lines = f.readlines()
            lit_xfails_list = ";".join([line.strip() for line in xfail_lines])
            # Load the contents of the file into variables
            if env_var_name in os.environ:
                os.environ[env_var_name] = (
                    os.environ[env_var_name] + ";" + lit_xfails_list
                )
            else:
                os.environ[env_var_name] = lit_xfails_list
        else:
            raise FileNotFoundError(
                f"Error: Expected file '{input_filename}' does not exist."
            )


load_env_var("LIT_XFAIL", known_args.xfails_file)
load_env_var("LIT_XFAIL_NOT", known_args.xfails_not_file)

# Remove expected custom args from the arg list.
new_argv = [sys.argv[0]] + unknown_args
sys.argv = new_argv

# Run lit with the new arg list.
runpy.run_path(known_args.real_lit_path, run_name="__main__")
