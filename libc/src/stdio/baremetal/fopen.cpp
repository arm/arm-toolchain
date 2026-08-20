#include "src/stdio/fopen.h"

#include "hdr/errno_macros.h"
#include "src/__support/OSUtil/io.h"
#include "src/__support/common.h"
#include "src/__support/libc_errno.h"

namespace LIBC_NAMESPACE_DECL {

LLVM_LIBC_FUNCTION(::FILE *, fopen,
                   (const char *__restrict path, const char *__restrict mode)) {
  if (path == nullptr || mode == nullptr || mode[0] == '\0') {
    libc_errno = EINVAL;
    return nullptr;
  }
  void *cookie = nullptr;
  int result = __llvm_libc_stdio_open(path, mode, &cookie);
  if (result != 0 || cookie == nullptr) {
    libc_errno = result != 0 ? -result : EINVAL;
    return nullptr;
  }
  return reinterpret_cast<::FILE *>(cookie);
}

} // namespace LIBC_NAMESPACE_DECL
