#!/usr/bin/env bash
set -o errexit
set -o pipefail
if [[ "${TRACE-0}" == "1" ]]; then
    set -o xtrace
fi

################################
## Configuration: Directories ##
################################

BASE_DIR="$(pwd)"
SOURCES_DIR="${BASE_DIR}/src"
LIBRARIES_DIR="${BASE_DIR}/lib"
PATCHES_DIR="${BASE_DIR}/patches"
BUILD_DIR="${BASE_DIR}/build"
LOGS_DIR="${BASE_DIR}/logs"
OUTPUT_DIR="${BASE_DIR}/output"
DIRS=("${SOURCES_DIR}" "${LIBRARIES_DIR}" "${PATCHES_DIR}" "${BUILD_DIR}"  "${LOGS_DIR}" "${OUTPUT_DIR}")

#########################
## Configuration: User ##
#########################

CURRENT_USER_NAME="$(id -un)"

##########################
## Configuration: Build ##
##########################

ATFL_REPOSITORY="${ATFL_REPOSITORY:-"https://github.com/arm/arm-toolchain.git"}"
ATFL_BRANCH=${ATFL_BRANCH:-"arm-software"}
ATFL_COMMIT=${ATFL_COMMIT:-"HEAD"}

########################
## Setup: Directories ##
########################

# Check that the directories exist and are owned by the current user.
# These paths should have been created by the build.Dockerfile execution.
function check_directories() {
    for dir in "${DIRS[@]}"; do
        if [[ ! -d "${dir}" ]]; then
            echo "Expected directory not found in environment: ${dir}"
            exit 1
        fi
        if [[ "$(stat -c %U "${dir}")" != "${CURRENT_USER_NAME}" ]]; then
            echo "Directory not owned by workspace user: ${dir}"
            exit 1
        fi
    done
}

#######################
## Functions: Helper ##
#######################

# Print a title banner using figlet if available
function print_banner() {
    echo "############################"
    echo "## ATfL Build Environment ##"
    echo "############################"
}

# Ouptut an echo but using bold font
function echo_bold() {
    echo -e "\033[1m${1}\033[0m"
}

##################
## Main Program ##
##################

print_banner
check_directories

echo_bold "Executing command: $*"
exec "$@"
