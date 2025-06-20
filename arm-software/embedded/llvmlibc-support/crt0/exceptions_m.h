//
// Copyright (c) 2025, Arm Limited and affiliates.
//
// Part of the Arm Toolchain project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//

// M-profile exception handling

#ifndef BOOTCODE_EXCEPTIONS_M_H
#define BOOTCODE_EXCEPTIONS_M_H

#include "exceptions_common.h"
#include "system_registers_m.h"
#include <stdlib.h>
#include <string.h>

namespace bootcode {
namespace exceptions {

using namespace sysreg;

extern "C" void _start();

EXFN_ATTR void __print_faulting_instruction(unsigned short *ptr) {
  unsigned short first = ptr[0];
  // Check whether this is a 32-bit instruction by examining the top 5 bits of
  // the first half-word.
  if ((first & 0x1800) != 0 && (first & 0xe000) == 0xe000) {
    // 32-bit instruction, printed as two half-words, which matches the order in
    // the ARMARM.
    unsigned short second = ptr[1];
    print_str("  Faulting instruction: ");
    print_hex(first);
    print_str(" ");
    print_hex(second);
    print_str("\n");
  } else {
    // 16-bit instruction
    print_str("  Faulting instruction: ");
    print_hex(first);
    print_str("\n");
  }
  abort();
}

EXFN_ATTR void __hardfault_handler() {
  unsigned *pushed_regs = 2 + (unsigned *)__builtin_frame_address(0);
  unsigned pc_val = pushed_regs[6];
  print_str("CPU Exception: HardFault\n");
  print_str("  PC = 0x");
  print_hex(pc_val);
  print_str("\n");
  print_str("  HFSR = 0x");
  print_hex((unsigned int)HFSR);
  print_str("\n");
  __print_faulting_instruction((unsigned short *)pc_val);
}

EXFN_ATTR void __memmanage_handler() {
  unsigned *pushed_regs = 2 + (unsigned *)__builtin_frame_address(0);
  unsigned pc_val = pushed_regs[6];
  print_str("CPU Exception: MemMange\n");
  print_str("  PC = 0x");
  print_hex(pc_val);
  print_str("\n");
  print_str("  CFSR.MemManage = 0x");
  print_hex((unsigned int)CFSR.MMFSR);
  print_str("\n");
  print_str("  MMFAR = 0x");
  print_hex((unsigned int)MMFAR);
  print_str("\n");
  __print_faulting_instruction((unsigned short *)pc_val);
}

EXFN_ATTR void __busfault_handler() {
  unsigned *pushed_regs = 2 + (unsigned *)__builtin_frame_address(0);
  unsigned pc_val = pushed_regs[6];
  print_str("CPU Exception: BusFault\n");
  print_str("  PC = 0x");
  print_hex(pc_val);
  print_str("\n");
  print_str("  CFSR.BusFault = 0x");
  print_hex((unsigned int)CFSR.BFSR);
  print_str("\n");
  print_str("  BFAR = 0x");
  print_hex((unsigned int)BFAR);
  print_str("\n");
  __print_faulting_instruction((unsigned short *)pc_val);
}

EXFN_ATTR void __usagefault_handler() {
  unsigned *pushed_regs = 2 + (unsigned *)__builtin_frame_address(0);
  unsigned pc_val = pushed_regs[6];
  print_str("CPU Exception: UsageFault\n");
  print_str("  PC = 0x");
  print_hex(pc_val);
  print_str("\n");
  print_str("  CFSR.UsageFault = 0x");
  print_hex((unsigned short)CFSR.UFSR), print_str("\n");
  __print_faulting_instruction((unsigned short *)pc_val);
}

EXFN_ATTR void __securefault_handler() {
  unsigned *pushed_regs = 2 + (unsigned *)__builtin_frame_address(0);
  unsigned pc_val = pushed_regs[6];
  print_str("CPU Exception: SecureFault\n");
  print_str("  PC = 0x");
  print_hex(pc_val);
  print_str("\n");
  print_str("  SFSR.BusFault = 0x");
  print_hex((unsigned int)SFSR);
  print_str("\n");
  print_str("  SFAR = 0x");
  print_hex((unsigned int)SFAR);
  print_str("\n");
  __print_faulting_instruction((unsigned short *)pc_val);
}

extern "C" unsigned int __systick_count = 0;
EXFN_ATTR void __systick_handler() { __systick_count += 1; }

EXFN_ATTR void __exception_handler() { abort(); }

// Architecturally the bottom 7 bits of VTOR are zero, meaning the vector table
// has to be 128-byte aligned, however an implementation can require more bits
// to be zero and cortex-m23 can require up to 10, so 1024-byte align the vector
// table.
extern char __stack __attribute__((weak));
using vtable_t = void (*)(void);
const vtable_t vector_table[]
    __attribute__((section(".vectors"), aligned(1024))) = {
        (vtable_t)&__stack,    // SP
        _start,                // Reset
        __exception_handler,   // NMI
        __hardfault_handler,   // HardFault
        __memmanage_handler,   // MemManage fault
        __busfault_handler,    // BusFault
        __usagefault_handler,  // UsageFault
        __securefault_handler, // SecureFault
        __exception_handler,   // Reserved
        __exception_handler,   // Reserved
        __exception_handler,   // Reserved
        __exception_handler,   // SVCall
        __exception_handler,   // DebugMonitor
        __exception_handler,   // Reserved
        __exception_handler,   // PendSV
        __systick_handler,     // SysTick
};

void setup() {
  // It's implementation-defined whether VTOR is writable, and if it is
  // writeable then it's implementation-defined how many of the bottom bits are
  // zero (though it must be at least 7). First try setting the top bit to see
  // if any part of it is writable.
  VTOR = 0x80000000;
  if (VTOR != 0x80000000) {
    // VTOR isn't writable, which means it must be hardwired to a suitable
    // memory location, so copy the vector table to that location.
    if (VTOR == 0x0) {
      // Do nothing, vector_table is already placed in the correct place
    } else {
      memcpy((void *)(unsigned int)VTOR, &vector_table, sizeof(vector_table));
    }
  } else {

    // VTOR is writable, so set it to the address of the vector table. This
    // should be sufficiently-aligned for all existing CPUs, but it's possible
    // a future one will require more alignment so check just in case.
    VTOR = (unsigned long)&vector_table;
    if (VTOR != (unsigned long)&vector_table) {
      print_str("Bootcode failed to set VTOR\n");
      abort();
    }
  }

  // For v7M and v8M.main, set bits [19:16] of SHCSR to enable SecureFault,
  // UsageFault, BusFault and MemManage. Without this, they all get converted
  // to HardFault, and detailed info about the exception is lost.
  SHCSR.MEMFAULTENA = 1;
  SHCSR.BUSFAULTENA = 1;
  SHCSR.USGFAULTENA = 1;
  SHCSR.SECUREFAULTENA = 1;

#if __ARM_ARCH == 7
  // Align stack on exception entry. This is only needed for v7M, for all other
  // M-profile architectures stack alignment cannot be disabled.
  CCR.STKALIGN = 1;
#endif

  // Disable interrupts on all interrupt lines
  for (int line = ICTR.INTLINESNUM; line > 0; --line) {
    NVIC_ICER[line] = 0xffffffff;
  }
  SYST_CSR.ENABLE = 0;  // Disable SysTick counter
  SYST_CSR.TICKINT = 0; // Disable SysTick interrupt
  ICSR.PENDSTCLR = 1;   // Clear pending SysTick
  ICSR.PENDSVCLR = 1;   // Clean pending PendSV
}

} // namespace exceptions
} // namespace bootcode

#endif // BOOTCODE_EXCEPTIONS_M_H
