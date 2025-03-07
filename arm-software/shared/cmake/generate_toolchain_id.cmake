# Generate a toolchain ID.
# This should identify the product, and where the build came from.

# The ID begins with a product code (e.g. E for Embedded), followed
# by build identifier. The build identifier will depend on the
# build environment: for Jenkins builds, the pipeline and build
# number will be sufficient.

if(NOT DEFINED LLVM_TOOLCHAIN_PROJECT_CODE)
    message(FATAL_ERROR "LLVM_TOOLCHAIN_PROJECT_CODE must be set before including generate_toolchain_id.cmake")
endif()

# JENKINS_URL, BUILD_URL and BUILD_NUMBER will be set in a Jenkins environment.
if(DEFINED ENV{JENKINS_URL} AND DEFINED ENV{BUILD_URL} AND DEFINED ENV{BUILD_NUMBER})
    set(builder_name $ENV{BUILD_URL})
    set(build_number $ENV{BUILD_NUMBER})
    # BUILD_URL also contains the build number, which is not needed.
    string(REGEX REPLACE "/${build_number}/$" "/" builder_name ${builder_name})
else()
    # Use HOSTNAME if nothing else is available.
    cmake_host_system_information(RESULT builder_name QUERY HOSTNAME)
    set(build_number 0)
endif()

# Pad build number
string(LENGTH ${build_number} len_build_number)
if(len_build_number LESS 4)
    math(EXPR pad_len "4 - ${len_build_number}")
    string(REPEAT "0" ${pad_len} pad_chars)
    string(PREPEND build_number ${pad_chars})
endif()

# Version suffix should be optional.
if(DEFINED LLVM_TOOLCHAIN_VERSION_SUFFIX)
    set(ID_SUFFIX "-${LLVM_TOOLCHAIN_VERSION_SUFFIX}")
else()
    set(ID_SUFFIX "")
endif()

string(SHA256 builder_name_hash ${builder_name})
string(SUBSTRING ${builder_name_hash} 0 8 builder_name_hash)
set(ARM_TOOLCHAIN_ID
    "${LLVM_TOOLCHAIN_PROJECT_CODE}${build_number}${ID_SUFFIX} (${builder_name_hash})" CACHE STRING
    "Toolchain ID to identify product."
)

# Print the ID in script mode.
get_property(cmake_role GLOBAL PROPERTY CMAKE_ROLE)
if(cmake_role STREQUAL SCRIPT)
    execute_process(COMMAND ${CMAKE_COMMAND} -E echo ${ARM_TOOLCHAIN_ID})
endif()
