//
// Copyright (c) 2025, Arm Limited and affiliates.
//
// Part of the Arm Toolchain project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//

// ARMv8-A exception handling

#ifndef BOOTCODE_EXCEPTIONS_8A_H
#define BOOTCODE_EXCEPTIONS_8A_H

#include "exceptions_common.h"
#include "system_registers_a.h"
#include <stdlib.h>

namespace bootcode {
namespace exceptions {

using namespace sysreg;

EXFN_ATTR void data_abort_handler(unsigned long esr_val,
                                  unsigned long far_val) {
  unsigned dfsc = esr_val & 0x3f;
  if (dfsc == 0x10) {
    print_str("  Synchronous external abort, not on translation table walk\n");
  } else if (dfsc == 0x11) {
    print_str("  MTE tag check fault\n");
#ifdef __ARM_FEATURE_MEMORY_TAGGING
    void *tagged_ptr = __arm_mte_get_tag((void *)(far_val & ~15));
    unsigned long tag = ((unsigned long)tagged_ptr) >> 56;
    print_str("  MTE allocation tag at faulting address: 0x");
    print_hex(tag, false);
    print_str("\n");
#endif
  } else if (dfsc == 0x13) {
    print_str("  Synchronous external abort, level -1\n");
  } else if (dfsc == 0x18) {
    print_str(
        "  Synchronous parity or ECC error, not on translation table walk\n");
  } else if (dfsc == 0x1b) {
    print_str("  Synchronous parity or ECC error, level -1\n");
  } else if (dfsc == 0x21) {
    print_str("  Alignment fault\n");
  } else if (dfsc == 0x23) {
    print_str("  Granule protection fault, level -1\n");
  } else if (dfsc == 0x29) {
    print_str("  Address size fault, level -1\n");
  } else if (dfsc == 0x2b) {
    print_str("  Translation fault, level -1\n");
  } else if (dfsc == 0x30) {
    print_str("  TLB conflict abort\n");
  } else if (dfsc == 0x31) {
    print_str("  Unsupported atomic hardware update fault\n");
  } else {
    // Generic messages that print the fault type and fault level
    const char *fault_types[0x10] = {
        /* 0x0 */ "Address size fault",
        /* 0x1 */ "Translation fault",
        /* 0x2 */ "Access flag fault",
        /* 0x3 */ "Permission fault",
        /* 0x4 */ "?",
        /* 0x5 */ "Synchronous external abort",
        /* 0x6 */ "?",
        /* 0x7 */ "Synchronous parity or ECC error",
        /* 0x8 */ "?",
        /* 0x9 */ "Granule protection fault",
        /* 0xa */ "?",
        /* 0xb */ "?",
        /* 0xc */ "?",
        /* 0xd */ "?",
        /* 0xe */ "IMPLEMENTATION DEFINED",
        /* 0xf */ "RESERVED",
    };
    const char *fault_levels[] = {", level 0\n", ", level 1\n", ", level 2\n",
                                  ", level 3\n"};
    print_str("  ");
    print_str(fault_types[dfsc >> 2]);
    print_str(fault_levels[dfsc & 0x3]);
  }
}

// Table of ESR.EC value descriptions and iss printing functions
const struct exception_info {
  const char *description;
  void (*handler_fn)(unsigned long, unsigned long);
} ec_values[0x40] = {
    /* 0x00 */ {"Unknown (probably undefined instruction)\n", 0},
    /* 0x01 */ {"Trapped WF*\n", 0},
    /* 0x02 */ {0, 0},
    /* 0x03 */ {0, 0},
    /* 0x04 */ {0, 0},
    /* 0x05 */ {0, 0},
    /* 0x06 */ {0, 0},
    /* 0x07 */ {"Trapped floating-point operation\n", 0},
    /* 0x08 */ {0, 0},
    /* 0x09 */ {"Trapped PAuth\n", 0},
    /* 0x0a */ {"Exception from LD64B or ST64B\n", 0},
    /* 0x0b */ {0, 0},
    /* 0x0c */ {0, 0},
    /* 0x0d */ {"Branch target exception (BTI)\n", 0},
    /* 0x0e */ {"Illegal Execution state\n", 0},
    /* 0x0f */ {0, 0},
    /* 0x10 */ {0, 0},
    /* 0x11 */ {0, 0},
    /* 0x12 */ {0, 0},
    /* 0x13 */ {0, 0},
    /* 0x14 */ {0, 0},
    /* 0x15 */ {"SVC\n", 0},
    /* 0x16 */ {"HVC\n", 0},
    /* 0x17 */ {"SMC\n", 0},
    /* 0x18 */ {"Trapped MSR, MRS, or System\n", 0},
    /* 0x19 */ {"Trapped SVE\n", 0},
    /* 0x1a */ {0, 0},
    /* 0x1b */ {"Exception from TSTART\n", 0},
    /* 0x1c */ {"Pointer Authentication failure\n", 0},
    /* 0x1d */ {"Trapped SME\n", 0},
    /* 0x1e */ {"Granule Protection Check failure\n", 0},
    /* 0x1f */ {0, 0},
    /* 0x20 */ {"Instruction Abort (lower exception level)\n", 0},
    /* 0x21 */ {"Instruction Abort (same exception level)\n", 0},
    /* 0x22 */ {"PC alignment fault\n", 0},
    /* 0x23 */ {0, 0},
    /* 0x24 */ {"Data Abort (lower exception level)\n", data_abort_handler},
    /* 0x25 */ {"Data Abort (same exception level)\n", data_abort_handler},
    /* 0x26 */ {"SP alignment fault\n", 0},
    /* 0x27 */ {"Memory Operation Exception\n", 0},
    /* 0x28 */ {0, 0},
    /* 0x29 */ {0, 0},
    /* 0x2a */ {0, 0},
    /* 0x2b */ {0, 0},
    /* 0x2c */ {"Trapped floating-point exception\n", 0},
    /* 0x2d */ {0, 0},
    /* 0x2e */ {0, 0},
    /* 0x2f */ {"SError\n", 0},
    /* 0x30 */ {0, 0},
    /* 0x31 */ {0, 0},
    /* 0x32 */ {0, 0},
    /* 0x33 */ {0, 0},
    /* 0x34 */ {0, 0},
    /* 0x35 */ {0, 0},
    /* 0x36 */ {0, 0},
    /* 0x37 */ {0, 0},
    /* 0x38 */ {0, 0},
    /* 0x39 */ {0, 0},
    /* 0x3a */ {0, 0},
    /* 0x3b */ {0, 0},
    /* 0x3c */ {"Breakpoint instruction executed\n", 0},
    /* 0x3d */ {0, 0},
    /* 0x3e */ {0, 0},
    /* 0x3f */ {0, 0}};

EXFN_ATTR void exception_handler() {
  unsigned long esr_val, elr_val, far_val;
  esr_val = ESR;
  elr_val = ELR;
  far_val = FAR;

  // Print a message, including the syndrome, link and faulting address
  // registers.
  print_str("CPU Exception:\n");
  print_str("  ESR = 0x");
  print_hex(esr_val);
  print_str("\n");
  print_str("  ELR = 0x");
  print_hex(elr_val);
  print_str("\n");
  print_str("  FAR = 0x");
  print_hex(far_val);
  print_str("\n");

  unsigned ec_val = (esr_val >> 26) & 0x3f;
  const struct exception_info *ei = &ec_values[ec_val];

  // Print a human-readable description of the ESR value
  print_str("Fault description: ");
  if (ei->description) {
    print_str(ei->description);
  } else {
    print_str("Unknown (EC=0x");
    print_hex(ec_val, false);
    print_str(")\n");
  }

  // Print exception-specific information
  if (ei->handler_fn) {
    ei->handler_fn(esr_val, far_val);
  }

  // Print the instruction which faulted. This is done as a separate print_str
  // call so that we will have already printed something useful if the load
  // faults (e.g. the ELR contained a bad value).
  if (ec_val != 0x20 && ec_val != 0x21 && ec_val != 0x22) {
    print_str("  Faulting instruction = 0x");
    print_hex(*(unsigned int *)elr_val);
    print_str("\n");
  }

  // Stop execution
  abort();
}

// The AArch64 exception vector table has 16 entries, each of which is 128
// bytes long, and contains code. The whole table must be 2048-byte aligned.
// For our purposes, each entry just contains one branch instruction to the
// exception reporting function, since we never want to resume after an
// exception.
__attribute__((naked, section(".vectors"), aligned(2048))) void vector_table() {
#define VECTOR_TABLE_ENTRY                                                     \
  asm(".balign 128");                                                          \
  asm("B %0" : : "X"(exception_handler));

  VECTOR_TABLE_ENTRY;
  VECTOR_TABLE_ENTRY;
  VECTOR_TABLE_ENTRY;
  VECTOR_TABLE_ENTRY;
  VECTOR_TABLE_ENTRY;
  VECTOR_TABLE_ENTRY;
  VECTOR_TABLE_ENTRY;
  VECTOR_TABLE_ENTRY;
  VECTOR_TABLE_ENTRY;
  VECTOR_TABLE_ENTRY;
  VECTOR_TABLE_ENTRY;
  VECTOR_TABLE_ENTRY;
  VECTOR_TABLE_ENTRY;
  VECTOR_TABLE_ENTRY;
  VECTOR_TABLE_ENTRY;
  VECTOR_TABLE_ENTRY;
}

} // namespace exceptions
} // namespace bootcode

#endif
