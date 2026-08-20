//===-- Implementation of fseeko for baremetal -----------------*- C++ -*-===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#include "src/stdio/fseeko.h"

#include "hdr/errno_macros.h"
#include "src/__support/OSUtil/io.h"
#include "src/__support/common.h"
#include "src/__support/libc_errno.h"

namespace LIBC_NAMESPACE_DECL {

LLVM_LIBC_FUNCTION(int, fseeko, (::FILE * stream, off_t offset, int whence)) {
  if (stream == nullptr) {
    libc_errno = EINVAL;
    return -1;
  }
  off_t result = __llvm_libc_stdio_seek(stream, offset, whence);
  if (result < 0) {
    libc_errno = static_cast<int>(-result);
    return -1;
  }
  return 0;
}

} // namespace LIBC_NAMESPACE_DECL
