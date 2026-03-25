//
// Copyright (c) 2026, Arm Limited and affiliates.
//
// Part of the Arm Toolchain project, under the Apache License v2.0 with LLVM
// Exceptions. See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//

#if __ARM_ARCH_PROFILE == 'M'

#include "system_registers_m.h"

namespace bootcode {
namespace sysreg {

ICTR_Class ICTR;
SYST_CSR_Class SYST_CSR;
SysReg<SysRegName::SYST_RVR> SYST_RVR;
SysReg<SysRegName::SYST_CVR> SYST_CVR;
SysReg<SysRegName::SYST_CALIB> SYST_CALIB;
CPUID_Class CPUID;
ICSR_Class ICSR;
SysReg<SysRegName::VTOR> VTOR;
CCR_Class CCR;
CLIDR_Class CLIDR;
CCSIDR_Class CCSIDR;
SysReg<SysRegName::CSSELR> CSSELR;
SysReg<SysRegName::ICIALLU> ICIALLU;
SysReg<SysRegName::DCISW> DCISW;
SHCSR_Class SHCSR;
CFSR_Class CFSR;
SysReg<SysRegName::HFSR> HFSR;
SysReg<SysRegName::MMFAR> MMFAR;
SysReg<SysRegName::BFAR> BFAR;
CPACR_Class CPACR;
NSACR_Class NSACR;
MPU_TYPE_Class MPU_TYPE;
MPU_CTRL_Class MPU_CTRL;
SysReg<SysRegName::SFSR> SFSR;
SysReg<SysRegName::SFAR> SFAR;
FPCCR_Class FPCCR;
CONTROL_Class CONTROL;
SysReg<SysRegName::PAC_KEY_P_0> PAC_KEY_P_0;
SysReg<SysRegName::PAC_KEY_P_1> PAC_KEY_P_1;
SysReg<SysRegName::PAC_KEY_P_2> PAC_KEY_P_2;
SysReg<SysRegName::PAC_KEY_P_3> PAC_KEY_P_3;
SysReg<SysRegName::PAC_KEY_U_0> PAC_KEY_U_0;
SysReg<SysRegName::PAC_KEY_U_1> PAC_KEY_U_1;
SysReg<SysRegName::PAC_KEY_U_2> PAC_KEY_U_2;
SysReg<SysRegName::PAC_KEY_U_3> PAC_KEY_U_3;
SysRegSet<SysRegSetName::NVIC_ICERn> NVIC_ICER;

} // namespace sysreg
} // namespace bootcode
#endif
