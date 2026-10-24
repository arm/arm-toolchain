/* Copyright (c) 2023-2026, Arm Limited and affiliates. 
 * 
 * Part of the Arm Toolchain project, under the Apache License v2.0 with LLVM Exceptions.
 * See https://llvm.org/LICENSE.txt for license information.
 * SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception */

/*
 * This program includes a greeting message in TLS to check
 * the thread pointer was set up correctly, and a zero-initialised
 * variable to check that the zero initialisation happened.
 *
 * Since this example is also compressing the RW data segment, we
 * should include some highly compressible data in that segment to
 * prove it ends up smaller.
 */

#include <stdio.h>

_Thread_local const char *tls_string = "hello via a pointer in TLS";

static int zeroinit_variable;

static char highly_compressible_data[] =
"Ten green bottles hanging on the wall,\n"
"Ten green bottles hanging on the wall.\n"
"And if one green bottle should accidentally fall\n"
"There'd be nine green bottles hanging on the wall.\n"
"\n"
"Nine green bottles hanging on the wall,\n"
"Nine green bottles hanging on the wall.\n"
"And if one green bottle should accidentally fall\n"
"There'd be eight green bottles hanging on the wall.\n"
"\n"
"Eight green bottles hanging on the wall,\n"
"Eight green bottles hanging on the wall.\n"
"And if one green bottle should accidentally fall\n"
"There'd be seven green bottles hanging on the wall.\n"
"\n"
"Seven green bottles hanging on the wall,\n"
"Seven green bottles hanging on the wall.\n"
"And if one green bottle should accidentally fall\n"
"There'd be six green bottles hanging on the wall.\n"
"\n"
"Six green bottles hanging on the wall,\n"
"Six green bottles hanging on the wall.\n"
"And if one green bottle should accidentally fall\n"
"There'd be five green bottles hanging on the wall.\n"
"\n"
"Five green bottles hanging on the wall,\n"
"Five green bottles hanging on the wall.\n"
"And if one green bottle should accidentally fall\n"
"There'd be four green bottles hanging on the wall.\n"
"\n"
"Four green bottles hanging on the wall,\n"
"Four green bottles hanging on the wall.\n"
"And if one green bottle should accidentally fall\n"
"There'd be three green bottles hanging on the wall.\n"
"\n"
"Three green bottles hanging on the wall,\n"
"Three green bottles hanging on the wall.\n"
"And if one green bottle should accidentally fall\n"
"There'd be two green bottles hanging on the wall.\n"
"\n"
"Two green bottles hanging on the wall,\n"
"Two green bottles hanging on the wall.\n"
"And if one green bottle should accidentally fall\n"
"There'd be one green bottle hanging on the wall.\n"
"\n"
"One green bottle hanging on the wall,\n"
"One green bottle hanging on the wall.\n"
"And if that green bottle should accidentally fall\n"
"There'd be no green bottles hanging on the wall.";

int main(void)
{
    printf("tls_string = \"%s\"\nzeroinit_variable = %d\n%s\n",
           tls_string, zeroinit_variable, highly_compressible_data);
    return 0;
}
