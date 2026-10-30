//
// Copyright (c) 2026, Arm Limited and affiliates.
//
// Part of the Arm Toolchain project, under the Apache License v2.0 with LLVM
// Exceptions. See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//

#if defined(__ARM_ARCH_ISA_A64)

// https://github.com/ARM-software/abi-aa/blob/main/aapcs64/aapcs64.rst#811__arm_sme_state
//
// Preserve x2-x15, x19-x29, SP, all Z and P registers, and ZA.

.section .text.__arm_sme_state, "axy", %progbits
.p2align 2
.globl __arm_sme_state
.type __arm_sme_state, %function
.variant_pcs __arm_sme_state
__arm_sme_state:
  .cfi_startproc
  // HINT encodings allow assembly for baseline Armv8-A.
#ifdef __ARM_FEATURE_BTI_DEFAULT
  hint #34 // BTI c
#endif
// PAC_DEFAULT: key A=1, key B=2, leaf=4, PC diversification=8.
#if __ARM_FEATURE_PAC_DEFAULT & 4
#if __ARM_FEATURE_PAC_DEFAULT & 8
  hint #39 // PACM
.Lsign:
#endif
#if __ARM_FEATURE_PAC_DEFAULT & 2
  .cfi_b_key_frame
  hint #27 // PACIBSP
#else
  hint #25 // PACIASP
#endif
#if __ARM_FEATURE_PAC_DEFAULT & 8
  .cfi_set_ra_state 2, .Lsign
#else
  .cfi_negate_ra_state
#endif
#endif
  mrs x0, ID_AA64PFR1_EL1
  ubfx x0, x0, #24, #4 // ID_AA64PFR1_EL1.SME
  mov x1, xzr
  cbz x0, .Lreturn

  // Use architectural encodings to allow building without +sme.
  mrs x0, S3_3_C4_C2_2 // SVCR
  and x0, x0, #3 // PSTATE.SM and PSTATE.ZA
  orr x0, x0, #0xc000000000000000 // SME and TPIDR2_EL0 available.
  mrs x1, S3_3_C13_C0_5 // TPIDR2_EL0

.Lreturn:
#if __ARM_FEATURE_PAC_DEFAULT & 4
#if __ARM_FEATURE_PAC_DEFAULT & 8
  adr x16, .Lsign // Original signing PC for AUTI[AB]SP.
  hint #39 // PACM
#endif
#if __ARM_FEATURE_PAC_DEFAULT & 2
  hint #31 // AUTIBSP
#else
  hint #29 // AUTIASP
#endif
#if __ARM_FEATURE_PAC_DEFAULT & 8
  .cfi_set_ra_state 0, 0
#else
  .cfi_negate_ra_state
#endif
#endif
  ret
  // Mitigate straight-line speculation; -mharden-sls has no predefined macro.
  dsb sy
  isb
  .cfi_endproc
.size __arm_sme_state, .-__arm_sme_state

// GNU property note layout from compiler-rt/lib/builtins/assembly.h.
#define NT_GNU_PROPERTY_TYPE_0 5
#define GNU_PROPERTY_AARCH64_FEATURE_1_AND 0xc0000000
#define GNU_PROPERTY_AARCH64_FEATURE_1_BTI (1 << 0)
#define GNU_PROPERTY_AARCH64_FEATURE_1_PAC (1 << 1)
#define GNU_PROPERTY_AARCH64_FEATURE_1_GCS (1 << 2)

.set .Lfeature_flags, 0
#ifdef __ARM_FEATURE_BTI_DEFAULT
.set .Lfeature_flags, .Lfeature_flags | GNU_PROPERTY_AARCH64_FEATURE_1_BTI
#endif
#if __ARM_FEATURE_PAC_DEFAULT & 3
.set .Lfeature_flags, .Lfeature_flags | GNU_PROPERTY_AARCH64_FEATURE_1_PAC
#endif
#ifdef __ARM_FEATURE_GCS_DEFAULT
.set .Lfeature_flags, .Lfeature_flags | GNU_PROPERTY_AARCH64_FEATURE_1_GCS
#endif

.if .Lfeature_flags
.pushsection .note.gnu.property, "a", %note
.p2align 3
.word 4 // namesz
.word 16 // descsz
.word NT_GNU_PROPERTY_TYPE_0
.asciz "GNU"
.word GNU_PROPERTY_AARCH64_FEATURE_1_AND
.word 4 // pr_datasz
.word .Lfeature_flags
.word 0 // Padding
.popsection
.endif

#endif // __ARM_ARCH_ISA_A64

#if defined(__ELF__)
.section .note.GNU-stack, "", %progbits
#endif
