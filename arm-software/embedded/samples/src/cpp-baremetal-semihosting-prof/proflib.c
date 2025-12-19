// Copyright (c) 2023-2025, Arm Limited and affiliates.

// This file is derived from various files in compiler-rt of the llvm-project,
// see https://github.com/llvm/llvm-project/tree/main/compiler-rt/lib/profile

// This C file should be compiled without -fprofile-instr-generate.
// It will provide enough of the runtime for files compiled with
// -fprofile-instr-generate and, optionally, -fcoverage-mapping

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

// Callsbacks into the profile runtime
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
