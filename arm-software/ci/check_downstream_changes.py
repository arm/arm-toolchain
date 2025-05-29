#!/usr/bin/env python3

# Copyright (c) 2025, Arm Limited and affiliates.
# Part of the Arm Toolchain project, under the Apache License v2.0 with LLVM Exceptions.
# See https://llvm.org/LICENSE.txt for license information.
# SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception

"""
A script to check that a pull request adheres to the downstream patch policy.
If the pull request modifies file outside the arm-software build directory
(or any other files excluded from automerge) then the pull request needs to
contain specific text to link to a downstream tracking issue.

Requires the GitHub CLI tool (gh) to query the repo.
"""

import argparse
import json
import logging
import os
import re
import shlex
import subprocess
import sys
from pathlib import Path


logging.basicConfig(level=logging.INFO, format="%(levelname)s: %(message)s")
logger = logging.getLogger(__name__)

MERGE_IGNORE_PATHSPEC_FILE = Path(__file__).parent / ".automerge_ignore"


# Check gh is working before using it
def check_gh_status() -> None:
    args = [
        "gh",
        "auth",
        "status",
    ]
    logger.debug(f"Running `{shlex.join(args)}`")
    try:
        p = subprocess.run(
            args,
            check=True,
            capture_output=True,
            text=True,
        )
    except subprocess.CalledProcessError as error:
        logger.error(
            f"Check error. Failure checking gh\ncmd:{shlex.join(error.cmd)}\ncode:{error.returncode}\nstdout:{error.stdout}\nstderr:{error.stderr}"
        )
        sys.exit(1)


# Use gh to get information about the pull request.
def get_pr_json(pr_num: str, repo: str) -> dict:
    args = ["gh", "pr", "view", pr_num, "--repo", repo, "--json", "body,files,title"]
    logger.debug(f"Running `{shlex.join(args)}`")
    try:
        p = subprocess.run(
            args,
            check=True,
            capture_output=True,
            text=True,
        )
    except subprocess.CalledProcessError as error:
        logger.error(
            f"Check error. Failure querying pull request\ncmd:{shlex.join(error.cmd)}\ncode:{error.returncode}\nstdout:{error.stdout}\nstderr:{error.stderr}"
        )
        sys.exit(1)
    j = json.loads(p.stdout)
    logger.debug(
        f"Response from server for pull request #{pr_num}:\n{json.dumps(j, indent=4)}"
    )
    return j


# Check that a value matches a valid issue.
def is_valid_issue_num(issue_num: str, repo: str) -> bool:
    args = [
        "gh",
        "issue",
        "list",
        "--search",
        issue_num,
        "--repo",
        repo,
        "--state",
        "all",
        "--json",
        "id",
    ]
    logger.debug(f"Running `{shlex.join(args)}`")
    try:
        p = subprocess.run(
            args,
            check=True,
            capture_output=True,
            text=True,
        )
    except subprocess.CalledProcessError as error:
        logger.error(
            f"Check error. Failure querying issue\ncmd:{shlex.join(error.cmd)}\ncode:{error.returncode}\nstdout:{error.stdout}\nstderr:{error.stderr}"
        )
        sys.exit(1)
    j = json.loads(p.stdout)
    logger.debug(
        f"Response from server for issue {issue_num}:\n{json.dumps(j, indent=4)}"
    )
    if len(j) > 0:
        logger.info(f"Issue found matching number #{issue_num}")
        return True
    else:
        logger.info(f"No issue found matching number #{issue_num}")
        return False


# Test if a path is in the ignore list.
def is_path_ignored(test_path: str, ignored_paths: list[str]) -> bool:
    for ignored_path in ignored_paths:
        # The ignore list contains paths or directories.
        # Anything in an ignored subdirectory should also be ignored.
        if os.path.commonpath([ignored_path, test_path]) == ignored_path:
            logger.debug(f"{test_path} ignored by line {ignored_path}")
            return True
    return False


# Test if a pull request contains a downstream change
def has_downstream_changes(input_json: dict) -> bool:
    excluded_files = []
    included_files = []
    with open(MERGE_IGNORE_PATHSPEC_FILE, "r") as f:
        ignored_paths = f.read().splitlines()
    for file in input_json["files"]:
        changed_file = file["path"]
        if is_path_ignored(changed_file, ignored_paths):
            excluded_files.append(changed_file)
        else:
            included_files.append(changed_file)
    if len(excluded_files) > 0:
        excluded_list = "\n".join(excluded_files)
        logger.info(f"File modifications excluded by ignore list:\n{excluded_list}")
    if len(included_files) > 0:
        included_list = "\n".join(included_files)
        logger.info(f"File modifications that require tracking:\n{included_list}")
    else:
        logger.info("No modifications to files outside exclude list found.")
    return len(included_files) > 0


# Check if a pull request has been correctly tagged.
# Expected formatting per the policy (with leeway for spaces):
# Downstream issue:#123
# Downstream issue: #123
# Removes downstream issue:#123
# Removes downstream issue: #123
def find_pr_issue(input_json: dict) -> str:
    logger.debug("body text: %s", input_json["body"])
    matches = re.findall(
        "^((?:removes )?downstream issue: *#([0-9]+))",
        input_json["body"],
        flags=re.I | re.M,
    )
    if len(matches) == 0:
        logger.info("No downstream issue link found in pull request body.")
        return None
    tag_list = "\n".join([result[0] for result in matches])
    logger.info(f"Found issue links:\n{tag_list}")
    # There should only be one match.
    if len(matches) > 1:
        logger.info(
            "Multiple downstream issue links found in pull request body. Only one is expected."
        )
        return None
    issue_num = matches[0][1]

    logger.info(f"Pull request text links to issue #{issue_num}")
    return issue_num


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument(
        "--repo",
        required=True,
        help="GitHub repo where the pull request can be found",
    )
    parser.add_argument(
        "--pr",
        required=True,
        help="The number of the pull request to be checked",
    )
    parser.add_argument(
        "--verbose",
        action="store_true",
        help="Print verbose log messages",
    )

    args = parser.parse_args()

    if args.verbose:
        logger.setLevel(logging.DEBUG)

    check_gh_status()

    pr_json = get_pr_json(args.pr, args.repo)
    pr_title = pr_json["title"]
    logger.info(f"Checking pull request #{args.pr}: '{pr_title}'")
    needs_tagging = has_downstream_changes(pr_json)
    issue_num = find_pr_issue(pr_json)

    link_text = "Please check https://github.com/arm/arm-toolchain/blob/arm-software/CONTRIBUTING.md#downstream-patch-policy for information on the downstream patch policy and how changes need to be tracked."
    if needs_tagging:
        if issue_num is None:
            logger.info(
                f"Check failed. Pull request #{args.pr} contains downstream changes, but does not have a correctly formatted link to a downstream tracking issue. {link_text}"
            )
            sys.exit(1)
        else:
            if not is_valid_issue_num(issue_num, args.repo):
                logger.info(
                    f"Check failed. Pull request #{args.pr} contains downstream changes, but the link to the downstream tracking issue is not valid. {link_text}"
                )
                sys.exit(1)
            else:
                logger.info(
                    f"Check passed. Pull request #{args.pr} contains downstream changes, and a correctly formatted link to a downstream tracking issue."
                )
                sys.exit(0)
    else:
        if issue_num is None:
            logger.info(
                f"Check passed. Pull request #{args.pr} contains no downstream changes, and does not link to a downstream tracking issue."
            )
            sys.exit(0)
        else:
            logger.info(
                f"Check failed. Pull request #{args.pr} contains no downstream changes, but links to a downstream tracking issue. {link_text}"
            )
            sys.exit(1)


if __name__ == "__main__":
    main()
