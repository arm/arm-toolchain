#!/bin/bash

# SPDX-FileCopyrightText: Copyright 2024 Arm Limited and/or its affiliates <open-source-office@arm.com>

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
    URL_CORSTONE_310='https://developer.arm.com/-/cdn-downloads/permalink/FVPs-Corstone-IoT/Corstone-310/FVP_Corstone_SSE-310_11.27_42_Linux64.tgz'
    URL_BASE_AEM_A='https://developer.arm.com/-/cdn-downloads/permalink/FVPs-Architecture/FM-11.28/FVP_Base_RevC-2xAEMvA_11.28_23_Linux64.tgz'
    URL_BASE_AEM_R='https://developer.arm.com/-/cdn-downloads/permalink/FVPs-Architecture/FM-11.28/FVP_Base_AEMv8R_11.28_23_Linux64.tgz'
    URL_CRYPTO='https://developer.arm.com/-/cdn-downloads/permalink/Fast-Models-Crypto-Plug-in/FM-11.27/FastModels_crypto_11.27.019_Linux64.tgz'
elif [ "$MACHINE_HARDWARE" == 'aarch64' ]; then
    URL_CORSTONE_310='https://developer.arm.com/-/cdn-downloads/permalink/FVPs-Corstone-IoT/Corstone-310/FVP_Corstone_SSE-310_11.27_42_Linux64_armv8l.tgz'
    URL_BASE_AEM_A='https://developer.arm.com/-/cdn-downloads/permalink/FVPs-Architecture/FM-11.28/FVP_Base_RevC-2xAEMvA_11.28_23_Linux64_armv8l.tgz'
    URL_BASE_AEM_R='https://developer.arm.com/-/cdn-downloads/permalink/FVPs-Architecture/FM-11.28/FVP_Base_AEMv8R_11.28_23_Linux64_armv8l.tgz'
    URL_CRYPTO='https://developer.arm.com/-/cdn-downloads/permalink/Fast-Models-Crypto-Plug-in/FM-11.27/FastModels_crypto_11.27.019_Linux64_armv8l.tgz'
else
    echo Unknown architecture: $MACHINE_HARDWARE
    exit 1
fi

FILENAME_CORSTONE_310=$(basename "$URL_CORSTONE_310")
FILENAME_BASE_AEM_A=$(basename "$URL_BASE_AEM_A")
FILENAME_BASE_AEM_R=$(basename "$URL_BASE_AEM_R")
FILENAME_CRYPTO=$(basename "$URL_CRYPTO")
EXTRACTEDNAME_CRYPTO=$(basename "$URL_CRYPTO" .tgz)

mkdir -p download
pushd download
DOWNLOAD_DIR="$(pwd)"
wget --content-disposition --no-clobber "${URL_CORSTONE_310}"
wget --content-disposition --no-clobber "${URL_BASE_AEM_A}"
wget --content-disposition --no-clobber "${URL_BASE_AEM_R}"
wget --content-disposition --no-clobber "${URL_CRYPTO}"
popd

mkdir -p install
pushd install

if [ ! -d "Corstone-310" ]; then
tar -xf ${DOWNLOAD_DIR}/${FILENAME_CORSTONE_310}
./FVP_Corstone_SSE-310.sh --destination ./Corstone-310 "${INSTALLER_FLAGS_CORSTONE[@]}"
fi

if [ ! -d "Base_RevC_AEMvA_pkg" ]; then
tar -xf ${DOWNLOAD_DIR}/${FILENAME_BASE_AEM_A}
# (Extracted directly into ./Base_RevC_AEMvA_pkg/, no installer)
fi

if [ ! -d "AEMv8R_base_pkg" ]; then
tar -xf ${DOWNLOAD_DIR}/${FILENAME_BASE_AEM_R}
# (Extracted directly into ./AEMv8R_base_pkg/, no installer)
fi

if [ ! -d "FastModelsPortfolio_11.27" ]; then
tar -xf ${DOWNLOAD_DIR}/${FILENAME_CRYPTO}
# SDDKW-93582: Non-interactive installation fails if cwd is different.
pushd ${EXTRACTEDNAME_CRYPTO}
# This installer doesn't allow providing a default path for
# interactive installation. The user will have to enter the install
# directory as the target by hand.
./setup.bin "${INSTALLER_FLAGS_CRYPTO[@]}"
popd
fi

popd
