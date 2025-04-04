// Copyright (c) 2025, Arm Limited and affiliates.
//
// Part of the Arm Toolchain project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception

#include <cstdio> // iostream is not supported yet
#include <vector>

// Implementation of errno
extern "C" {
  int *__llvm_libc_errno() {
    static int internal_err;
    return &internal_err;
  }
}

int test_exceptions(int i)
{
  if (i == 0) {
    throw "Bad argument.";
  } else {
    return i + 1;
  }
}

// Example that uses a C++ container and exceptions
int main(void) {
  std::vector<int> v = {1, 2, 3};
  v.push_back(4);
  v.insert(v.end(), 5);

  for (int elem: v) {
    std::printf("%d ", elem);
  }
  std::puts("");

  try {
    int result = test_exceptions(0);
  } catch (...) {
    std::puts("Exception caught.");
  }
  return 0;
}
