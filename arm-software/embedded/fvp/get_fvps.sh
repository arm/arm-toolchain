#!/bin/bash

# SPDX-FileCopyrightText: Copyright 2024, 2026 Arm Limited and/or its affiliates <open-source-office@arm.com>

# This script downloads the FVP models which we use for testing the toolchain.
# These are available at no cost, but come with EULAs which must be agreed to
# before running them. When run with --non-interactive, this script assumes
# that you agree to these EULAs. If run without that option, the installers for
# some of the packages will present you with the license before installing. The
# AEMv8A and AEMv8R packages do not have installers, instead they place their
# license into the `fvp/install/license_terms' directory.

set -euxo pipefail

args=$(getopt --options "" --longoptions "non-interactive" -- "${@}") || exit
eval "set -- ${args}"

# Change into the directory containing this script. We'll make
# "download" and "install" subdirectories below that.
cd "$(dirname "$0")"

INSTALLER_FLAGS_CORSTONE=()
INSTALLER_FLAGS_CRYPTO=()

while true; do
    case "${1}" in
        (--non-interactive)
            INSTALLER_FLAGS_CORSTONE=(--i-agree-to-the-contained-eula --no-interactive --force)
            INSTALLER_FLAGS_CRYPTO=(--i-accept-the-end-user-license-agreement --basepath "$PWD/install")
            shift 1
        ;;
        (--)
            shift
            break
        (*)
            exit 1
        ;;
    esac
done

# Downloads are currently found on the following pages:
# Corstone:
# https://developer.arm.com/Tools%20and%20Software/Fixed%20Virtual%20Platforms/IoT%20FVPs
# AEMv8A and AEMv8R:
# https://developer.arm.com/Tools%20and%20Software/Fixed%20Virtual%20Platforms/Arm%20Architecture%20FVPs
# Crypto plug-in:
# https://developer.arm.com/Tools%20and%20Software/Fast%20Models#Downloads

MACHINE_HARDWARE=$(uname -m)

if [ "$MACHINE_HARDWARE" == 'x86_64' ]; then
    URL_CORSTONE_320='https://developer.arm.com/-/cdn-downloads/permalink/FVPs-Corstone-IoT/Corstone-320/FVP_Corstone_SSE-320_11.32.cs320_3_Linux_x86.tar.gz'
    URL_BASE_AEM_A='https://developer.arm.com/-/cdn-downloads/permalink/FVPs-Architecture/FM-11.31/FVP_Base_RevC_AEMvA_11.31_28_Linux_x86.tar.gz'
    URL_BASE_AEM_R='https://developer.arm.com/-/cdn-downloads/permalink/FVPs-Architecture/FM-11.31/FVP_Base_AEMv8R_11.31_28_Linux_x86.tar.gz'
    URL_CRYPTO='https://developer.arm.com/-/cdn-downloads/permalink/Fast-Models-Crypto-Plug-in/FM-11.31/FastModels_crypto_11.31_28_Linux_x86.tar.gz'
elif [ "$MACHINE_HARDWARE" == 'aarch64' ]; then
    URL_CORSTONE_320='https://developer.arm.com/-/cdn-downloads/permalink/FVPs-Corstone-IoT/Corstone-320/FVP_Corstone_SSE-320_11.32.cs320_3_Linux_armv8.tar.gz'
    URL_BASE_AEM_A='https://developer.arm.com/-/cdn-downloads/permalink/FVPs-Architecture/FM-11.31/FVP_Base_RevC_AEMvA_11.31_28_Linux_armv8.tar.gz'
    URL_BASE_AEM_R='https://developer.arm.com/-/cdn-downloads/permalink/FVPs-Architecture/FM-11.31/FVP_Base_AEMv8R_11.31_28_Linux_armv8.tar.gz'
    URL_CRYPTO='https://developer.arm.com/-/cdn-downloads/permalink/Fast-Models-Crypto-Plug-in/FM-11.31/FastModels_crypto_11.31_28_Linux_armv8.tar.gz'
else
    echo Unknown architecture: $MACHINE_HARDWARE
    exit 1
fi

FILENAME_CORSTONE_320=$(basename "$URL_CORSTONE_320")
FILENAME_BASE_AEM_A=$(basename "$URL_BASE_AEM_A")
FILENAME_BASE_AEM_R=$(basename "$URL_BASE_AEM_R")
FILENAME_CRYPTO=$(basename "$URL_CRYPTO")

if which curl >&/dev/null; then
    download_to() { curl --retry 5 "$1" -o "$2"; }
elif which wget >&/dev/null; then
    download_to() { wget --no-clobber "$1" -O "$2"; }
else
    echo "Need one of curl and wget installed to download files" >&2
    exit 1
fi

mkdir -p download
pushd download
DOWNLOAD_DIR="$(pwd)"
download_to "${URL_CORSTONE_320}" "${FILENAME_CORSTONE_320}"
download_to "${URL_BASE_AEM_A}" "${FILENAME_BASE_AEM_A}"
download_to "${URL_BASE_AEM_R}" "${FILENAME_BASE_AEM_R}"
download_to "${URL_CRYPTO}" "${FILENAME_CRYPTO}"
popd

mkdir -p install
pushd install

if [ ! -d "Corstone-320_11.32.cs320_3" ]; then
tar -xf ${DOWNLOAD_DIR}/${FILENAME_CORSTONE_320}
./"${FILENAME_CORSTONE_320%.tar.gz}.sh" --destination ./Corstone-320_11.32.cs320_3 "${INSTALLER_FLAGS_CORSTONE[@]}"
fi

if [ ! -d "FVP_Base_RevC_AEMvA_11.31_28" ]; then
tar -xf ${DOWNLOAD_DIR}/${FILENAME_BASE_AEM_A}
# (Extracted directly into the directory named above, no installer)
fi

if [ ! -d "FVP_Base_AEMv8R_11.31_28" ]; then
tar -xf ${DOWNLOAD_DIR}/${FILENAME_BASE_AEM_R}
# (Extracted directly into the directory named above, no installer)
fi

if [ ! -d "FastModels_11.31_28" ]; then
tar -xf ${DOWNLOAD_DIR}/${FILENAME_CRYPTO}
# (Extracted directly into the directory named above, no installer)
fi

popd
