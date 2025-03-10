//
// Copyright (c) 2022-2025, Arm Limited and affiliates.
//
// Part of the Arm Toolchain project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//

#include <stddef.h>
#include <stdlib.h>

#include "platform.h"

int main(int, char **);

__attribute__((used)) static void c_startup(void) {
  _platform_init();
  _Exit(main(0, NULL));
}

extern long __stack[];
__attribute__((naked)) void _start(void) {
  __asm__("mov sp, %0" : : "r"(__stack));
  __asm__("b c_startup");
}
