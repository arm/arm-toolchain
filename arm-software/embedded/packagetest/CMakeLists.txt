if(LLVM_ENABLE_BACKTRACES)
  set(ENABLE_BACKTRACES 1)
endif()
llvm_canonicalize_cmake_booleans(
  ENABLE_BACKTRACES
)

# If samples are included in the package then use the packaged samples in
# post-package tests, otherwise use the samples from the source directory.
if(llvm-toolchain-samples IN_LIST LLVM_TOOLCHAIN_DISTRIBUTION_COMPONENTS)
    set(samples_dir ${unpack_directory}/samples)
else()
    set(samples_dir ${CMAKE_CURRENT_SOURCE_DIR}/../samples)
endif()

set(LLVM_TOOLCHAIN_PACKAGETEST_LIBC "")
configure_lit_site_cfg(
  ${CMAKE_CURRENT_SOURCE_DIR}/lit.site.cfg.py.in
  ${CMAKE_CURRENT_BINARY_DIR}/lit.site.cfg.py
  MAIN_CONFIG
  ${CMAKE_CURRENT_SOURCE_DIR}/lit.cfg.py
)

list(APPEND LLVM_TOOLCHAIN_PACKAGE_TEST_DEPS
  unpack-llvm-toolchain
  FileCheck
  count
  not
)

set(LLVM_TOOLCHAIN_PACKAGE_TEST_LIBCS "")
if(LLVM_TOOLCHAIN_ENABLE_PICOLIBC)
  list(APPEND LLVM_TOOLCHAIN_PACKAGE_TEST_LIBCS picolibc)
endif()
if(LLVM_TOOLCHAIN_ENABLE_LLVMLIBC)
  list(APPEND LLVM_TOOLCHAIN_PACKAGE_TEST_LIBCS llvmlibc)
endif()

set(LLVM_TOOLCHAIN_PACKAGE_TEST_DIRS "")
if(LLVM_TOOLCHAIN_PACKAGE_TEST_LIBCS)
  foreach(libc IN LISTS LLVM_TOOLCHAIN_PACKAGE_TEST_LIBCS)
    set(LLVM_TOOLCHAIN_PACKAGETEST_LIBC ${libc})
    set(packagetest_libc_dir ${CMAKE_CURRENT_BINARY_DIR}/${libc})
    configure_lit_site_cfg(
      ${CMAKE_CURRENT_SOURCE_DIR}/lit.site.cfg.py.in
      ${packagetest_libc_dir}/lit.site.cfg.py
      MAIN_CONFIG
      ${CMAKE_CURRENT_SOURCE_DIR}/lit.cfg.py
    )
    list(APPEND LLVM_TOOLCHAIN_PACKAGE_TEST_DIRS ${packagetest_libc_dir})
  endforeach()
else()
  list(APPEND LLVM_TOOLCHAIN_PACKAGE_TEST_DIRS ${CMAKE_CURRENT_BINARY_DIR})
endif()

add_lit_testsuite(check-package-llvm-toolchain
  "Running Arm Toolchain for Embedded package tests"
  ${LLVM_TOOLCHAIN_PACKAGE_TEST_DIRS}
  DEPENDS ${LLVM_TOOLCHAIN_PACKAGE_TEST_DEPS}
  ARGS -j 1
)

add_lit_testsuites(package-llvm-toolchain ${CMAKE_CURRENT_SOURCE_DIR}
  DEPENDS ${LLVM_TOOLCHAIN_PACKAGE_TEST_DEPS}
)
