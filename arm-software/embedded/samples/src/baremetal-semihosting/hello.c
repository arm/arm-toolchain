/* Copyright (c) 2020-2025, Arm Limited and affiliates. 
 *
 * Part of the Arm Toolchain project, under the Apache License v2.0 with LLVM Exceptions.
 * See https://llvm.org/LICENSE.txt for license information.
 * SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception */

#include <stdio.h>
#include <math.h>

int main(void) {
  printf("Hello World!\n");
  printf("pi = %f\n", 4.0f * atanf(1.0f));
  return 0;
}
