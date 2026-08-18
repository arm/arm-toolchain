/* Copyright (c) 2024-2025, Arm Limited and affiliates. 
 *
 * Part of the Arm Toolchain project, under the Apache License v2.0 with LLVM Exceptions.
 * See https://llvm.org/LICENSE.txt for license information.
 * SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception */

#include <iostream>

int main(void) {
  try {
    throw "error";
  } catch(...) {
    std::cout << "Exception caught." << std::endl;
    return 0;
  }
  std::cout << "Exception skipped." << std::endl;
  return 1;
}
