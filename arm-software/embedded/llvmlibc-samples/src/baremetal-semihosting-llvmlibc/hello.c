// Copyright (c) 2024-2025, Arm Limited and affiliates.
//
// Part of the Arm Toolchain project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

// Implementation of errno
int *__llvm_libc_errno() {
  static int internal_err;
  return &internal_err;
}

// Example that uses heap, string and math library.

int main(void) {
  const char *hello_s = "hello ";
  const char *world_s = "world";
  const size_t hello_s_len = strlen(hello_s);
  const size_t world_s_len = strlen(world_s);
  const size_t out_s_len = hello_s_len + world_s_len + 1;
  char *out_s = (char*) malloc(out_s_len);
  assert(out_s_len >= hello_s_len + 1);
  strncpy(out_s, hello_s, hello_s_len + 1);
  assert(out_s_len >= strlen(out_s) + world_s_len + 1);
  strncat(out_s, world_s, world_s_len + 1);
  printf("%s\npi = %f\n", out_s, 4.0f * atanf(1.0f));
  free(out_s);
  return 0;
}
