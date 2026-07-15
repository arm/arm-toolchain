#!/bin/bash
set -e  # Exit immediately if a command exits with a non-zero status

# Docker environment does not set the USER environment variable.
# So rely on 'id'.
CURRENT_USER_NAME="$(id -un)"

# Source the script
if [ -f "/home/${CURRENT_USER_NAME}/spack/share/spack/setup-env.sh" ]; then
    source /home/${CURRENT_USER_NAME}/spack/share/spack/setup-env.sh
fi

# Execute the main command
exec "$@"

