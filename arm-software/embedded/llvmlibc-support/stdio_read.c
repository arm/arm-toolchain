//
// Copyright (c) 2022-2025, Arm Limited and affiliates.
//
// Part of the Arm Toolchain project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//

#include <stddef.h>

#include "semihost.h"

ssize_t __llvm_libc_stdio_read(struct __llvm_libc_stdio_cookie *cookie,
                               const char *buf, size_t size) {
  size_t args[4];
  args[0] = (size_t)cookie->handle;
  args[1] = (size_t)buf;
  args[2] = (size_t)size;
  args[3] = 0;
  ssize_t retval = semihosting_call(SYS_READ, args);
  if (retval >= 0)
    retval = size - retval;
  return retval;
}
