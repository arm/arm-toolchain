/* Copyright (c) 2020-2025, Arm Limited and affiliates. 
 *
 * Part of the Arm Toolchain project, under the Apache License v2.0 with LLVM Exceptions.
 * See https://llvm.org/LICENSE.txt for license information.
 * SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception */

#include <stdio.h>

// https://infocenter.nordicsemi.com/pdf/nRF51_RM_v3.0.pdf sections 14.2 and 29.10

#define DEFINE_PORT(name,address) volatile unsigned int* name = (unsigned int*) address

DEFINE_PORT(gpio_dir_set, 0x50000518);

DEFINE_PORT(uart_starttx, 0x40002008);
DEFINE_PORT(uart_txdrdy, 0x4000211C);
DEFINE_PORT(uart_enable, 0x40002500);
DEFINE_PORT(uart_pseltxd, 0x4000250C);
DEFINE_PORT(uart_txd, 0x4000251C);
DEFINE_PORT(uart_baud_rate, 0x40002524);
DEFINE_PORT(uart_config, 0x4000256C);

__attribute__ ((noinline)) void delay(int cycles)
{
  for (int c = cycles; --c; ) {
    // prevent the compiler from optimising out the loop
    asm("");
  }
}

void uart_init()
{
  *gpio_dir_set = 1 << 24;
  *uart_baud_rate = 0x01D7E000; // 115200
  *uart_config = 0;
  *uart_pseltxd = 24;
  *uart_enable = 4;
  *uart_starttx = 1;

  delay(1000);
}

int uart_putc(char ch, FILE* file)
{
  (void) file; /* unused */

  *uart_txdrdy = 0;
  *uart_txd = ch;
  while (!*uart_txdrdy) {};

  return ch;
}

#if defined(__PICOLIBC_VERSION__) // built with picolibc

  /* Redirect stdio as per https://github.com/picolibc/picolibc/blob/main/doc/os.md */
  static FILE __stdio = FDEV_SETUP_STREAM(uart_putc, NULL, NULL, _FDEV_SETUP_WRITE);
  FILE *const stdin = &__stdio;
  __strong_reference(stdin, stdout);
  __strong_reference(stdin, stderr);

#elif __LLVM_LIBC__  // built with llvmlibc

  void* __llvm_libc_stdin_cookie;
  void* __llvm_libc_stdout_cookie;
  void* __llvm_libc_stderr_cookie;

  void _platform_init(void) {
    // Initialize cookies here
    __llvm_libc_stdin_cookie = NULL;
    __llvm_libc_stdout_cookie = NULL;
    __llvm_libc_stderr_cookie = NULL;
}

  ssize_t __llvm_libc_stdio_read(void *cookie, char *buf, size_t size) {
    return -1;  // Not implemented and not used in this sample
  }

  ssize_t __llvm_libc_stdio_write(void *cookie, const char *buf, size_t size) {
    (void) cookie; // Check for specific cookies here as needed

    for(size_t i = 0; i < size; i++) {
      uart_putc(buf[i], NULL);
    }
    return size;
  }

#else
  #error "Unsupported C library"
#endif

int main(void)
{
  uart_init();
  printf("Hello World!");
  return 0;
}
