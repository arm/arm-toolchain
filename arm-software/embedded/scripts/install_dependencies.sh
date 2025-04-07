#!/bin/bash
set -e

# This script installs the essential build dependencies for ATfE.

sudo apt-get update && sudo apt-get install -y --no-install-recommends \
    cmake \
    ninja-build \
    clang
