#!/usr/bin/env bash

# Copyright (c) 2025, Arm Limited and affiliates.
# Part of the Arm Toolchain project, under the Apache License v2.0 with LLVM Exceptions.
# See https://llvm.org/LICENSE.txt for license information.
# SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
#
# This script installs the essential build dependencies for ATfE on macOS.

set -euo pipefail

# Ensure the script is running on macOS
if [[ "$(uname)" != "Darwin" ]]; then
    echo "This install script is intended for macOS runners only." >&2
    exit 1
fi

# Check for Homebrew installation
if ! command -v brew >/dev/null 2>&1; then
    echo "Homebrew is required but was not found on this runner." >&2
    exit 1
fi

# Install required Homebrew packages if not already installed
REQUIRED_FORMULAE=(
    ccache
    ninja
    qemu
)
MISSING_FORMULAE=()
for formula in "${REQUIRED_FORMULAE[@]}"; do
    if ! brew list --formula "${formula}" >/dev/null 2>&1; then
        MISSING_FORMULAE+=("${formula}")
    fi
done

if [[ "${#MISSING_FORMULAE[@]}" -gt 0 ]]; then
    echo "Installing missing Homebrew packages: ${MISSING_FORMULAE[*]}"
    brew install --formula "${MISSING_FORMULAE[@]}"
else
    echo "Required Homebrew packages already installed."
fi

# Set up a Python virtual environment for ATfE tools
VENV_DIR="${HOME}/.atfe-venv"
python3 -m venv "${VENV_DIR}"
source "${VENV_DIR}/bin/activate"

# Export PATH
export PATH="${VENV_DIR}/bin:${PATH}"
if [[ -n "${GITHUB_PATH:-}" ]]; then
    echo "${VENV_DIR}/bin" >> "${GITHUB_PATH}"
fi

# Upgrade pip and install Python tooling inside the virtual environment
python -m pip install --upgrade pip
python -m pip install cmake==4.3.0 meson==1.2.3 psutil==7.2.2 pyyaml==6.0.3 ruff==0.8.6

cmake --version
