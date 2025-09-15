/* Copyright (c) 2021-2025, Arm Limited and affiliates. 
 *
 * Part of the Arm Toolchain project, under the Apache License v2.0 with LLVM Exceptions.
 * See https://llvm.org/LICENSE.txt for license information.
 * SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception */

#include <vector>
#include <iostream>

int main(void) {
  std::vector<int> v = {1, 2, 3};
  v.push_back(4);
  v.insert(v.end(), 5);

  for (int elem: v) {
    std::cout << elem << " ";
  }
  std::cout << std::endl;

  return 0;
}
