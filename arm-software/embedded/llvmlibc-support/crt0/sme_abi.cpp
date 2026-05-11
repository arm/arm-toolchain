//
// Copyright (c) 2026, Arm Limited and affiliates.
//
// Part of the Arm Toolchain project, under the Apache License v2.0 with LLVM
// Exceptions. See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//

#if defined(__ARM_ARCH_ISA_A64)

// Minimal SME ABI support for compiler-rt and LLVM libc users.
// https://github.com/ARM-software/abi-aa/blob/main/aapcs64/aapcs64.rst#811__arm_sme_state
// TODO: Consider caching the state to avoid rechecking id_aa64pfr1_el1
// when running user workloads.
asm(
    ".text\n"
    ".globl __arm_sme_state\n"
    ".type __arm_sme_state, %function\n"
    ".variant_pcs __arm_sme_state\n"
    "__arm_sme_state:\n"
    "  mov x0, xzr\n"
    "  mov x1, xzr\n"
    "\n"
    "  /* Check whether SME or SME2 is present. */\n"
    "  mrs x16, id_aa64pfr1_el1\n"
    "  tst w16, #0x3000000\n"
    "  b.eq .L__arm_sme_state_no_sme\n"
    "\n"
    "  orr x0, x0, #0xC000000000000000\n"
    "  /* Read SVCR. */\n"
    "  mrs x16, s3_3_c4_c2_2\n"
    "  bfxil x0, x16, #0, #2\n"
    "  /* Read TPIDR2_EL0. */\n"
    "  mrs x1, s3_3_c13_c0_5\n"
    ".L__arm_sme_state_no_sme:\n"
    "  ret\n"
    ".size __arm_sme_state, .-__arm_sme_state\n");

#endif
