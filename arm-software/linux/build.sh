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

RELEASE_FLAGS=${RELEASE_FLAGS:-"false"}
ATFL_VERSION=${ATFL_VERSION:-"0.0"}
OS_NAME=${OS_NAME:-"linux"}
TAR_NAME=${TAR_NAME:-"atfl-${ATFL_VERSION}-${OS_NAME}-`uname -m`.tar.gz"}
ATFL_ASSERTIONS=${ATFL_ASSERTIONS:-"ON"}
ATFL_TARGET_TRIPLE=${ATFL_TARGET_TRIPLE:-"`uname -m`-unknown-linux-gnu"}
ARM_TOOLCHAIN_ID=$(cmake -DLLVM_TOOLCHAIN_PROJECT_CODE=L -P ${SOURCES_DIR}/arm-software/shared/cmake/generate_toolchain_id.cmake)
PROCESSOR_COUNT=$(getconf _NPROCESSORS_ONLN)
PARALLEL_JOBS=${PARALLEL_JOBS:-"${PROCESSOR_COUNT}"}
# " <-- this is to help syntax highlighters to find a matching double quote
STAGES=(
    "bootstrap_compiler_build"
    "libcpp_build"
    "product_build"
    "shared_lib_build"
)
ZLIB_STATIC_PATH=${ZLIB_STATIC_PATH:-"/usr/lib/`uname -m`-linux-gnu/libz.a"}
COMMON_CMAKE_FLAGS=(
    ${COMMON_CMAKE_FLAGS}
    -DCLANG_ENABLE_LIBXML2=OFF
    -DLLVM_ENABLE_LIBXML2=OFF
    -DLLVM_ENABLE_ZLIB=ON
    -DLLVM_USE_STATIC_ZSTD=True
    -DLLVM_ENABLE_ZSTD=ON
    -DLLVM_BINUTILS_INCDIR=/usr/include
    -DLLVM_ENABLE_PIC=ON
    -DLLVM_ENABLE_ASSERTIONS="${ATFL_ASSERTIONS}"
    -DLLVM_ENABLE_FFI=OFF
    -DLLVM_ENABLE_BINDINGS=OFF
    -DLLVM_ENABLE_PLUGINS=ON
    -DLLVM_TOOL_LIBUNWIND_BUILD=ON
    -DLLVM_TARGETS_TO_BUILD=AArch64
    -DLLVM_DEFAULT_TARGET_TRIPLE=${ATFL_TARGET_TRIPLE}
    -DZLIB_LIBRARY_RELEASE=${ZLIB_STATIC_PATH}
)
PRODUCT_CMAKE_FLAGS=(
    -DCMAKE_C_COMPILER="${BUILD_DIR}/bootstrap_compiler/bin/clang"
    -DCMAKE_CXX_COMPILER="${BUILD_DIR}/bootstrap_compiler/bin/clang++"
    -DCMAKE_INSTALL_PREFIX="${ATFL_DIR}"
    -DLLVM_ENABLE_LLD=ON
)
COMPILER_CMAKE_FLAGS=(
    -DCMAKE_CXX_FLAGS="-stdlib++-isystem ${ATFL_DIR}/include/c++/v1 -D_LIBCPP_VERBOSE_ABORT_NOT_NOEXCEPT"
    -DCMAKE_EXE_LINKER_FLAGS="-L${ATFL_DIR}/lib -rtlib=compiler-rt -unwindlib=libunwind -Wl,--as-needed -stdlib=libc++"
    -DCMAKE_MODULE_LINKER_FLAGS="-L${ATFL_DIR}/lib -rtlib=compiler-rt -unwindlib=libunwind -Wl,--as-needed -stdlib=libc++"
    -DCMAKE_SHARED_LINKER_FLAGS="-L${ATFL_DIR}/lib -rtlib=compiler-rt -unwindlib=libunwind -Wl,--as-needed -stdlib=libc++"
    -DCMAKE_BUILD_TYPE=Release
    -DCMAKE_SKIP_RPATH=No
    -DCMAKE_SKIP_INSTALL_RPATH=No
    -DLLVM_BUILD_DOCS=ON
    -DLLVM_ENABLE_SPHINX=ON
    -DSPHINX_WARNINGS_AS_ERRORS=OFF
    -DLLVM_ENABLE_PROJECTS="llvm;clang;flang;lld"
    -DLLVM_ENABLE_RUNTIMES="compiler-rt;flang-rt;libunwind;openmp"
    -DCLANG_ENABLE_LIBXML2=OFF
    -DCLANG_PLUGIN_SUPPORT=ON
    -DCLANG_ENABLE_STATIC_ANALYZER=ON
    -DCLANG_TOOL_LIBCLANG_BUILD=ON
    -DLIBCLANG_BUILD_STATIC=ON
    -DCOMPILER_RT_DEFAULT_TARGET_ARCH=AArch64
    -DCOMPILER_RT_BUILD_SANITIZERS=OFF
    -DCOMPILER_RT_BUILD_LIBFUZZER=ON
    -DCOMPILER_RT_BUILD_ORC=OFF
    -DCOMPILER_RT_USE_LIBCXX=ON
    -DCOMPILER_RT_BUILD_BUILTINS=ON
    -DCOMPILER_RT_USE_BUILTINS_LIBRARY=ON
    -DCOMPILER_RT_EXCLUDE_ATOMIC_BUILTIN=OFF
    -DCOMPILER_RT_BUILD_STANDALONE_LIBATOMIC=OFF
    -DCOMPILER_RT_USE_ATOMIC_LIBRARY=ON
    -DCOMPILER_RT_USE_LLVM_UNWINDER=OFF
    -DCOMPILER_RT_LIBRARY_atomic_${ATFL_TARGET_TRIPLE}="-rtlib=compiler-rt"
    -DFLANG_RT_ENABLE_SHARED=ON
    -DFLANG_RT_ENABLE_STATIC=ON
    -DLIBOMP_COPY_EXPORTS=False
    -DLIBOMP_USE_HWLOC=False
    -DLIBOMP_OMPT_SUPPORT=ON
    -DLIBOMP_OMPD_GDB_SUPPORT=OFF
    -DARM_TOOLCHAIN_ID="${ARM_TOOLCHAIN_ID}"
    -DCLANG_VENDOR="Arm Toolchain for Linux ${ATFL_VERSION}"
    -DFLANG_VENDOR="Arm Toolchain for Linux ${ATFL_VERSION}"
    -DLLVM_VERSION_SUFFIX=""
)
LIBUNWIND_SHARED_CMAKE_FLAGS=(
    -DLIBUNWIND_USE_COMPILER_RT=ON
    -DLIBUNWIND_ENABLE_SHARED=ON
    -DLIBUNWIND_ENABLE_STATIC=ON
    -DLIBUNWIND_ENABLE_ASSERTIONS=OFF
    -DLIBUNWIND_ENABLE_THREADS=ON
)
LIBUNWIND_NOSHARED_CMAKE_FLAGS=(
    -DLIBUNWIND_USE_COMPILER_RT=ON
    -DLIBUNWIND_ENABLE_SHARED=OFF
    -DLIBUNWIND_ENABLE_STATIC=ON
    -DLIBUNWIND_ENABLE_ASSERTIONS=OFF
    -DLIBUNWIND_ENABLE_THREADS=ON
)

CMAKE_ARGS=""
CMAKE_BUILD_ARGS="-j${PARALLEL_JOBS}"
NINJA_ARGS="-j${PARALLEL_JOBS}"
if [[ "${TRACE-0}" == "1" ]]; then
    run_command CMAKE_ARGS="${CMAKE_ARGS} --trace-expand"
    COMMON_CMAKE_FLAGS="${COMMON_CMAKE_FLAGS} -DCMAKE_VERBOSE_MAKEFILE=ON"
    run_command CMAKE_BUILD_ARGS="${CMAKE_BUILD_ARGS} -v"
    NINJA_ARGS="${NINJA_ARGS} -v"
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
    echo "With: PATH=\"$PATH\" LD_LIBRARY_PATH=\"$LD_LIBRARY_PATH\""
    echo "Running: $*"
    "$@"
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
                        (default: $SOURCES_DIR)
    LIBRARIES_DIR       The optional directory where the ArmPL veclibs will be stored
                        (default: $LIBRARIES_DIR)
    PATCHES_DIR         The optional directory where all patches will be stored
                        (default: $PATCHES_DIR)
    DOCS_DIR            The directory where ATfL documents will be stored
                        (default: $DOCS_DIR)
    BUILD_DIR           The directory where all build output will be stored
                        (default: $BUILD_DIR)
    LOGS_DIR            The directory where all build logs will be stored
                        (default: $LOGS_DIR)
    OUTPUT_DIR          The directory where all build output will be stored
                        (default: $OUTPUT_DIR)
    RELEASE_FLAGS       Enable release flags in the build true/false
                        (default: $RELEASE_FLAGS)
    PARALLEL_JOBS       The number of parallel jobs to run during the build
                        (default: $PARALLEL_JOBS)
    ATFL_ASSERTIONS     Enable assertions in the build ON/OFF
                        (default: $ATFL_ASSERTIONS)
    ATFL_VERSION        Specify the version string
                        (default: $ATFL_VERSION)
    ATFL_TARGET_TRIPLE  Specify the default target triple
                        (default: $ATFL_TARGET_TRIPLE)
    OS_NAME             Specify the OS name
                        (default: $OS_NAME)
    TAR_NAME            The name of the tarball to be created
                        (default: $TAR_NAME)
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

bootstrap_compiler_build() {
    mkdir -p "${BUILD_DIR}/stage/bootstrap_compiler"
    cd "${BUILD_DIR}/stage/bootstrap_compiler"

    run_command cmake ${CMAKE_ARGS} -G Ninja "${SOURCES_DIR}/llvm" \
        -DBUILD_SHARED_LIBS=False \
        -DCMAKE_BUILD_TYPE=Release \
        -DCMAKE_SKIP_RPATH=No \
        -DCMAKE_SKIP_INSTALL_RPATH=No \
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
        "${COMMON_CMAKE_FLAGS[@]}" "${LIBUNWIND_NOSHARED_CMAKE_FLAGS[@]}" 2>&1 |
        tee "${LOGS_DIR}/bootstrap_compiler.txt"
    run_command cmake --build . ${CMAKE_BUILD_ARGS} 2>&1 | tee -a "${LOGS_DIR}/bootstrap_compiler.txt"
    run_command cmake --install . 2>&1 | tee -a "${LOGS_DIR}/bootstrap_compiler.txt"
    export PATH="${BUILD_DIR}/bootstrap_compiler/bin:$PATH"
    echo "-fuse-ld=lld" >${BUILD_DIR}/bootstrap_compiler/bin/clang.cfg
    echo "-fuse-ld=lld" >${BUILD_DIR}/bootstrap_compiler/bin/clang++.cfg
    run_command ninja ${NINJA_ARGS} check-all 2>&1 | tee -a "${LOGS_DIR}/bootstrap_compiler.txt"
}

libcpp_build() {
    mkdir -p "${BUILD_DIR}/stage/libcpp_build"
    cd "${BUILD_DIR}/stage/libcpp_build"
    run_command cmake ${CMAKE_ARGS} -G Ninja "${SOURCES_DIR}/runtimes" \
        -DBUILD_SHARED_LIBS=False \
        -DCMAKE_CXX_FLAGS="-D_LIBCPP_VERBOSE_ABORT_NOT_NOEXCEPT" \
        -DCMAKE_EXE_LINKER_FLAGS="-rtlib=compiler-rt -unwindlib=libunwind -Wl,--as-needed" \
        -DCMAKE_MODULE_LINKER_FLAGS="-rtlib=compiler-rt -unwindlib=libunwind -Wl,--as-needed" \
        -DCMAKE_SHARED_LINKER_FLAGS="-rtlib=compiler-rt -unwindlib=libunwind -Wl,--as-needed" \
        -DCMAKE_SKIP_RPATH=Yes \
        -DCMAKE_SKIP_INSTALL_RPATH=Yes \
        -DCMAKE_BUILD_TYPE=RelWithDebInfo \
        -DLLVM_ENABLE_RUNTIMES="libcxx;libcxxabi;libunwind" \
        -DLIBCXXABI_USE_COMPILER_RT=ON \
        -DLIBCXXABI_USE_LLVM_UNWINDER=ON \
        -DLIBCXXABI_ENABLE_STATIC_UNWINDER=ON \
        -DLIBCXXABI_ENABLE_EXCEPTIONS=ON \
        -DLIBCXXABI_ENABLE_ASSERTIONS=OFF \
        -DLIBCXXABI_ENABLE_SHARED=ON \
        -DLIBCXXABI_ENABLE_STATIC=ON \
        -DLIBCXXABI_ENABLE_THREADS=ON \
        -DLIBCXXABI_HAS_EXTERNAL_THREAD_API=OFF \
        -DLIBCXX_CXX_ABI="libcxxabi" \
        -DLIBCXX_USE_COMPILER_RT=ON \
        -DLIBCXX_ENABLE_EXCEPTIONS=ON \
        -DLIBCXX_ENABLE_ASSERTIONS=OFF \
        -DLIBCXX_ENABLE_SHARED=ON \
        -DLIBCXX_ENABLE_STATIC=ON \
        -DLIBCXX_ENABLE_THREADS=ON \
        -DLIBCXX_HAS_EXTERNAL_THREAD_API=OFF \
        -DLIBCXX_ENABLE_LOCALIZATION=ON \
        -DLIBCXX_ENABLE_TIME_ZONE_DATABASE=OFF \
        -DLIBCXX_ENABLE_UNICODE=ON \
        -DLIBCXX_ENABLE_WIDE_CHARACTERS=ON \
        "${COMMON_CMAKE_FLAGS[@]}" "${PRODUCT_CMAKE_FLAGS[@]}" "${LIBUNWIND_NOSHARED_CMAKE_FLAGS[@]}" 2>&1 |
        tee "${LOGS_DIR}/libcpp.txt"
    run_command cmake --build . ${CMAKE_BUILD_ARGS} 2>&1 | tee -a "${LOGS_DIR}/libcpp.txt"
    run_command cmake --install . 2>&1 | tee -a "${LOGS_DIR}/libcpp.txt"
    export LD_LIBRARY_PATH="${ATFL_DIR}/lib:$LD_LIBRARY_PATH"
    run_command ninja ${NINJA_ARGS} check-cxx 2>&1 | tee -a "${LOGS_DIR}/libcpp.txt"
    run_command ninja ${NINJA_ARGS} check-cxxabi 2>&1 | tee -a "${LOGS_DIR}/libcpp.txt"
}

product_build() {
    local extra_flags=""
    if [[ "${RELEASE_FLAGS}" == "true" ]]; then
        extra_flags="-DLLVM_APPEND_VC_REV=OFF"
    else
        extra_flags="-DLLVM_APPEND_VC_REV=ON"
    fi

    mkdir -p "${BUILD_DIR}/stage/product_build"
    cd "${BUILD_DIR}/stage/product_build"
    run_command cmake ${CMAKE_ARGS} -G Ninja "${SOURCES_DIR}/llvm" \
        -DBUILD_SHARED_LIBS=False \
        -DLIBOMP_ENABLE_SHARED=True \
        -DRUNTIMES_CMAKE_ARGS="-DCMAKE_CXX_FLAGS=-stdlib++-isystem${ATFL_DIR}/include/c++/v1 -D_LIBCPP_VERBOSE_ABORT_NOT_NOEXCEPT;-DCMAKE_EXE_LINKER_FLAGS=-L${ATFL_DIR}/lib -rtlib=compiler-rt -unwindlib=libunwind -Wl,--as-needed -stdlib=libc++;-DCMAKE_MODULE_LINKER_FLAGS=-L${ATFL_DIR}/lib -rtlib=compiler-rt -unwindlib=libunwind -Wl,--as-needed -stdlib=libc++;-DCMAKE_SHARED_LINKER_FLAGS=-L${ATFL_DIR}/lib  -rtlib=compiler-rt -unwindlib=libunwind -Wl,--as-needed -stdlib=libc++" \
        "${COMMON_CMAKE_FLAGS[@]}" "${PRODUCT_CMAKE_FLAGS[@]}" "${COMPILER_CMAKE_FLAGS[@]}" "${LIBUNWIND_SHARED_CMAKE_FLAGS[@]}" ${extra_flags} 2>&1 |
        tee "${LOGS_DIR}/product.txt"
    run_command cmake --build . ${CMAKE_BUILD_ARGS} 2>&1 | tee -a "${LOGS_DIR}/product.txt"
    run_command cmake --install . 2>&1 | tee -a "${LOGS_DIR}/product.txt"
    run_command ninja ${NINJA_ARGS} check-all | tee -a "${LOGS_DIR}/product.txt"
}

shared_lib_build() {
    local extra_flags=""
    if [[ "${RELEASE_FLAGS}" == "true" ]]; then
        extra_flags="-DLLVM_APPEND_VC_REV=OFF"
    else
        extra_flags="-DLLVM_APPEND_VC_REV=ON"
    fi

    mkdir -p "${BUILD_DIR}/stage/shared_lib_build"
    cd "${BUILD_DIR}/stage/shared_lib_build"
    run_command cmake ${CMAKE_ARGS} -G Ninja "${SOURCES_DIR}/llvm" \
        -DBUILD_SHARED_LIBS=True \
        -DLIBOMP_ENABLE_SHARED=False \
        -DRUNTIMES_CMAKE_ARGS="-DCMAKE_CXX_FLAGS=-stdlib++-isystem${ATFL_DIR}/include/c++/v1 -D_LIBCPP_VERBOSE_ABORT_NOT_NOEXCEPT;-DCMAKE_EXE_LINKER_FLAGS=-L${ATFL_DIR}/lib -rtlib=compiler-rt -unwindlib=libunwind -Wl,--as-needed -stdlib=libc++;-DCMAKE_MODULE_LINKER_FLAGS=-L${ATFL_DIR}/lib -rtlib=compiler-rt -unwindlib=libunwind -Wl,--as-needed -stdlib=libc++;-DCMAKE_SHARED_LINKER_FLAGS=-L${ATFL_DIR}/lib  -rtlib=compiler-rt -unwindlib=libunwind -Wl,--as-needed -stdlib=libc++" \
        "${COMMON_CMAKE_FLAGS[@]}" -DLLVM_ENABLE_ZSTD=OFF "${PRODUCT_CMAKE_FLAGS[@]}" "${COMPILER_CMAKE_FLAGS[@]}" -DLIBOMP_OMPT_SUPPORT=OFF "${LIBUNWIND_SHARED_CMAKE_FLAGS[@]}" ${extra_flags} 2>&1 |
        tee "${LOGS_DIR}/shared_lib.txt"
    run_command cmake --build . ${CMAKE_BUILD_ARGS} 2>&1 | tee -a "${LOGS_DIR}/shared_lib.txt"
    rm -rf "${ATFL_DIR}.keep" "${ATFL_DIR}.libs"
    mv "${ATFL_DIR}" "${ATFL_DIR}.keep"
    run_command cmake --install . 2>&1 | tee -a "${LOGS_DIR}/shared_lib.txt"
    mv "${ATFL_DIR}" "${ATFL_DIR}.libs"
    mv "${ATFL_DIR}.keep" "${ATFL_DIR}"
    cp "${ATFL_DIR}.libs/lib/${ATFL_TARGET_TRIPLE}/libomp.a" \
        "${ATFL_DIR}/lib/${ATFL_TARGET_TRIPLE}"
    cp -d ${ATFL_DIR}.libs/lib/clang/*/lib/${ATFL_TARGET_TRIPLE}/libflang_rt* \
        "${ATFL_DIR}/lib/${ATFL_TARGET_TRIPLE}"
    rm -r "${ATFL_DIR}.libs"
    echo '-L<CFGDIR>/../runtimes/runtimes-bins/openmp/runtime/src $-Wl,--push-state $-Wl,--as-needed $-lomp $-ldl $-Wl,--pop-state' >bin/clang.cfg
    echo '-L<CFGDIR>/../runtimes/runtimes-bins/openmp/runtime/src $-Wl,--push-state $-Wl,--as-needed $-lomp $-ldl $-Wl,--pop-state' >bin/clang++.cfg
    run_command ninja ${NINJA_ARGS} check-all | tee -a "${LOGS_DIR}/shared_lib.txt"
}

package() {
    cp "${SOURCES_DIR}/LICENSE.TXT" "${ATFL_DIR}/LICENSE.TXT"
    cp "${CHANGELOG_MD_PATH}" "${ATFL_DIR}/CHANGELOG.md"
    cp "${SBOM_FILE_PATH}" "${ATFL_DIR}/ATfL-SBOM.spdx.json"
    mkdir -p "${ATFL_DIR}/arm"
    cp "${MKMODULEDIRS_PATH}" "${ATFL_DIR}/arm/mkmoduledirs.sh"
    mkdir -p "${ATFL_DIR}/docs"
    cp "${DOCS_DIR}"/*.md "${ATFL_DIR}/docs"
    sed -i "s/%ATFL_VERSION%/${ATFL_VERSION}/g" "${ATFL_DIR}/arm/mkmoduledirs.sh"
    sed -i "s/%ATFL_BUILD%/${BUILD_NUMBER:-"unknown"}/g" "${ATFL_DIR}/arm/mkmoduledirs.sh"
    sed -i "s/%ATFL_INSTALL_PREFIX%/\$\(dirname \$\(dirname \`realpath \$BASH_SOURCE\`\)\)/g" "${ATFL_DIR}/arm/mkmoduledirs.sh"
    chmod 0755 ${ATFL_DIR}/arm/mkmoduledirs.sh
    if ! libraries_present; then
      echo "The Amath libraries will not be packaged."
    else
      cp "${LIBRARIES_DIR}/libamath.a" \
          "${ATFL_DIR}/lib/${ATFL_TARGET_TRIPLE}"
      cp "${LIBRARIES_DIR}/libamath.so" \
          "${ATFL_DIR}/lib/${ATFL_TARGET_TRIPLE}"
    fi
    cp ${ATFL_DIR}/include/flang/omp* "${ATFL_DIR}/include"
    cp "${ATFL_DIR}/share/man/man1/clang.1" "${ATFL_DIR}/share/man/man1/armclang.1"
    sed -i "s/clang /armclang /g" "${ATFL_DIR}/share/man/man1/armclang.1"
    sed -i "s/Bclang/Barmclang/g" "${ATFL_DIR}/share/man/man1/armclang.1"
    sed -i "s/CLANG/ARMCLANG/g" "${ATFL_DIR}/share/man/man1/armclang.1"
    sed -i "s/\"Clang\"/\"Armclang\"/g" "${ATFL_DIR}/share/man/man1/armclang.1"
    cp "${ATFL_DIR}/share/man/man1/flang.1" "${ATFL_DIR}/share/man/man1/armflang.1"
    sed -i "s/^flang\ /armflang\ /g" "${ATFL_DIR}/share/man/man1/armflang.1"
    sed -i "s/\ flang\ /\ armflang\ /g" "${ATFL_DIR}/share/man/man1/armflang.1"
    sed -i "s/FLANG/ARMFLANG/g" "${ATFL_DIR}/share/man/man1/armflang.1"
    sed -i "s/\"Flang\"/\"Armflang\"/g" "${ATFL_DIR}/share/man/man1/armflang.1"
    echo 'export PATH="$(dirname `realpath $BASH_SOURCE`)/bin:$PATH"' >"${ATFL_DIR}/env.bash"
    echo 'export MANPATH="$(dirname `realpath $BASH_SOURCE`)/share/man:$MANPATH"' >>"${ATFL_DIR}/env.bash"
    echo "export PS1=\"(ATfL ${ATFL_VERSION}) \$PS1\"" >>"${ATFL_DIR}/env.bash"
    cd "${ATFL_DIR}/bin"
    ln -sf clang armclang
    ln -sf clang++ armclang++
    ln -sf flang armflang
    ln -sf llvm-objdump armllvm-objdump
    if ! libraries_present; then
      echo "-mllvm -gvn-add-phi-translation=1 -mllvm -store-to-load-forwarding-conflict-detection=0" > atfl-performance.cfg
    else
      echo "-fveclib=ArmPL -mllvm -gvn-add-phi-translation=1 -mllvm -store-to-load-forwarding-conflict-detection=0" > atfl-performance.cfg
    fi
    echo "-frtlib-add-rpath @atfl-performance.cfg" > clang.cfg
    echo "-frtlib-add-rpath @atfl-performance.cfg" > clang++.cfg
    echo "-frtlib-add-rpath @atfl-performance.cfg" > flang.cfg
    cd -
    echo "complete -F _clang armclang" >> ${ATFL_DIR}/share/clang/bash-autocomplete.sh
    echo "complete -F _clang armclang++" >> ${ATFL_DIR}/share/clang/bash-autocomplete.sh
    echo "complete -F _clang armflang" >> ${ATFL_DIR}/share/clang/bash-autocomplete.sh
    run_command tar --owner=root --group=root -czf "$OUTPUT_DIR/$TAR_NAME" -C "$BUILD_DIR" atfl |
        tee "${LOGS_DIR}/package.txt"
}

################
## Main Logic ##
################

main() {
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
    echo_bold "Packaging...."
    package
    echo_bold "Packaged."
    echo_bold "Preparing compilers.yaml..."
    cat <<SPACK_EOF >$OUTPUT_DIR/compilers.yaml
compilers:
- compiler:
    spec: arm@=${ATFL_VERSION}
    paths:
      cc: ${ATFL_DIR}/bin/armclang
      cxx: ${ATFL_DIR}/bin/armclang++
      f77: ${ATFL_DIR}/bin/armflang
      fc: ${ATFL_DIR}/bin/armflang
    flags:
      cflags: -Wno-error=implicit-function-declaration
      cxxflags: -Wno-error=implicit-function-declaration
    operating_system: $(source /etc/os-release && echo ${ID}${VERSION_ID})
    target: $(uname -m)
    modules: []
    environment: {}
    extra_rpaths: []
SPACK_EOF
    echo_bold "Done."
}

trap 'abort' 0
cd $BASE_DIR
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

mkdir -p "${BUILD_DIR}"
mkdir -p "${OUTPUT_DIR}"
mkdir -p "${LOGS_DIR}"

main
trap : 0
if ${INTERACTIVE}; then
    bash
fi
