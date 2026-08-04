#!/usr/bin/env bash
set -o errexit
set -o pipefail
if [[ "${TRACE-0}" == "1" ]]; then
    set -o xtrace
fi

################################
## Configuration: Directories ##
################################

BASE_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
CHANGELOG_MD_PATH=${CHANGELOG_MD_PATH:-"${BASE_DIR}/CHANGELOG.md"}
SBOM_FILE_PATH=${SBOM_FILE_PATH:-"${BASE_DIR}/SBOM_Files/ATfL-SBOM.spdx.json"}
MKMODULEDIRS_PATH=${MKMODULEDIRS_PATH:-"${BASE_DIR}/mkmoduledirs.sh.var"}
SOURCES_DIR=${SOURCES_DIR:-"$(git -C "${BASE_DIR}" rev-parse --show-toplevel)"}
LIBRARIES_DIR=${LIBRARIES_DIR:-"${BASE_DIR}/lib"}
BOLTTESTS_DIR=${BOLTTESTS_DIR:-"${BASE_DIR}/bolt-tests"}
PATCHES_DIR=${PATCHES_DIR:-"${BASE_DIR}/patches"}
DOCS_DIR=${DOCS_DIR:-"${BASE_DIR}/docs"}
BUILD_DIR=${BUILD_DIR:-"${BASE_DIR}/build"}
ATFL_DIR=${ATFL_DIR:-"${BUILD_DIR}/atfl"}
LOGS_DIR=${LOGS_DIR:-"${BASE_DIR}/logs"}
OUTPUT_DIR=${OUTPUT_DIR:-"${BASE_DIR}/output"}

#########################
## Configuration: Mode ##
#########################

INTERACTIVE=false

##########################
## Configuration: Build ##
##########################

if [[ -n "${COMMON_CMAKE_FLAGS}" ]]; then
    echo "Do not pass the obsolete/undocumented/misleading COMMON_CMAKE_FLAGS variable."
    exit 1
fi

RELEASE_FLAGS=${RELEASE_FLAGS:-"false"}
LLVM_VERSION_MAJOR=$(grep -i set "${SOURCES_DIR}/cmake/Modules/LLVMVersion.cmake" | grep LLVM_VERSION_MAJOR | grep -o '[0-9]\+')
LLVM_VERSION_MINOR=$(grep -i set "${SOURCES_DIR}/cmake/Modules/LLVMVersion.cmake" | grep LLVM_VERSION_MINOR | grep -o '[0-9]\+')
LLVM_VERSION_PATCH=$(grep -i set "${SOURCES_DIR}/cmake/Modules/LLVMVersion.cmake" | grep LLVM_VERSION_PATCH | grep -o '[0-9]\+')
TOOLCHAIN_VERSION="${LLVM_VERSION_MAJOR}.${LLVM_VERSION_MINOR}.${LLVM_VERSION_PATCH}"
ATFL_VERSION=${ATFL_VERSION:-"${TOOLCHAIN_VERSION}"}
if [[ "${ATFL_VERSION}" == "0.0" ]]
then
  TOOLCHAIN_VERSION="0.0"
fi
OS_NAME=${OS_NAME:-"linux"}
TAR_NAME=${TAR_NAME:-"atfl-${ATFL_VERSION}-${OS_NAME}-$(uname -m).tar.gz"}
ATFL_ASSERTIONS=${ATFL_ASSERTIONS:-"ON"}
ATFL_BOLTED=${ATFL_BOLTED:-"OFF"}
ATFL_TARGET_TRIPLE=${ATFL_TARGET_TRIPLE:-"$(uname -m)-unknown-linux-gnu"}
ARM_TOOLCHAIN_ID=$(cmake -DLLVM_TOOLCHAIN_PROJECT_CODE=L -P "${SOURCES_DIR}"/arm-software/shared/cmake/generate_toolchain_id.cmake)
PROCESSOR_COUNT=$(getconf _NPROCESSORS_ONLN)
PARALLEL_JOBS=${PARALLEL_JOBS:-"${PROCESSOR_COUNT}"}
# " <-- this is to help syntax highlighters to find a matching double quote
STAGES=(
    "bootstrap_compiler_build"
    "libcpp_build"
    "product_build"
    "static_libomp_build"
)
ZLIB_STATIC_PATH=${ZLIB_STATIC_PATH:-"/usr/lib/$(uname -m)-linux-gnu/libz.a"}
RELOCS_LINKER_FLAGS="-Wl,--emit-relocs,-znow"
COMMON_LINKER_FLAGS="-Wl,--build-id"

# Safe to use by all stages
declare -A COMMON_CMAKE_FLAGS=(
    ["CLANG_ENABLE_LIBXML2:BOOL"]=OFF
    ["LLVM_ENABLE_LIBXML2:BOOL"]=OFF
    ["LLVM_ENABLE_ZLIB:BOOL"]=ON
    ["LLVM_USE_STATIC_ZSTD:BOOL"]=True
    ["LLVM_ENABLE_ZSTD:BOOL"]=ON
    ["LLVM_BINUTILS_INCDIR:PATH"]="\"/usr/include\""
    ["LLVM_ENABLE_ASSERTIONS:BOOL"]=${ATFL_ASSERTIONS}
    ["LLVM_ENABLE_PIC:BOOL"]=ON
    ["LLVM_ENABLE_FFI:BOOL"]=OFF
    ["LLVM_ENABLE_BINDINGS:BOOL"]=OFF
    ["LLVM_ENABLE_PLUGINS:BOOL"]=ON
    ["LLVM_TOOL_LIBUNWIND_BUILD:BOOL"]=ON
    ["LLVM_TARGETS_TO_BUILD:STRING"]="\"AArch64\""
    ["LLVM_DEFAULT_TARGET_TRIPLE:STRING"]="\"${ATFL_TARGET_TRIPLE}\""
    ["ZLIB_LIBRARY_RELEASE:FILEPATH"]="\"${ZLIB_STATIC_PATH}\""
)

declare -A USE_BOOTSTRAP_CMAKE_FLAGS=(
    ["CMAKE_C_COMPILER:FILEPATH"]="\"${BUILD_DIR}/bootstrap_compiler/bin/clang\""
    ["CMAKE_CXX_COMPILER:FILEPATH"]="\"${BUILD_DIR}/bootstrap_compiler/bin/clang++\""
    ["CMAKE_INSTALL_PREFIX:PATH"]="\"${ATFL_DIR}\""
    ["LLVM_ENABLE_LLD:BOOL"]=ON
)

declare -A COMPILER_CMAKE_FLAGS=(
    ["CMAKE_CXX_FLAGS:STRING"]="\"-stdlib++-isystem ${ATFL_DIR}/include/c++/v1 -D_LIBCPP_VERBOSE_ABORT_NOT_NOEXCEPT\""
    ["CLANG_PLUGIN_SUPPORT:BOOL"]=ON
    ["CLANG_ENABLE_STATIC_ANALYZER:BOOL"]=ON
    ["CLANG_TOOL_LIBCLANG_BUILD:BOOL"]=ON
    ["LIBCLANG_BUILD_STATIC:BOOL"]=ON
    ["ARM_TOOLCHAIN_ID:STRING"]="\"${ARM_TOOLCHAIN_ID}\""
    ["CLANG_VENDOR:STRING"]="\"Arm Toolchain for Linux ${TOOLCHAIN_VERSION}\""
    ["FLANG_VENDOR:STRING"]="\"Arm Toolchain for Linux ${TOOLCHAIN_VERSION}\""
    ["LLVM_VERSION_SUFFIX:STRING"]="\"\""
    ["LLVM_TOOL_BOLT_BUILD:BOOL"]=True
)

declare -A SKIP_RPATH_CMAKE_FLAGS=(
    ["CMAKE_SKIP_RPATH:BOOL"]=Yes
    ["CMAKE_SKIP_INSTALL_RPATH:BOOL"]=Yes
)

declare -A USE_RPATH_CMAKE_FLAGS=(
    ["CMAKE_SKIP_RPATH:BOOL"]=No
    ["CMAKE_SKIP_INSTALL_RPATH:BOOL"]=No
)

declare -A BOLT_CMAKE_FLAGS=(
    ["BOLT_TARGETS_TO_BUILD:STRING"]="\"AArch64\""
    ["BOLT_BUILD_TOOLS:BOOL"]=ON
    ["BOLT_ENABLE_RUNTIME:BOOL"]=ON
)

declare -A COMPILER_RT_CMAKE_FLAGS=(
    ["COMPILER_RT_DEFAULT_TARGET_ARCH:STRING"]="\"AArch64\""
    ["COMPILER_RT_BUILD_SANITIZERS:BOOL"]=OFF
    ["COMPILER_RT_BUILD_LIBFUZZER:BOOL"]=ON
    ["COMPILER_RT_BUILD_ORC:BOOL"]=OFF
    ["COMPILER_RT_USE_LIBCXX:BOOL"]=ON
    ["COMPILER_RT_BUILD_BUILTINS:BOOL"]=ON
    ["COMPILER_RT_USE_BUILTINS_LIBRARY:BOOL"]=ON
    ["COMPILER_RT_EXCLUDE_ATOMIC_BUILTIN:BOOL"]=OFF
    ["COMPILER_RT_BUILD_STANDALONE_LIBATOMIC:BOOL"]=OFF
    ["COMPILER_RT_USE_ATOMIC_LIBRARY:BOOL"]=ON
    ["COMPILER_RT_USE_LLVM_UNWINDER:BOOL"]=OFF
    ["COMPILER_RT_LIBRARY_atomic_${ATFL_TARGET_TRIPLE}:STRING"]="\"-rtlib=compiler-rt\""
)

declare -A FLANG_RT_CMAKE_FLAGS=(
    ["FLANG_RT_ENABLE_SHARED:BOOL"]=ON
    ["FLANG_RT_ENABLE_STATIC:BOOL"]=ON
)

declare -A LIBCXX_CMAKE_FLAGS=(
    ["LIBCXXABI_USE_COMPILER_RT:BOOL"]=ON
    ["LIBCXXABI_USE_LLVM_UNWINDER:BOOL"]=ON
    ["LIBCXXABI_ENABLE_STATIC_UNWINDER:BOOL"]=ON
    ["LIBCXXABI_ENABLE_EXCEPTIONS:BOOL"]=ON
    ["LIBCXXABI_ENABLE_ASSERTIONS:BOOL"]=OFF
    ["LIBCXXABI_ENABLE_SHARED:BOOL"]=ON
    ["LIBCXXABI_ENABLE_STATIC:BOOL"]=ON
    ["LIBCXXABI_ENABLE_THREADS:BOOL"]=ON
    ["LIBCXXABI_HAS_EXTERNAL_THREAD_API:BOOL"]=OFF
    ["LIBCXX_CXX_ABI:STRING"]="\"libcxxabi\""
    ["LIBCXX_USE_COMPILER_RT:BOOL"]=ON
    ["LIBCXX_ENABLE_EXCEPTIONS:BOOL"]=ON
    ["LIBCXX_ENABLE_ASSERTIONS:BOOL"]=OFF
    ["LIBCXX_ENABLE_SHARED:BOOL"]=ON
    ["LIBCXX_ENABLE_STATIC:BOOL"]=ON
    ["LIBCXX_ENABLE_THREADS:BOOL"]=ON
    ["LIBCXX_HAS_EXTERNAL_THREAD_API:BOOL"]=OFF
    ["LIBCXX_ENABLE_LOCALIZATION:BOOL"]=ON
    ["LIBCXX_ENABLE_TIME_ZONE_DATABASE:BOOL"]=OFF
    ["LIBCXX_ENABLE_UNICODE:BOOL"]=ON
    ["LIBCXX_ENABLE_WIDE_CHARACTERS:BOOL"]=ON
)

declare -A LIBOMP_SHARED_CMAKE_FLAGS=(
    ["LIBOMP_ENABLE_SHARED:BOOL"]=True
    ["LIBOMP_OMPT_SUPPORT:BOOL"]=ON
    ["LIBOMP_COPY_EXPORTS:BOOL"]=False
    ["LIBOMP_USE_HWLOC:BOOL"]=False
    ["LIBOMP_OMPD_GDB_SUPPORT:BOOL"]=OFF
)

declare -A LIBOMP_NOSHARED_CMAKE_FLAGS=(
    ["LIBOMP_ENABLE_SHARED:BOOL"]=False
    ["LIBOMP_OMPT_SUPPORT:BOOL"]=OFF
    ["LIBOMP_COPY_EXPORTS:BOOL"]=False
    ["LIBOMP_USE_HWLOC:BOOL"]=False
    ["LIBOMP_OMPD_GDB_SUPPORT:BOOL"]=OFF
)

declare -A LIBUNWIND_SHARED_CMAKE_FLAGS=(
    ["LIBUNWIND_USE_COMPILER_RT:BOOL"]=ON
    ["LIBUNWIND_ENABLE_SHARED:BOOL"]=ON
    ["LIBUNWIND_ENABLE_STATIC:BOOL"]=ON
    ["LIBUNWIND_ENABLE_ASSERTIONS:BOOL"]=OFF
    ["LIBUNWIND_ENABLE_THREADS:BOOL"]=ON
)

declare -A LIBUNWIND_NOSHARED_CMAKE_FLAGS=(
    ["LIBUNWIND_USE_COMPILER_RT:BOOL"]=ON
    ["LIBUNWIND_ENABLE_SHARED:BOOL"]=OFF
    ["LIBUNWIND_ENABLE_STATIC:BOOL"]=ON
    ["LIBUNWIND_ENABLE_ASSERTIONS:BOOL"]=OFF
    ["LIBUNWIND_ENABLE_THREADS:BOOL"]=ON
)

CMAKE_ARGS=()
CMAKE_BUILD_ARGS=(-j"${PARALLEL_JOBS}")
NINJA_ARGS=(-j"${PARALLEL_JOBS}")

if [[ "${TRACE-0}" == "1" ]]; then
    CMAKE_ARGS+=(--trace-expand)
    COMMON_CMAKE_FLAGS["CMAKE_VERBOSE_MAKEFILE:BOOL"]=ON
    CMAKE_BUILD_ARGS+=(-v)
    NINJA_ARGS+=(-v)
fi

###############
## Functions ##
###############

abort() {
    echo >&2 '
    ***************
    *** ABORTED ***
    ***************
    '
    echo "An error occurred. Exiting..." >&2
    if ${INTERACTIVE}; then
        cd "${BASE_DIR}"
        bash
    else
        exit 1
    fi
}

echo_bold() {
    echo -e "\033[1m$1\033[0m"
}

run_command() {
    echo "With: PATH=\"${PATH}\" LD_LIBRARY_PATH=\"${LD_LIBRARY_PATH}\""
    echo "Running: $*"
    "$@"
}

run_test_command() {
    local xml_output="$1"
    local log_file="$2"
    shift 2

    if ! LIT_OPTS="${LIT_OPTS} --xunit-xml-output=${xml_output}" \
        run_command ninja "${NINJA_ARGS[@]}" "$@" 2>&1 | tee -a "${log_file}"; then
        echo "WARNING: Test command failed, continuing: $*" | tee -a "${log_file}"
    fi
}

print_help() {
    cat <<EOF
Usage: $0 [OPTIONS]

Options:
  -h, --help          Show this help message and exit
  -i, --interactive   Run in interactive mode (builds fail into a bash shell)

Environment Variables:

    CHANGELOG_MD_PATH   Specifies the location of the CHANGELOG.md file to bundle
                        (default: ${CHANGELOG_MD_PATH})
    SBOM_FILE_PATH      Specifies the location of the SBOM JSON file to bundle
                        (default: ${SBOM_FILE_PATH})
    MKMODULEDIRS_PATH   Specifies the location of mkmoduledirs.sh.var to tweak
                        (default: ${MKMODULEDIRS_PATH})
    SOURCES_DIR         The directory where all source code will be stored
                        (default: ${SOURCES_DIR})
    BOLTTESTS_DIR       The optional directory where the bolt-tests repo has been cloned
                        (default: ${BOLTTESTS_DIR})
    LIBRARIES_DIR       The optional directory where the ArmPL veclibs will be stored
                        (default: ${LIBRARIES_DIR})
    PATCHES_DIR         The optional directory where all patches will be stored
                        (default: ${PATCHES_DIR})
    DOCS_DIR            The directory where ATfL documents will be stored
                        (default: ${DOCS_DIR})
    BUILD_DIR           The directory where all build output will be stored
                        (default: ${BUILD_DIR})
    LOGS_DIR            The directory where all build logs will be stored
                        (default: ${LOGS_DIR})
    OUTPUT_DIR          The directory where all build output will be stored
                        (default: ${OUTPUT_DIR})
    RELEASE_FLAGS       Enable release flags in the build true/false
                        (default: ${RELEASE_FLAGS})
    PARALLEL_JOBS       The number of parallel jobs to run during the build
                        (default: ${PARALLEL_JOBS})
    ATFL_ASSERTIONS     Enable assertions in the build ON/OFF
                        (default: ${ATFL_ASSERTIONS})
    ATFL_BOLTED         Specify whether the clang and flang compilers should be bolted
                        (default: ${ATFL_BOLTED})
    ATFL_VERSION        Specify the version string
                        (default: ${ATFL_VERSION})
    ATFL_TARGET_TRIPLE  Specify the default target triple
                        (default: ${ATFL_TARGET_TRIPLE})
    OS_NAME             Specify the OS name
                        (default: ${OS_NAME})
    TAR_NAME            The name of the tarball to be created
                        (default: ${TAR_NAME})
    ZLIB_STATIC_PATH    Specifies the location of the static zlib library (libz.a)
                        (default: ${ZLIB_STATIC_PATH})
EOF
}

libraries_present() {
    if [ "$(ls -A "${LIBRARIES_DIR}")" ]; then
        return 0
    else
        return 1
    fi
}

bolttests_present() {
    if [ "$(ls -A "${BOLTTESTS_DIR}")" ]; then
        return 0
    else
        return 1
    fi
}

patches_present() {
    if [ "$(ls -A "${PATCHES_DIR}")" ]; then
        return 0
    else
        return 1
    fi
}

apply_patches() {
    if ! patches_present; then
        echo "No patches to apply."
        return
    fi
    cd "${SOURCES_DIR}"
    echo "ATfL SHA: $(git rev-parse HEAD)"
    echo_bold "Applying patches..."
    for patch in "${PATCHES_DIR}"/*.patch; do
        echo "Applying patch: ${patch}"
        patch -p1 <"${patch}" || true
    done
    echo_bold "Applying patches...done"
}

print_forced_cached_flag() {
    echo "set($(echo "$1" | cut -d ":" -f1) $2 CACHE $(echo "$1" | cut -s -d ":" -f2) \"\" FORCE)"
}

print_forced_cmake_flags_cache() {
    local -n arr=$1
    local -a keys=()

    mapfile -t keys < <(printf '%s\n' "${!arr[@]}" | LC_ALL=C sort)
    for i in "${keys[@]}"; do
        print_forced_cached_flag "$i" "${arr["$i"]}"
    done
}

bootstrap_compiler_default_config() {
    echo "-fuse-ld=lld" >"${BUILD_DIR}"/bootstrap_compiler/bin/clang.cfg
    echo "-fuse-ld=lld" >"${BUILD_DIR}"/bootstrap_compiler/bin/clang++.cfg
}

bootstrap_compiler_build() {
    mkdir -p "${BUILD_DIR}/stage/bootstrap_compiler"
    cd "${BUILD_DIR}/stage/bootstrap_compiler"

    { print_forced_cmake_flags_cache "COMMON_CMAKE_FLAGS"
      print_forced_cmake_flags_cache "USE_RPATH_CMAKE_FLAGS"
      print_forced_cmake_flags_cache "LIBUNWIND_NOSHARED_CMAKE_FLAGS"
    } > flags.cmake

    run_command cmake "${CMAKE_ARGS[@]}" -G Ninja "${SOURCES_DIR}/llvm" \
        -C flags.cmake \
        -DBUILD_SHARED_LIBS=False \
        -DCMAKE_BUILD_TYPE=Release \
        -DCMAKE_ASM_FLAGS_RELEASE="-O2 -DNDEBUG" \
        -DCMAKE_CXX_FLAGS_RELEASE="-O2 -DNDEBUG" \
        -DCMAKE_C_FLAGS_RELEASE="-O2 -DNDEBUG" \
        -DCMAKE_INSTALL_PREFIX="${BUILD_DIR}/bootstrap_compiler" \
        -DLLVM_ENABLE_LLD=OFF \
        -DLLVM_ENABLE_LIBCXX=OFF \
        -DLLVM_ENABLE_PROJECTS="llvm;clang;lld" \
        -DLLVM_ENABLE_RUNTIMES="compiler-rt;libunwind" \
        -DCLANG_DEFAULT_RTLIB="compiler-rt" \
        -DCLANG_DEFAULT_UNWINDLIB="libunwind" \
        -DCLANG_ENABLE_LIBXML2=OFF \
        -DCLANG_PLUGIN_SUPPORT=ON \
        -DCLANG_ENABLE_STATIC_ANALYZER=ON \
        -DCLANG_TOOL_LIBCLANG_BUILD=ON \
        -DLIBCLANG_BUILD_STATIC=ON \
        -DCOMPILER_RT_DEFAULT_TARGET_ARCH="AArch64" \
        -DCOMPILER_RT_BUILD_SANITIZERS=OFF \
        -DCOMPILER_RT_BUILD_LIBFUZZER=OFF \
        -DCOMPILER_RT_BUILD_ORC=OFF \
        -DCOMPILER_RT_USE_LIBCXX=OFF \
        -DCOMPILER_RT_BUILD_BUILTINS=ON \
        -DCOMPILER_RT_USE_BUILTINS_LIBRARY=OFF \
        -DCOMPILER_RT_EXCLUDE_ATOMIC_BUILTIN=OFF \
        -DCOMPILER_RT_BUILD_STANDALONE_LIBATOMIC=OFF \
        -DCOMPILER_RT_USE_ATOMIC_LIBRARY=ON \
        -DCOMPILER_RT_USE_LLVM_UNWINDER=ON \
        -DCOMPILER_RT_ENABLE_STATIC_UNWINDER=ON \
        2>&1 | tee "${LOGS_DIR}/bootstrap_compiler.txt"
    run_command cmake --build . "${CMAKE_BUILD_ARGS[@]}" 2>&1 | tee -a "${LOGS_DIR}/bootstrap_compiler.txt"
    run_command cmake --install . 2>&1 | tee -a "${LOGS_DIR}/bootstrap_compiler.txt"
    export PATH="${BUILD_DIR}/bootstrap_compiler/bin:${PATH}"
    bootstrap_compiler_default_config
    run_test_command "${LOGS_DIR}/bootstrap_check_all.xml" "${LOGS_DIR}/bootstrap_compiler.txt" check-all
}

libcpp_build() {
    mkdir -p "${BUILD_DIR}/stage/libcpp_build"
    cd "${BUILD_DIR}/stage/libcpp_build"
    bootstrap_compiler_default_config

    { print_forced_cmake_flags_cache "COMMON_CMAKE_FLAGS"
      print_forced_cmake_flags_cache "USE_BOOTSTRAP_CMAKE_FLAGS"
      print_forced_cmake_flags_cache "SKIP_RPATH_CMAKE_FLAGS"
      print_forced_cmake_flags_cache "LIBCXX_CMAKE_FLAGS"
      print_forced_cmake_flags_cache "LIBUNWIND_NOSHARED_CMAKE_FLAGS"
    } > flags.cmake

    local libs="-rtlib=compiler-rt -unwindlib=libunwind -Wl,--as-needed ${COMMON_LINKER_FLAGS} ${RELOCS_LINKER_FLAGS}"
    run_command cmake "${CMAKE_ARGS[@]}" -G Ninja "${SOURCES_DIR}/runtimes" \
        -C flags.cmake \
        -DBUILD_SHARED_LIBS=False \
        -DCMAKE_BUILD_TYPE=RelWithDebInfo \
        -DCMAKE_CXX_FLAGS="-D_LIBCPP_VERBOSE_ABORT_NOT_NOEXCEPT" \
        -DCMAKE_EXE_LINKER_FLAGS="${libs}" \
        -DCMAKE_MODULE_LINKER_FLAGS="${libs}" \
        -DCMAKE_SHARED_LINKER_FLAGS="${libs}" \
        -DLLVM_ENABLE_RUNTIMES="libcxx;libcxxabi;libunwind" \
        2>&1 | tee "${LOGS_DIR}/libcpp.txt"
    run_command cmake --build . "${CMAKE_BUILD_ARGS[@]}" 2>&1 | tee -a "${LOGS_DIR}/libcpp.txt"
    run_command cmake --install . 2>&1 | tee -a "${LOGS_DIR}/libcpp.txt"
    export LD_LIBRARY_PATH="${ATFL_DIR}/lib:${ATFL_DIR}/lib/${ATFL_TARGET_TRIPLE}:${LD_LIBRARY_PATH}"
    run_test_command "${LOGS_DIR}/check_cxx.xml" "${LOGS_DIR}/libcpp.txt" check-cxx
    run_test_command "${LOGS_DIR}/check_cxxabi.xml" "${LOGS_DIR}/libcpp.txt" check-cxxabi
}

product_build() {
    local extra_flags=()
    if ! bolttests_present; then
        echo "Bolt tests not present, external Bolt tests will not be executed."
    else
        extra_flags+=(
            -DLLVM_EXTERNAL_PROJECTS=bolttests
            "-DLLVM_EXTERNAL_BOLTTESTS_SOURCE_DIR=${BOLTTESTS_DIR}"
        )
    fi
    if [[ "${RELEASE_FLAGS}" == "true" ]]; then
        extra_flags+=(-DLLVM_APPEND_VC_REV=OFF)
    else
        extra_flags+=(-DLLVM_APPEND_VC_REV=ON)
    fi

    mkdir -p "${BUILD_DIR}/stage/product_build"
    cd "${BUILD_DIR}/stage/product_build"
    bootstrap_compiler_default_config

    local libs="-L${ATFL_DIR}/lib -rtlib=compiler-rt -unwindlib=libunwind -Wl,--as-needed -stdlib=libc++ ${COMMON_LINKER_FLAGS}"
    local cmake_caches="${BUILD_DIR}/stage/product_build/cmake_caches"

    mkdir -p "${cmake_caches}"
    cp "${SOURCES_DIR}/clang/cmake/caches/BOLT.cmake" "${cmake_caches}"
    { print_forced_cmake_flags_cache "COMMON_CMAKE_FLAGS"
      print_forced_cmake_flags_cache "USE_RPATH_CMAKE_FLAGS"
      print_forced_cmake_flags_cache "COMPILER_CMAKE_FLAGS"
      print_forced_cmake_flags_cache "COMPILER_RT_CMAKE_FLAGS"
      print_forced_cmake_flags_cache "LIBOMP_SHARED_CMAKE_FLAGS"
      print_forced_cmake_flags_cache "LIBUNWIND_SHARED_CMAKE_FLAGS"
      print_forced_cached_flag "CMAKE_EXE_LINKER_FLAGS:STRING" "\"${libs} ${RELOCS_LINKER_FLAGS}\""
      print_forced_cached_flag "CMAKE_MODULE_LINKER_FLAGS:STRING" "\"${libs} ${RELOCS_LINKER_FLAGS}\""
      print_forced_cached_flag "CMAKE_SHARED_LINKER_FLAGS:STRING" "\"${libs} ${RELOCS_LINKER_FLAGS}\""
      print_forced_cached_flag "LLVM_ENABLE_RUNTIMES:STRING" "\"compiler-rt;libunwind;openmp\""
      print_forced_cached_flag "RUNTIMES_CMAKE_ARGS:STRING" "\"-DCMAKE_C_COMPILER=${ATFL_DIR}/bin/clang;-DCMAKE_CXX_COMPILER=${ATFL_DIR}/bin/clang++;-DCMAKE_Fortran_COMPILER=${ATFL_DIR}/bin/flang;-DCMAKE_CXX_FLAGS=-stdlib++-isystem${ATFL_DIR}/include/c++/v1 -D_LIBCPP_VERBOSE_ABORT_NOT_NOEXCEPT;-DCMAKE_EXE_LINKER_FLAGS=${libs};-DCMAKE_MODULE_LINKER_FLAGS=${libs};-DCMAKE_SHARED_LINKER_FLAGS=${libs}\""
    } >> ${cmake_caches}/BOLT.cmake

    if [[ "${RELEASE_FLAGS}" == "true" ]]; then
        print_forced_cached_flag "LLVM_APPEND_VC_REV:BOOL" OFF >> ${cmake_caches}/BOLT.cmake
    else
        print_forced_cached_flag "LLVM_APPEND_VC_REV:BOOL" ON >> ${cmake_caches}/BOLT.cmake
    fi
    mkdir -p tools/clang/stage2-instrumented-bins/lib
    cp -d "${BUILD_DIR}"/stage/libcpp_build/lib/lib* tools/clang/stage2-instrumented-bins/lib

    { print_forced_cmake_flags_cache "COMMON_CMAKE_FLAGS"
      print_forced_cmake_flags_cache "USE_BOOTSTRAP_CMAKE_FLAGS"
      print_forced_cmake_flags_cache "USE_RPATH_CMAKE_FLAGS"
      print_forced_cmake_flags_cache "COMPILER_CMAKE_FLAGS"
      print_forced_cmake_flags_cache "COMPILER_RT_CMAKE_FLAGS"
      print_forced_cmake_flags_cache "FLANG_RT_CMAKE_FLAGS"
      print_forced_cmake_flags_cache "LIBOMP_SHARED_CMAKE_FLAGS"
      print_forced_cmake_flags_cache "LIBUNWIND_SHARED_CMAKE_FLAGS"
      print_forced_cmake_flags_cache "BOLT_CMAKE_FLAGS"
    } > flags.cmake

    run_command cmake "${CMAKE_ARGS[@]}" -G Ninja "${SOURCES_DIR}/llvm" \
        -DPGO_BUILD_CONFIGURATION="${cmake_caches}/BOLT.cmake" \
        -C "${SOURCES_DIR}/clang/cmake/caches/BOLT-PGO.cmake" \
        -C flags.cmake \
        -DBUILD_SHARED_LIBS=False \
        -DCMAKE_BUILD_TYPE=Release \
        -DCMAKE_EXE_LINKER_FLAGS="${libs}" \
        -DCMAKE_MODULE_LINKER_FLAGS="${libs}" \
        -DCMAKE_SHARED_LINKER_FLAGS="${libs}" \
        -DLLVM_BUILD_DOCS=ON \
        -DLLVM_ENABLE_SPHINX=ON \
        -DSPHINX_WARNINGS_AS_ERRORS=OFF \
        -DLLVM_ENABLE_PROJECTS="llvm;clang;flang;bolt;lld" \
        -DLLVM_ENABLE_RUNTIMES="compiler-rt;flang-rt;libunwind;openmp" \
        -DRUNTIMES_CMAKE_ARGS="-DCMAKE_CXX_FLAGS=-stdlib++-isystem${ATFL_DIR}/include/c++/v1 -D_LIBCPP_VERBOSE_ABORT_NOT_NOEXCEPT;-DCMAKE_EXE_LINKER_FLAGS=${libs};-DCMAKE_MODULE_LINKER_FLAGS=${libs};-DCMAKE_SHARED_LINKER_FLAGS=${libs}" \
        "${extra_flags[@]}" 2>&1 | tee "${LOGS_DIR}/product.txt"
    run_command cmake --build . "${CMAKE_BUILD_ARGS[@]}" 2>&1 | tee -a "${LOGS_DIR}/product.txt"
    run_command cmake --install . 2>&1 | tee -a "${LOGS_DIR}/product.txt"
    cp -d "${ATFL_DIR}"/lib/clang/*/lib/"${ATFL_TARGET_TRIPLE}"/libflang_rt* \
        "${ATFL_DIR}/lib/${ATFL_TARGET_TRIPLE}"
    if [[ "${ATFL_BOLTED}" == "ON" ]]; then
       run_command ninja "${NINJA_ARGS[@]}" stage2-clang-bolt 2>&1 | tee "${LOGS_DIR}/product-bolted.txt"
    fi
    echo "-Wl,-rpath=${ATFL_DIR}/lib" >> "${BUILD_DIR}"/bootstrap_compiler/bin/clang++.cfg
    run_test_command "${LOGS_DIR}/product_check_all.xml" "${LOGS_DIR}/product.txt" check-all
    if [[ "${ATFL_BOLTED}" == "ON" ]]; then
      run_test_command "${LOGS_DIR}/product_bolted_check_clang.xml" "${LOGS_DIR}/product-bolted.txt" stage2-check-clang
      # We insist that clang and flang are symlinks. This should fail if they are not.
      local clang_name="$(readlink "${ATFL_DIR}/bin/clang")"
      local flang_name="$(readlink "${ATFL_DIR}/bin/flang")"
      mv "${ATFL_DIR}/bin/${clang_name}" "${ATFL_DIR}/bin/${clang_name}.not_bolted"
      cp "${BUILD_DIR}/stage/product_build/tools/clang/stage2-instrumented-bins/tools/clang/stage2-bins/bin/${clang_name}" "${ATFL_DIR}/bin"
    fi

    bootstrap_compiler_default_config
}

static_libomp_build() {
    mkdir -p "${BUILD_DIR}/stage/static_libomp_build"
    cd "${BUILD_DIR}/stage/static_libomp_build"
    bootstrap_compiler_default_config

    { print_forced_cmake_flags_cache "COMMON_CMAKE_FLAGS"
      print_forced_cmake_flags_cache "USE_BOOTSTRAP_CMAKE_FLAGS"
      print_forced_cmake_flags_cache "SKIP_RPATH_CMAKE_FLAGS"
      print_forced_cmake_flags_cache "LIBOMP_NOSHARED_CMAKE_FLAGS"
    } > flags.cmake

    local libs="-L${ATFL_DIR}/lib -rtlib=compiler-rt -Wl,--as-needed ${COMMON_LINKER_FLAGS}"
    run_command cmake "${CMAKE_ARGS[@]}" -G Ninja "${SOURCES_DIR}/runtimes" \
        -C flags.cmake \
        -DBUILD_SHARED_LIBS=False \
        -DLLVM_ENABLE_RUNTIMES="openmp" \
        -DCMAKE_BUILD_TYPE=Release \
        -DCMAKE_Fortran_COMPILER="${ATFL_DIR}/bin/flang" \
        -DCMAKE_LINKER="${ATFL_DIR}/bin/ld.lld" \
        -DCMAKE_CXX_FLAGS="-stdlib++-isystem${ATFL_DIR}/include/c++/v1 -D_LIBCPP_VERBOSE_ABORT_NOT_NOEXCEPT" \
        -DCMAKE_EXE_LINKER_FLAGS="${libs}" \
        -DCMAKE_MODULE_LINKER_FLAGS="${libs}" \
        -DCMAKE_SHARED_LINKER_FLAGS="${libs}" \
        -DOPENMP_TEST_C_COMPILER="${ATFL_DIR}/bin/clang" \
        -DOPENMP_TEST_CXX_COMPILER="${ATFL_DIR}/bin/clang++" \
        -DOPENMP_TEST_Fortran_COMPILER="${ATFL_DIR}/bin/flang" \
        -DOPENMP_LLVM_LIT_EXECUTABLE="${BUILD_DIR}/stage/product_build/bin/llvm-lit" \
        -DOPENMP_FILECHECK_EXECUTABLE="${BUILD_DIR}/stage/product_build/bin/FileCheck" \
        2>&1 | tee "${LOGS_DIR}/static_libomp.txt"
    run_command cmake --build . "${CMAKE_BUILD_ARGS[@]}" 2>&1 | tee -a "${LOGS_DIR}/static_libomp.txt"
    rm -rf "${ATFL_DIR}.keep" "${ATFL_DIR}.libs"
    mv "${ATFL_DIR}" "${ATFL_DIR}.keep"
    run_command cmake --install . 2>&1 | tee -a "${LOGS_DIR}/static_libomp.txt"
    mv "${ATFL_DIR}" "${ATFL_DIR}.libs"
    mv "${ATFL_DIR}.keep" "${ATFL_DIR}"
    cp "${ATFL_DIR}".libs/lib/lib*.a \
        "${ATFL_DIR}/lib/${ATFL_TARGET_TRIPLE}"
    rm -r "${ATFL_DIR}.libs"
    run_test_command "${LOGS_DIR}/check_openmp.xml" "${LOGS_DIR}/static_libomp.txt" check-openmp
}

check_lit_xml_results() {
    local failed=false
    local result_file
    local result_files=(
        "${LOGS_DIR}/bootstrap_check_all.xml"
        "${LOGS_DIR}/check_cxx.xml"
        "${LOGS_DIR}/check_cxxabi.xml"
        "${LOGS_DIR}/product_check_all.xml"
        # OpenMP tests do not get executed currently
        # "${LOGS_DIR}/check_openmp.xml"
    )
    if [[ "${ATFL_BOLTED}" == "ON" ]]; then
        result_files+=(
            "${LOGS_DIR}/product_bolted_check_flang.xml"
            "${LOGS_DIR}/product_bolted_check_clang.xml"
        )
    fi

    echo_bold "Checking lit XML test results...."
    for result_file in "${result_files[@]}"; do
        if [[ ! -f "${result_file}" ]]; then
            echo "Expected lit XML result file was not created: ${result_file}" >&2
            failed=true
            continue
        fi

        if grep -Eq '<failure([ >])| failures="[1-9][0-9]*"| errors="[1-9][0-9]*"' "${result_file}"; then
            echo "lit reported failures in: ${result_file}" >&2
            failed=true
        fi
    done

    if ${failed}; then
        return 1
    fi
    echo_bold "Checking lit XML test results....done"
}

package() {
    cp "${SOURCES_DIR}/LICENSE.TXT" "${ATFL_DIR}/LICENSE.TXT"
    cp "${CHANGELOG_MD_PATH}" "${ATFL_DIR}/CHANGELOG.md"
    cp "${SBOM_FILE_PATH}" "${ATFL_DIR}/ATfL-SBOM.spdx.json"
    mkdir -p "${ATFL_DIR}/arm"
    cp "${MKMODULEDIRS_PATH}" "${ATFL_DIR}/arm/mkmoduledirs.sh"
    mkdir -p "${ATFL_DIR}/docs"
    cp "${DOCS_DIR}"/*.md "${ATFL_DIR}/docs"
    sed -i "s/%ATFL_VERSION%/${TOOLCHAIN_VERSION}/g" "${ATFL_DIR}/arm/mkmoduledirs.sh"
    sed -i "s/%ATFL_BUILD%/${BUILD_NUMBER:-"unknown"}/g" "${ATFL_DIR}/arm/mkmoduledirs.sh"
    sed -i "s/%ATFL_INSTALL_PREFIX%/\$\(dirname \$\(dirname \`realpath \$BASH_SOURCE\`\)\)/g" "${ATFL_DIR}/arm/mkmoduledirs.sh"
    chmod 0755 "${ATFL_DIR}"/arm/mkmoduledirs.sh
    if ! libraries_present; then
      echo "The Amath libraries will not be packaged."
    else
      cp "${LIBRARIES_DIR}/libamath.a" \
          "${ATFL_DIR}/lib/${ATFL_TARGET_TRIPLE}"
      cp "${LIBRARIES_DIR}/libamath.so" \
          "${ATFL_DIR}/lib/${ATFL_TARGET_TRIPLE}"
    fi
    if compgen -G "${ATFL_DIR}/include/flang/omp*" >/dev/null; then
      # Handle the old directory layout
      cp "${ATFL_DIR}"/include/flang/omp* "${ATFL_DIR}/include"
    else
      # Handle the new directory layout
      cp "${ATFL_DIR}"/lib/clang/*/include/omp* "${ATFL_DIR}/include"
      cp "${ATFL_DIR}"/lib/clang/*/finclude/flang/"${ATFL_TARGET_TRIPLE}"/omp* "${ATFL_DIR}/include"
    fi
    if ! compgen -G "${ATFL_DIR}/include/omp*.h" >/dev/null; then
      echo "The OpenMP headers could not be copied to ${ATFL_DIR}/include"
      exit 1
    fi
    if ! compgen -G "${ATFL_DIR}/include/omp*.mod" >/dev/null; then
      echo "The OpenMP Fortran modules could not be copied to ${ATFL_DIR}/include"
      exit 1
    fi
    cp "${ATFL_DIR}/share/man/man1/clang.1" "${ATFL_DIR}/share/man/man1/armclang.1"
    sed -i "s/clang /armclang /g" "${ATFL_DIR}/share/man/man1/armclang.1"
    sed -i "s/Bclang/Barmclang/g" "${ATFL_DIR}/share/man/man1/armclang.1"
    sed -i "s/CLANG/ARMCLANG/g" "${ATFL_DIR}/share/man/man1/armclang.1"
    sed -i "s/\"Clang\"/\"Armclang\"/g" "${ATFL_DIR}/share/man/man1/armclang.1"
    sed -i "s/Xarmclang/Xclang/g" "${ATFL_DIR}/share/man/man1/armclang.1"
    cp "${ATFL_DIR}/share/man/man1/flang.1" "${ATFL_DIR}/share/man/man1/armflang.1"
    sed -i "s/flang /armflang /g" "${ATFL_DIR}/share/man/man1/armflang.1"
    sed -i "s/Bflang/Barmflang/g" "${ATFL_DIR}/share/man/man1/armflang.1"
    sed -i "s/FLANG/ARMFLANG/g" "${ATFL_DIR}/share/man/man1/armflang.1"
    sed -i "s/\"Flang\"/\"Armflang\"/g" "${ATFL_DIR}/share/man/man1/armflang.1"
    sed -i "s/Xarmflang/Xflang/g" "${ATFL_DIR}/share/man/man1/armflang.1"

    echo "export PATH=\"\$(dirname \"\$(realpath \"\${BASH_SOURCE[0]}\")\")/bin:\${PATH}\"" >"${ATFL_DIR}/env.bash"
    echo "export MANPATH=\"\$(dirname \"\$(realpath \"\${BASH_SOURCE[0]}\")\")/share/man:\${MANPATH:-}\"" >>"${ATFL_DIR}/env.bash"
    echo "export PS1=\"(ATfL ${TOOLCHAIN_VERSION}) \$PS1\"" >>"${ATFL_DIR}/env.bash"
    cd "${ATFL_DIR}/bin"
    ln -sf clang armclang
    ln -sf clang++ armclang++
    ln -sf flang armflang
    ln -sf llvm-objdump armllvm-objdump
    if ! libraries_present; then
      echo "-mllvm -gvn-add-phi-translation=1 -mllvm -store-to-load-forwarding-conflict-detection=0 -mllvm -use-dereferenceable-at-point-semantics=false" > atfl-performance.cfg
    else
      echo "-fveclib=ArmPL -mllvm -gvn-add-phi-translation=1 -mllvm -store-to-load-forwarding-conflict-detection=0 -mllvm -use-dereferenceable-at-point-semantics=false" > atfl-performance.cfg
    fi
    echo "-frtlib-add-rpath @atfl-performance.cfg" > clang.cfg
    echo "-frtlib-add-rpath @atfl-performance.cfg" > clang++.cfg
    echo "-frtlib-add-rpath @atfl-performance.cfg" > flang.cfg
    cd -
{
    echo "complete -F _clang armclang"
    echo "complete -F _clang armclang++"
    echo "complete -F _clang armflang"
} >> "${ATFL_DIR}"/share/clang/bash-autocomplete.sh
    run_command tar --owner=root --group=root -czf "${OUTPUT_DIR}/${TAR_NAME}" -C "${BUILD_DIR}" atfl |
        tee "${LOGS_DIR}/package.txt"
}

################
## Main Logic ##
################

BUILD_SH_STATUS=0
main() {
    local test_status=0

    echo_bold "Patching sources for ATfL...."
    apply_patches
    echo_bold "Done"
    echo_bold "Executing build stages...."
    for stage in "${STAGES[@]}"; do
        echo_bold "Executing stage: ${stage}...."
        ${stage}
        echo_bold "Completed stage: ${stage}."
    done
    echo_bold "Executed build stages."
    if ! check_lit_xml_results; then
        test_status=2
    fi
    echo_bold "Packaging...."
    package
    echo_bold "Packaged."
    if [[ "${test_status}" -ne 0 ]]; then
        echo "ATfL build completed, but lit tests failed." >&2
        BUILD_SH_STATUS="${test_status}"
    fi
    echo_bold "Done."
}

trap 'abort' 0
cd "${BASE_DIR}"
if [[ $# -gt 0 ]]; then
    case "$1" in
    -h | --help)
        print_help
        trap : 0
        exit 0
        ;;
    -i | --interactive)
        INTERACTIVE=true
        ;;
    *)
        echo "Unknown option: $1"
        print_help
        exit 1
        ;;
    esac
fi

if ! [[ -f "${CHANGELOG_MD_PATH}" ]]
then
  echo "The path to CHANGELOG.md file is configured incorrectly or does not exist."
  exit 1
fi

if ! [[ -f "${SBOM_FILE_PATH}" ]]
then
  echo "The path to SBOM JSON file is configured incorrectly or does not exist."
  exit 1
fi

if ! [[ -f "${MKMODULEDIRS_PATH}" ]]
then
  echo "The path to mkmoduledirs.sh.var file is configured incorrectly or does not exist."
  exit 1
fi

if ! [[ -e "${DOCS_DIR}" ]]
then
  echo "The documentation directory is configured incorrectly or does not exist."
  exit 1
fi

if ! [[ -e "${SOURCES_DIR}" ]]
then
  echo "The sources directory is configured incorrectly or does not exist."
  exit 1
fi

if ! [[ -e "${ZLIB_STATIC_PATH}" ]]
then
  echo "The path to libz.a file is configured incorrectly or does not exist."
  exit 1
fi

if ! [[ -x "/usr/bin/zdump" ]]
then
  echo "The zdump executable file is not present in /usr/bin, you must be building on non-Debian/Ubuntu Linux system."
  echo "Since the check-all testing relies on this debianism, consider copying /usr/sbin/zdump to /usr/bin."
  exit 1
fi

make_and_clean_directory() {
    local dir="$1"
    mkdir -p "${dir}"

    # Initial clean-up of directory contents. Directory itself may be mounted.
    find "${dir}" -mindepth 1 -maxdepth 1 -depth -exec rm -rf -- {} +
}

make_and_clean_directory "${BUILD_DIR:?}"
make_and_clean_directory "${OUTPUT_DIR:?}"
make_and_clean_directory "${LOGS_DIR:?}"

# If a test fails, lit will ordinarily return a non-zero result,
# which prevents further testing. Setting the --ignore-fail option
# will cause testing to continue, so that CI systems can get a
# full set of results.
# The lit test suites do not generate xml results by default.
# This can be enabled with the --xunit-xml-output option.
# Each check target appends its own --xunit-xml-output path under LOGS_DIR.
export LIT_OPTS="${LIT_OPTS:+${LIT_OPTS} }--ignore-fail"

main
trap : 0
if ${INTERACTIVE}; then
    bash
fi
exit "${BUILD_SH_STATUS}"
