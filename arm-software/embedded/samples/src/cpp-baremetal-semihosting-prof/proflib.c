// Copyright (c) 2023-2026, Arm Limited and affiliates.

// This file uses the libclang_rt.profile library and semihosting IO to produce
// a raw profile file that can be processed by the llvm tools
// such as llvm-profdata. 
// See https://clang.llvm.org/docs/UsersManual.html#profiling-with-instrumentation

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#if __LLVM_LIBC__
// Temporary workaround until LLVM libc gets file IO support.
// Based on semihost.h and semihost.cpp from
// arm-software/embedded/llvmlibc-support/semihost/

#include <string.h>

#if __ARM_64BIT_STATE
#define ARG_REG_0 "x0"
#define ARG_REG_1 "x1"
#else
#define ARG_REG_0 "r0"
#define ARG_REG_1 "r1"
#endif

#if __ARM_64BIT_STATE
#define SEMIHOST_INSTRUCTION "hlt #0xf000"
#define SEMIHOST_CLOBBER "memory", "cc"
#elif defined(__thumb__)
#if defined(__ARM_ARCH_PROFILE) && __ARM_ARCH_PROFILE == 'M'
#define SEMIHOST_INSTRUCTION "bkpt #0xAB"
#define SEMIHOST_CLOBBER "memory", "cc"
#elif defined(HLT_SEMIHOSTING)
#define SEMIHOST_INSTRUCTION ".inst.n 0xbabc" // hlt #60
#define SEMIHOST_CLOBBER "memory", "cc"
#else
#define SEMIHOST_INSTRUCTION "svc 0xab"
#define SEMIHOST_CLOBBER "memory", "cc", "lr"
#endif
#else
#if defined(HLT_SEMIHOSTING)
#define SEMIHOST_INSTRUCTION ".inst 0xe10f0070" // hlt #0xf000
#define SEMIHOST_CLOBBER "memory", "cc"
#else
#define SEMIHOST_INSTRUCTION "svc 0x123456"
#define SEMIHOST_CLOBBER "memory", "cc", "lr"
#endif
#endif

static long semihosting_call(long op, const void *args) {
  register long op_reg __asm__(ARG_REG_0) = op;
  register const void *args_reg __asm__(ARG_REG_1) = args;
  __asm__ __volatile__(SEMIHOST_INSTRUCTION
                       : "+r"(op_reg), "+r"(args_reg)
                       :
                       : SEMIHOST_CLOBBER);
  return op_reg;
}

enum {
  SYS_OPEN = 1,
  SYS_CLOSE = 2,
  SYS_WRITE = 5,
  OPENMODE_W = 4,
  OPENMODE_B = 1,
};

FILE *fopen(const char *filename, const char *mode) {
  if (!filename || !mode)
    return NULL;

  size_t args[3];
  args[0] = (size_t)filename;
  args[1] = (size_t)(OPENMODE_W | OPENMODE_B);
  args[2] = strlen(filename);

  long handle = semihosting_call(SYS_OPEN, args);
  if (handle < 0)
    return NULL;

  return (FILE *)(uintptr_t)(handle + 1);
}

size_t fwrite(const void *ptr, size_t size, size_t count, FILE *stream) {
  if (!ptr || !stream || size == 0 || count == 0)
    return 0;

  size_t total = size * count;
  size_t args[3];
  args[0] = (size_t)((uintptr_t)stream - 1);
  args[1] = (size_t)ptr;
  args[2] = total;

  long unwritten = semihosting_call(SYS_WRITE, args);
  if (unwritten < 0 || (size_t)unwritten > total)
    return 0;

  return (total - (size_t)unwritten) / size;
}

int fclose(FILE *stream) {
  if (!stream)
    return -1;
  long retval =
      semihosting_call(SYS_CLOSE, (const void *)((uintptr_t)stream - 1));
  return retval == 0 ? 0 : -1;
}
#endif // __LLVM_LIBC__

// Callbacks into the profile runtime
extern uint64_t __llvm_profile_get_size_for_buffer(void);
extern int __llvm_profile_write_buffer(char *buffer);

// Declare this to disable the default profile runtime initialization
uint64_t __llvm_profile_runtime;

// The profile file is written out using semihosting
void __llvm_profile_dump(void) {
  const char *file_name = "default.profraw";
  FILE *fd = fopen(file_name, "wb");
  
  if (!fd) {
    printf("proflib: fopen default.profraw failed.");
    return;
  }
  
  uint64_t size = __llvm_profile_get_size_for_buffer();
  char* buffer = (char*) malloc(size);

  if (buffer) {
    if (__llvm_profile_write_buffer(buffer) == -1) {
      printf("proflib: Getting profile data failed.\n");
    } else {
      fwrite(buffer, size, 1, fd);
    }
    free(buffer);
  } else {
    printf("proflib: malloc for profile data failed.\n");
  }

  fclose(fd);
}

// Register an atexit handler to dump the profile after main has exited
// If this is omitted, a program must manually call __llvm_profile_dump
// to write the profile

__attribute__((constructor)) void __proflib_initialize() {
  atexit(__llvm_profile_dump);
}
