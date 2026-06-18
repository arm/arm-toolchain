#
# Copyright (c) 2026, Arm Limited and affiliates.
# Part of the Arm Toolchain project, under the Apache License v2.0 with LLVM Exceptions.
# See https://llvm.org/LICENSE.txt for license information.
# SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
#

add_library(atomic-fallback STATIC
  atomic.cpp
)

target_compile_features(atomic-fallback PRIVATE cxx_std_17)
target_compile_options(atomic-fallback PRIVATE
  -fno-asynchronous-unwind-tables
  -fno-exceptions
  -fno-rtti
  -fno-unwind-tables
)
