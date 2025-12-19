#!/bin/bash

# Copyright (c) 2025, Arm Limited and affiliates.
# Part of the Arm Toolchain project, under the Apache License v2.0 with LLVM Exceptions.
# See https://llvm.org/LICENSE.txt for license information.
# SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception

# A bash script for extracting the contents of a MacOS .dmg image.

# The script takes the following arguments:
# extract_dmg.sh <path_to_dmg> [<glob_pattern_to_extract>] [<dst_dir>]
# * path_to_dmg: The path to the .dmg image to extract contents from
# * glob_pattern_to_extract (optional): A glob pattern to match the contents
#   to be extracted. The default is "*".
# * dst_dir (optional): The directory into which the contents should be
#   extracted. The default is the current working dir.

if [[ -z "${1}" ]]; then
    >&2 echo "ERROR: Path to .dmg file not provided."
    echo "usage: ${0} <path_to_dmg> [<glob_pattern_to_extract>] [<dst_dir>]"
    exit 1
fi

DMG_PATH="${1}"
PATTERN_TO_EXTRACT="${2:-"*"}"
DST_DIR="${3:-"."}"

if command -v hdiutil > /dev/null; then
    ATTACH_OUTPUT=$( hdiutil attach -nobrowse -readonly "${DMG_PATH}" )
    # The output of `hdiutil attach` includes a description of the mounted
    # volumes that correspond to the specified .dmg file, where one of them
    # maps to a path in `/Volumes/`. E.g.:
    # /dev/disk4     GUID_partition_scheme
    # /dev/disk4s1   EFI
    # /dev/disk4s2   Apple_HFS              /Volumes/ATfE-21.0.0-pre-Darwin-universal
    MOUNT_PATH=$( echo "${ATTACH_OUTPUT}" | grep '/Volumes/' | sed -r 's!.*(/Volumes/.*)!\1!' )
    if [[ -z "${MOUNT_PATH}" ]]; then
        >&2 echo "ERROR: Path to mounted volume not found."
        exit 1
    fi
    cp -R "${MOUNT_PATH}"/${PATTERN_TO_EXTRACT} .
    EXITCODE=$?
    hdiutil detach "${MOUNT_PATH}" && exit $EXITCODE
else
    >&2 echo "ERROR: No application capable of extracting .dmg contents found."
    exit 1
fi
