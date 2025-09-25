//
// Copyright (c) 2022-2025, Arm Limited and affiliates.
//
// Part of the Arm Toolchain project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//

// This header file provides internal definitions for libsemihost.a,
// including an inline function to make a semihosting call, and a lot
// of constant definitions.

#ifndef LLVMET_LLVMLIBC_SUPPORT_SEMIHOST_H
#define LLVMET_LLVMLIBC_SUPPORT_SEMIHOST_H

#include <llvm-libc-types/ssize_t.h>
#include <stdint.h>

#if __ARM_64BIT_STATE
#  define ARG_REG_0 "x0"
#  define ARG_REG_1 "x1"
#else
#  define ARG_REG_0 "r0"
#  define ARG_REG_1 "r1"
#endif

#if __ARM_64BIT_STATE // A64
#  define SEMIHOST_INSTRUCTION "hlt #0xf000"
#elif defined(__thumb__) // T32
#  if defined(__ARM_ARCH_PROFILE) && __ARM_ARCH_PROFILE == 'M'
#    define SEMIHOST_INSTRUCTION "bkpt #0xAB"
#  elif defined(HLT_SEMIHOSTING)
#    define SEMIHOST_INSTRUCTION ".inst.n 0xbabc" // hlt #60
#  else
#    define SEMIHOST_INSTRUCTION "svc 0xab"
#  endif
#else // A32
#  if defined(HLT_SEMIHOSTING)
#    define SEMIHOST_INSTRUCTION ".inst 0xe10f0070" // hlt #0xf000
#  else
#    define SEMIHOST_INSTRUCTION "svc 0x123456"
#  endif
#endif

[[clang::always_inline]]
static long semihosting_call(long val, const void *ptr) {
  register long v __asm__(ARG_REG_0) = val;
  register const void *p __asm__(ARG_REG_1) = ptr;
  __asm__ __volatile__(SEMIHOST_INSTRUCTION
                       : "+r"(v), "+r"(p)
                       :
                       : "memory", "cc");
  return v;
}

namespace {
inline constexpr uint32_t SYS_CLOCK = 0x10;
inline constexpr uint32_t SYS_CLOSE = 0x02;
inline constexpr uint32_t SYS_ELAPSED = 0x30;
inline constexpr uint32_t SYS_ERRNO = 0x13;
inline constexpr uint32_t SYS_EXIT = 0x18;
inline constexpr uint32_t SYS_EXIT_EXTENDED = 0x20;
inline constexpr uint32_t SYS_FLEN = 0x0c;
inline constexpr uint32_t SYS_GET_CMDLINE = 0x15;
inline constexpr uint32_t SYS_HEAPINFO = 0x16;
inline constexpr uint32_t SYS_ISERROR = 0x08;
inline constexpr uint32_t SYS_ISTTY = 0x09;
inline constexpr uint32_t SYS_OPEN = 0x01;
inline constexpr uint32_t SYS_READ = 0x06;
inline constexpr uint32_t SYS_READC = 0x07;
inline constexpr uint32_t SYS_REMOVE = 0x0e;
inline constexpr uint32_t SYS_RENAME = 0x0f;
inline constexpr uint32_t SYS_SEEK = 0x0a;
inline constexpr uint32_t SYS_SYSTEM = 0x12;
inline constexpr uint32_t SYS_TICKFREQ = 0x31;
inline constexpr uint32_t SYS_TIME = 0x11;
inline constexpr uint32_t SYS_TMPNAM = 0x0d;
inline constexpr uint32_t SYS_WRITE0 = 0x04;
inline constexpr uint32_t SYS_WRITE = 0x05;
inline constexpr uint32_t SYS_WRITEC = 0x03;

inline constexpr uint32_t ADP_Stopped_BranchThroughZero = 0x20000;
inline constexpr uint32_t ADP_Stopped_UndefinedInstr = 0x20001;
inline constexpr uint32_t ADP_Stopped_SoftwareInterrupt = 0x20002;
inline constexpr uint32_t ADP_Stopped_PrefetchAbort = 0x20003;
inline constexpr uint32_t ADP_Stopped_DataAbort = 0x20004;
inline constexpr uint32_t ADP_Stopped_AddressException = 0x20005;
inline constexpr uint32_t ADP_Stopped_IRQ = 0x20006;
inline constexpr uint32_t ADP_Stopped_FIQ = 0x20007;
inline constexpr uint32_t ADP_Stopped_BreakPoint = 0x20020;
inline constexpr uint32_t ADP_Stopped_WatchPoint = 0x20021;
inline constexpr uint32_t ADP_Stopped_StepComplete = 0x20022;
inline constexpr uint32_t ADP_Stopped_RunTimeErrorUnknown = 0x20023;
inline constexpr uint32_t ADP_Stopped_InternalError = 0x20024;
inline constexpr uint32_t ADP_Stopped_UserInterruption = 0x20025;
inline constexpr uint32_t ADP_Stopped_ApplicationExit = 0x20026;
inline constexpr uint32_t ADP_Stopped_StackOverflow = 0x20027;
inline constexpr uint32_t ADP_Stopped_DivisionByZero = 0x20028;
inline constexpr uint32_t ADP_Stopped_OSSpecific = 0x20029;

/* SYS_OPEN modes must be one of R,W,A, plus an optional B and optional PLUS */
inline constexpr uint32_t OPENMODE_R = 0;
inline constexpr uint32_t OPENMODE_W = 4;
inline constexpr uint32_t OPENMODE_A = 8;
inline constexpr uint32_t OPENMODE_B = 1;
inline constexpr uint32_t OPENMODE_PLUS = 2;
} // namespace

struct __llvm_libc_stdio_cookie { int handle; };

#endif // LLVMET_LLVMLIBC_SUPPORT_SEMIHOST_H
