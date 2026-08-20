#include "src/stdio/fclose.h"

#include "src/__support/OSUtil/io.h"
#include "src/__support/common.h"

namespace LIBC_NAMESPACE_DECL {

LLVM_LIBC_FUNCTION(int, fclose, (::FILE * stream)) {
  return __llvm_libc_stdio_close(stream);
}

} // namespace LIBC_NAMESPACE_DECL
