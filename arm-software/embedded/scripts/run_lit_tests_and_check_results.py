#!/usr/bin/env python3

# Copyright (c) 2026, Arm Limited and affiliates.
# Part of the Arm Toolchain project, under the Apache License v2.0 with LLVM Exceptions.
# See https://llvm.org/LICENSE.txt for license information.
# SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception

"""Run configured Ninja lit targets and check their JUnit results."""

import argparse
import os
import re
import shlex
import subprocess
import sys
from dataclasses import dataclass
from pathlib import Path
from typing import List, Set


@dataclass(frozen=True)
class CheckTarget:
    ninja_build_dir: Path
    name: str
    lit_result_prefix: str


def ninja_targets(build_dir: Path) -> Set[str]:
    result = subprocess.run(
        ["ninja", "-C", str(build_dir), "-t", "targets"],
        check=True,
        stdout=subprocess.PIPE,
        text=True,
    )
    # ninja outputs strings like:
    # check-llvm-toolchain-lit: phony
    return {line.split(":", 1)[0] for line in result.stdout.splitlines()}


def discover_check_targets(build_dir: Path) -> List[CheckTarget]:
    targets = []
    if "check-all" in ninja_targets(build_dir):
        targets.append(CheckTarget(build_dir, "check-all", "check-all"))

    # The top-level build contains convenience aliases for every variant in
    # the multilib definition, including variants that were not configured.
    # Each multilib sub-build exposes only check targets that are enabled for
    # its configured variants, so discover the runnable targets there.
    for multilib_root in sorted(build_dir.glob("multilib-*-builds")):
        build_file = multilib_root / "multilib" / "build" / "build.ninja"
        if not build_file.is_file():
            continue
        multilib_build_dir = build_file.parent
        multilib_name = multilib_root.name
        if multilib_name.endswith("-builds"):
            multilib_name = multilib_name[: -len("-builds")]
        for target in sorted(ninja_targets(multilib_build_dir)):
            if target.startswith("check-") and target != "check-all":
                targets.append(CheckTarget(multilib_build_dir, target, multilib_name))

    if not targets:
        raise RuntimeError(f"no lit check targets found in {build_dir}")
    return targets


def specified_check_targets(
    discovered_targets: List[CheckTarget], target_names: List[str]
) -> List[CheckTarget]:
    targets = []
    missing_targets = []
    for name in target_names:
        matching_targets = [
            target for target in discovered_targets if target.name == name
        ]
        if matching_targets:
            targets.extend(matching_targets)
        else:
            missing_targets.append(name)

    if missing_targets:
        raise RuntimeError(
            "Ninja check targets not found: " + ", ".join(missing_targets)
        )
    return targets


def target_label(target: CheckTarget) -> str:
    if target.lit_result_prefix == target.name:
        return target.name
    return f"{target.name} for {target.lit_result_prefix}"


def result_file_name(target: CheckTarget) -> str:
    prefix = (
        ""
        if target.lit_result_prefix == target.name
        else f"{target.lit_result_prefix}_"
    )
    name = re.sub(r"[^A-Za-z0-9_.-]", "_", target.name)
    return f"{prefix}{name}_lit_results.junit.xml"


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("results_dir", type=Path, help="Directory for JUnit results")
    parser.add_argument(
        "--lit-opts",
        default="",
        help="Options to pass to lit in addition to the JUnit output file",
    )
    parser.add_argument(
        "--check-targets",
        nargs="+",
        help=(
            "Optional Ninja check targets to run. "
            "Targets will be discovered if not specified."
        ),
    )
    parser.add_argument(
        "--show-discovered",
        action="store_true",
        help="Show all discovered Ninja check targets and exit",
    )
    parser.add_argument(
        "--dry-run",
        action="store_true",
        help="Show commands without running tests or modifying results",
    )
    args = parser.parse_args()

    build_dir = Path.cwd().resolve()
    results_dir = args.results_dir.resolve()

    try:
        discovered_targets = discover_check_targets(build_dir)
        if args.show_discovered:
            print(f"Discovered {len(discovered_targets)} Ninja check targets:")
            for target in discovered_targets:
                print(f"{target_label(target)} in {target.ninja_build_dir}")
            return 0

        if args.check_targets:
            targets = specified_check_targets(discovered_targets, args.check_targets)
        else:
            targets = discovered_targets
    except (OSError, RuntimeError, subprocess.CalledProcessError) as error:
        print(
            f"error: could not discover Ninja check targets: {error}", file=sys.stderr
        )
        return 1

    if not args.dry_run:
        results_dir.mkdir(parents=True, exist_ok=True)
        for stale_result in results_dir.glob("*_lit_results.junit.xml"):
            stale_result.unlink()

    checker = Path(__file__).resolve().with_name("fail_on_test_failures.py")
    failed_targets = []
    for target in targets:
        result_name = result_file_name(target)
        result_file = results_dir / result_name
        if not args.dry_run:
            result_file.unlink(missing_ok=True)

        environment = os.environ.copy()
        output_option = f"--xunit-xml-output={shlex.quote(str(result_file))}"
        environment["LIT_OPTS"] = " ".join(
            option for option in (args.lit_opts, output_option) if option
        )
        ninja_command = ["ninja", "-C", str(target.ninja_build_dir), target.name]
        checker_command = [
            sys.executable,
            str(checker),
            str(results_dir),
            result_name,
        ]

        if args.dry_run:
            print(
                f"LIT_OPTS={shlex.quote(environment['LIT_OPTS'])} "
                + shlex.join(ninja_command)
            )
            print(shlex.join(checker_command))
            continue

        print(
            f"Running {target.name}; results will be written to {result_file}",
            flush=True,
        )
        ninja_result = subprocess.run(
            ninja_command,
            env=environment,
            check=False,
        )
        check_result = subprocess.run(
            checker_command,
            check=False,
        )
        if ninja_result.returncode or check_result.returncode:
            failed_targets.append(target_label(target))

    if args.dry_run:
        print(f"Dry run: would run and check {len(targets)} targets")
        return 0

    if failed_targets:
        print(
            f"error: {len(failed_targets)} of {len(targets)} check targets failed: "
            + ", ".join(failed_targets),
            file=sys.stderr,
        )
        return 1

    print(f"All {len(targets)} check targets passed")
    return 0


if __name__ == "__main__":
    sys.exit(main())
