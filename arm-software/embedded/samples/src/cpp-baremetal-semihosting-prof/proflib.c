// Copyright (c) 2023-2026, Arm Limited and affiliates.

// This file uses the libclang_rt.profile library and semihosting IO to produce
// a raw profile file that can be processed by the llvm tools
// such as llvm-profdata. 
// See https://clang.llvm.org/docs/UsersManual.html#profiling-with-instrumentation

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

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
