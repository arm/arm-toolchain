//
// Copyright (c) 2025, Arm Limited and affiliates.
//
// Part of the Arm Toolchain project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//

// ARMv7-A exception handling

#ifndef BOOTCODE_EXCEPTIONS_7A_H
#define BOOTCODE_EXCEPTIONS_7A_H

#include "exceptions_common.h"
#include "system_registers_a.h"
#include <stdlib.h>

namespace bootcode {
namespace exceptions {

using namespace sysreg;

// The following function is only valid in architectures with the
// Virtualization Extension. armv7ve is the most basic architecture with it.
// However, because armv7ve is not a valid value for the target attribute,
// Cortex-A7 is used instead. The code is simple enough not to use instructions
// that would be valid for Cortex-A7 but not for armv7ve.
[[gnu::target("arch=cortex-a7")]] static uintptr_t read_elr_hyp() {
  return __arm_rsr("ELR_hyp");
}

static bool is_in_hyp_mode() {
  const uint32_t cpsr = __arm_rsr("CPSR");
  const uint32_t mode_bits = cpsr & 0x1F;
  return mode_bits == 0x1A;
}

#define GET_LR_VALUE()                                                         \
  (is_in_hyp_mode() ? read_elr_hyp() : (uintptr_t)__builtin_return_address(0))

EXFN_ATTR void handle_reset() {
  print_str("CPU Exception: Reset\n");
  abort();
}

EXFN_ATTR void handle_undefined() {
  uintptr_t lr_val = GET_LR_VALUE();
  print_str("CPU Exception: Undefined Instruction\n");
  print_str("  PC = ");
  print_hex(lr_val);
  print_str("\n");
  const uint16_t half_word0 = *(uint16_t *)lr_val;
  const uint16_t half_word1 = *((uint16_t *)lr_val + 1);
  print_str("  Instruction (first half word) = ");
  print_hex(half_word0);
  print_str("  Instruction (second half word) = ");
  print_hex(half_word1);
  print_str("\n");
  abort();
}

EXFN_ATTR void handle_svc_hyp_smc() {
  print_str("CPU Exception: SVC, HVC or SMC\n");
  print_str("  PC = ");
  print_hex(GET_LR_VALUE());
  print_str("\n");
  abort();
}

EXFN_ATTR void handle_prefetch_abort() {
  print_str("CPU Exception: Prefetch Abort\n");
  print_str("  PC = ");
  print_hex(GET_LR_VALUE());
  print_str("\n");
  print_str("  IFSR = 0x%08x\n");
  print_str("  IFAR = 0x%08x\n");
  abort();
}

EXFN_ATTR void handle_data_abort() {
  print_str("CPU Exception: Data Abort\n");
  print_str("  PC = ");
  print_hex(GET_LR_VALUE());
  print_str("\n");
  print_str("  DFSR = 0x%08x\n");
  print_str("  DFAR = 0x%08x\n");
  abort();
}

EXFN_ATTR void handle_hyp_trap() {
  print_str("CPU Exception: Hypervisor Trap\n");
  print_str("  PC = ");
  print_hex(GET_LR_VALUE());
  print_str("\n");
  print_str("  HSR = 0x%08x\n");
  abort();
}

EXFN_ATTR void handle_irq() {
  print_str("CPU Exception: IRQ\n");
  print_str("  PC = ");
  print_hex(GET_LR_VALUE());
  print_str("\n");
  abort();
}

EXFN_ATTR void handle_fiq() {
  print_str("CPU Exception: FIQ\n");
  print_str("  PC = ");
  print_hex(GET_LR_VALUE());
  print_str("\n");
  abort();
}

// The AArch32 exception vector table has 8 entries, each of which is 4
// bytes long, and contains code. The whole table must be 32-byte aligned.
// The table may also be relocated, so we make it position-independent by
// having a table of handler addresses and loading the address to pc.
[[gnu::section(".text.init.enter"), gnu::aligned(32), gnu::used, gnu::naked,
  gnu::target("arm")]]
void vector_table() {
  asm("LDR pc, [pc, #24]");
  asm("LDR pc, [pc, #24]");
  asm("LDR pc, [pc, #24]");
  asm("LDR pc, [pc, #24]");
  asm("LDR pc, [pc, #24]");
  asm("LDR pc, [pc, #24]");
  asm("LDR pc, [pc, #24]");
  asm("LDR pc, [pc, #24]");
  asm(".word %c0" : : "X"(handle_reset));
  asm(".word %c0" : : "X"(handle_undefined));
  asm(".word %c0" : : "X"(handle_svc_hyp_smc));
  asm(".word %c0" : : "X"(handle_prefetch_abort));
  asm(".word %c0" : : "X"(handle_data_abort));
  asm(".word %c0" : : "X"(handle_hyp_trap));
  asm(".word %c0" : : "X"(handle_irq));
  asm(".word %c0" : : "X"(handle_fiq));
}

} // namespace exceptions
} // namespace bootcode

#endif // BOOTCODE_EXCEPTIONS_7A_H
