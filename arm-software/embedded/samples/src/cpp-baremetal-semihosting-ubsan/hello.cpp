/* Copyright (c) 2023-2025, Arm Limited and affiliates. 
 *
 * Part of the Arm Toolchain project, under the Apache License v2.0 with LLVM Exceptions.
 * See https://llvm.org/LICENSE.txt for license information.
 * SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception */

#include <iostream>
#include <limits>

int main(void) {
  int max_int = std::numeric_limits<int>::max();
  [[maybe_unused]] int invoke_ubsan = max_int + max_int;

  std::cout << "C++ UBSan sample" << std::endl;

  return 0;
}
