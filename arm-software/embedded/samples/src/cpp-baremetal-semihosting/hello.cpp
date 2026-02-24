/* Copyright (c) 2021-2025, Arm Limited and affiliates. 
 *
 * Part of the Arm Toolchain project, under the Apache License v2.0 with LLVM Exceptions.
 * See https://llvm.org/LICENSE.txt for license information.
 * SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception */

#include <vector>
#include <iostream>
#include <cstdio>

int main(void) {
  std::vector<int> v = {1, 2, 3};
  v.push_back(4);
  v.insert(v.end(), 5);

  for (int elem: v) {
#if __LLVM_LIBC__
    std::printf("%d ", elem);
#else
    std::cout << elem << " ";
#endif
  }
  std::puts("\n");

  return 0;
}
