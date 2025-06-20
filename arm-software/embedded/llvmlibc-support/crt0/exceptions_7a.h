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

EXFN_ATTR void handle_reset() {
  print_str("CPU Exception: Reset\n");
  abort();
}

EXFN_ATTR void handle_undefined() {
  unsigned pc_val = __arm_rsr("ELR_hyp");
  unsigned instr = *(unsigned *)pc_val;
  print_str("CPU Exception: Undefined Instruction\n");
  print_str("  PC = ");
  print_hex(pc_val);
  print_str("\n");
  print_str("  Instruction = ");
  print_hex(instr);
  print_str("\n");
  abort();
}

EXFN_ATTR void handle_svc_hyp_smc() {
  print_str("CPU Exception: SVC, HVC or SMC\n");
  print_str("  PC = ");
  print_hex(__arm_rsr("ELR_hyp"));
  print_str("\n");
  abort();
}

EXFN_ATTR void handle_prefetch_abort() {
  print_str("CPU Exception: Prefetch Abort\n");
  print_str("  PC = ");
  print_hex(__arm_rsr("ELR_hyp"));
  print_str("\n");
  print_str("  IFSR = 0x%08x\n");
  print_str("  IFAR = 0x%08x\n");
  abort();
}

EXFN_ATTR void handle_data_abort() {
  print_str("CPU Exception: Data Abort\n");
  print_str("  PC = ");
  print_hex(__arm_rsr("ELR_hyp"));
  print_str("\n");
  print_str("  DFSR = 0x%08x\n");
  print_str("  DFAR = 0x%08x\n");
  abort();
}

EXFN_ATTR void handle_hyp_trap() {
  print_str("CPU Exception: Hypervisor Trap\n");
  print_str("  PC = ");
  print_hex(__arm_rsr("ELR_hyp"));
  print_str("\n");
  print_str("  HSR = 0x%08x\n");
  abort();
}

EXFN_ATTR void handle_irq() {
  print_str("CPU Exception: IRQ\n");
  print_str("  PC = ");
  print_hex(__arm_rsr("ELR_hyp"));
  print_str("\n");
  abort();
}

EXFN_ATTR void handle_fiq() {
  print_str("CPU Exception: FIQ\n");
  print_str("  PC = ");
  print_hex(__arm_rsr("ELR_hyp"));
  print_str("\n");
  abort();
}

// The AArch32 exception vector table has 8 entries, each of which is 4
// bytes long, and contains code. The whole table must be 32-byte aligned.
// The table may also be relocated, so we make it position-independent by
// having a table of handler addresses and loading the address to pc.
__attribute__((naked, section(".vectors"), aligned(32))) void vector_table() {
  asm("LDR pc, [pc, #24]");
  asm("LDR pc, [pc, #24]");
  asm("LDR pc, [pc, #24]");
  asm("LDR pc, [pc, #24]");
  asm("LDR pc, [pc, #24]");
  asm("LDR pc, [pc, #24]");
  asm("LDR pc, [pc, #24]");
  asm("LDR pc, [pc, #24]");
  asm(".word %0" : : "X"(handle_reset));
  asm(".word %0" : : "X"(handle_undefined));
  asm(".word %0" : : "X"(handle_svc_hyp_smc));
  asm(".word %0" : : "X"(handle_prefetch_abort));
  asm(".word %0" : : "X"(handle_data_abort));
  asm(".word %0" : : "X"(handle_hyp_trap));
  asm(".word %0" : : "X"(handle_irq));
  asm(".word %0" : : "X"(handle_fiq));
}

} // namespace exceptions
} // namespace bootcode

#endif // BOOTCODE_EXCEPTIONS_7A_H
