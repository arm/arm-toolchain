//
// Copyright (c) 2026, Arm Limited and affiliates.
//
// Part of the Arm Toolchain project, under the Apache License v2.0 with LLVM
// Exceptions. See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//

#if __ARM_ARCH_PROFILE == 'A' || __ARM_ARCH_PROFILE == 'R'

#include "system_registers_a.h"

namespace bootcode {
namespace sysreg {

#ifdef __ARM_ARCH_ISA_A64
CurrentEL_Class CurrentEL;
TPIDR2_Class TPIDR2;
SVCR_Class SVCR;
SMCR_EL2_Class SMCR_EL2;
SMCR_EL3_Class SMCR_EL3;
#endif
SCTLR_Class SCTLR;
CLIDR_Class CLIDR;
CCSIDR_Class CCSIDR;
CPTR_Class CPTR;
GCR_Class GCR;
DACR_Class DACR;
CPACR_Class CPACR;
PMCCFILTR_Class PMCCFILTR;
ID_DFR0_Class ID_DFR0;
ID_AA64PFR1_Class ID_AA64PFR1;
ID_AA64MMFR2_Class ID_AA64MMFR2;
SysReg<SysRegName::VBAR> VBAR;
SysReg<SysRegName::ESR> ESR;
SysReg<SysRegName::ELR> ELR;
SysReg<SysRegName::FAR> FAR;
SysReg<SysRegName::CSSELR> CSSELR;
SysReg<SysRegName::TTBR0> TTBR0;
SysReg<SysRegName::MAIR> MAIR;
SysReg<SysRegName::TCR> TCR;
SysReg<SysRegName::APIAKeyLo> APIAKeyLo;
SysReg<SysRegName::APIAKeyHi> APIAKeyHi;
SysReg<SysRegName::APIBKeyLo> APIBKeyLo;
SysReg<SysRegName::APIBKeyHi> APIBKeyHi;
SysReg<SysRegName::APDAKeyLo> APDAKeyLo;
SysReg<SysRegName::APDAKeyHi> APDAKeyHi;
SysReg<SysRegName::APDBKeyLo> APDBKeyLo;
SysReg<SysRegName::APDBKeyHi> APDBKeyHi;
SysReg<SysRegName::APGAKeyLo> APGAKeyLo;
SysReg<SysRegName::APGAKeyHi> APGAKeyHi;

} // namespace sysreg
} // namespace bootcode
#endif
