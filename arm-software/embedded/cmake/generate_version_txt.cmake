# The following line will look different depending on how you got this
# source file. If you got it from a Git repository then it will contain
# a string in the git pretty format with dollar symbols. If you got it
# from a source archive then the `git archive` command should have
# replaced the format string with the Git revision at the time the
# archive was created. This is configured in the .gitattributes file.
# In the former case, this script will run a Git command to find out the
# current revision. In the latter case the revision will be used as is.
set(armtoolchain_COMMIT "$Format:%H$")

if(NOT ${armtoolchain_COMMIT} MATCHES "^[a-f0-9]+$")
    execute_process(
        COMMAND git -C ${ArmToolchainForEmbedded_SOURCE_DIR} rev-parse HEAD
        OUTPUT_VARIABLE armtoolchain_COMMIT
        OUTPUT_STRIP_TRAILING_WHITESPACE
        COMMAND_ERROR_IS_FATAL ANY
    )
endif()

set(LLVM_TOOLCHAIN_LIBC_SOURCE_LINES "")
foreach(libc IN LISTS LLVM_TOOLCHAIN_ENABLED_LIBCS)
    if(libc STREQUAL llvmlibc)
        set(libc_url "https://github.com/arm/arm-toolchain/tree/arm-software/libc")
        set(libc_commit ${armtoolchain_COMMIT})
    else()
        if(libc MATCHES "^newlib")
            set(base_library newlib)
        else()
            set(base_library ${libc})
        endif()

        execute_process(
            COMMAND git -C ${${base_library}_SOURCE_DIR} rev-parse HEAD
            OUTPUT_VARIABLE ${base_library}_COMMIT
            OUTPUT_STRIP_TRAILING_WHITESPACE
            COMMAND_ERROR_IS_FATAL ANY
        )
        set(libc_url ${${base_library}_URL})
        set(libc_commit ${${base_library}_COMMIT})
    endif()
    string(APPEND LLVM_TOOLCHAIN_LIBC_SOURCE_LINES "* ${libc}: ${libc_url} (commit ${libc_commit})\n")
endforeach()

configure_file(
    ${CMAKE_CURRENT_LIST_DIR}/VERSION.txt.in
    ${CMAKE_CURRENT_BINARY_DIR}/VERSION.txt
)
