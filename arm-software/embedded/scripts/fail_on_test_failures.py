#!/usr/bin/env python3

# Copyright (c) 2026, Arm Limited and affiliates.
# Part of the Arm Toolchain project, under the Apache License v2.0 with LLVM Exceptions.
# See https://llvm.org/LICENSE.txt for license information.
# SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception

"""Fail if any lit JUnit report contains test failures or errors."""

import argparse
import sys
from pathlib import Path
from xml.etree import ElementTree


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("directory", type=Path, help="Directory containing test reports")
    args = parser.parse_args()

    result_files = sorted(args.directory.rglob("lit_results.junit.xml"))
    if not result_files:
        print(f"error: no lit JUnit results found below {args.directory}", file=sys.stderr)
        return 1

    failed_files = []
    failures = 0
    errors = 0
    for result_file in result_files:
        file_failures = 0
        file_errors = 0
        try:
            # JUnit reports can be large. Stream the XML so it is still
            # validated without retaining the complete document in memory.
            for _, element in ElementTree.iterparse(result_file, events=("end",)):
                tag = element.tag.rsplit("}", 1)[-1]
                if tag == "failure":
                    file_failures += 1
                elif tag == "error":
                    file_errors += 1
                element.clear()
        except (ElementTree.ParseError, OSError) as error:
            print(f"error: could not read {result_file}: {error}", file=sys.stderr)
            return 1

        failures += file_failures
        errors += file_errors
        if file_failures or file_errors:
            failed_files.append((result_file, file_failures, file_errors))

    print(
        f"Checked {len(result_files)} lit JUnit result files: "
        f"{failures} failures, {errors} errors"
    )
    for result_file, file_failures, file_errors in failed_files:
        print(
            f"error: {result_file}: {file_failures} failures, {file_errors} errors",
            file=sys.stderr,
        )

    return 1 if failed_files else 0


if __name__ == "__main__":
    sys.exit(main())
