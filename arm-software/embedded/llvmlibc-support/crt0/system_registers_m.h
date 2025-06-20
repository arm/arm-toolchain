//
// Copyright (c) 2025, Arm Limited and affiliates.
//
// Part of the Arm Toolchain project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//

// Definitions of m-profile system registers

#ifndef BOOTCODE_SYSTEM_REGISTERS_M_H
#define BOOTCODE_SYSTEM_REGISTERS_M_H

#include "system_registers_common.h"
#include <arm_acle.h>
#include <assert.h>

namespace bootcode {
namespace sysreg {

// System register names and the address each is located at
#define REGNAMES                                                               \
  REGNAME(ICTR, 0xE000E004)                                                    \
  REGNAME(SYST_CSR, 0xE000E010)                                                \
  REGNAME(SYST_RVR, 0xE000E014)                                                \
  REGNAME(SYST_CVR, 0xE000E018)                                                \
  REGNAME(SYST_CALIB, 0xE000E01C)                                              \
  REGNAME(CPUID, 0xE000ED00)                                                   \
  REGNAME(ICSR, 0xE000ED04)                                                    \
  REGNAME(VTOR, 0xE000ED08)                                                    \
  REGNAME(CCR, 0xE000ED14)                                                     \
  REGNAME(SHCSR, 0xE000ED24)                                                   \
  REGNAME(CFSR, 0xE000ED28)                                                    \
  REGNAME(HFSR, 0xE000ED2C)                                                    \
  REGNAME(MMFAR, 0xE000ED34)                                                   \
  REGNAME(BFAR, 0xE000ED38)                                                    \
  REGNAME(CPACR, 0xE000ED88)                                                   \
  REGNAME(NSACR, 0xE000ED8C)                                                   \
  REGNAME(MPU_CTRL, 0xE000ED94)                                                \
  REGNAME(SFSR, 0xE000EDE4)                                                    \
  REGNAME(SFAR, 0xE000EDE8)                                                    \
  REGNAME(FPCCR, 0xE000EF34)

enum class SysRegName {
#define REGNAME(X, Y) X,
  REGNAMES
#undef REGNAME
};

// The SysRegTraits template is specialised for each register name to define its
// address.
template <SysRegName Name> class SysRegTraits {};
#define REGNAME(X, Y)                                                          \
  template <> class SysRegTraits<SysRegName::X> {                              \
  public:                                                                      \
    static constexpr unsigned long Addr = Y;                                   \
  };
REGNAMES
#undef REGNAME

template <SysRegName Name> class SysReg : public SysRegBase<SysReg<Name>> {
private:
  static constexpr unsigned long Addr = SysRegTraits<Name>::Addr;

public:
  __attribute__((always_inline)) static unsigned long read() {
    return *(volatile unsigned long *)Addr;
  }

  __attribute__((always_inline)) static void write(unsigned long val) {
    *(volatile unsigned long *)Addr = val;
  }

  __attribute__((always_inline)) SysReg &operator=(unsigned long val) {
    write(val);
    return *this;
  }
};

// Register sets, the base address, and the maximum member index
#define REGSETNAMES REGNAME(NVIC_ICERn, 0xE000E180, 15)

enum class SysRegSetName {
#define REGNAME(X, Y, Z) X,
  REGSETNAMES
#undef REGNAME
};

// The SysRegSetTraits template is specialised for each register name to define
// its address and maximum index.
template <SysRegSetName Name> class SysRegSetTraits {};
#define REGNAME(X, Y, Z)                                                       \
  template <> class SysRegSetTraits<SysRegSetName::X> {                        \
  public:                                                                      \
    static constexpr unsigned long Addr = Y;                                   \
    static constexpr unsigned long Max = Z;                                    \
  };
REGSETNAMES
#undef REGNAME

template <SysRegSetName Name> class SysRegSet {
private:
  static constexpr unsigned long Addr = SysRegSetTraits<Name>::Addr;
  static constexpr unsigned long Max = SysRegSetTraits<Name>::Max;

public:
  __attribute__((always_inline)) volatile unsigned long &
  operator[](unsigned int idx) {
    assert(idx <= Max);
    return ((volatile unsigned long *)Addr)[idx];
  }
};

class CCR_Class : public SysReg<SysRegName::CCR> {
public:
  Bit<1> USERSETMPEND;
  Bit<3> UNALIGN_TRP;
  Bit<4> DIV_0_TRP;
  Bit<8> BFHFNMIGN;
  Bit<9> STKALIGN;
  Bit<10> STKOFHFNMIGN;
  Bit<16> DC;
  Bit<17> IC;
  Bit<18> BP;
  Bit<19> LOB;
  Bit<20> TRD;
};

class CFSR_Class : public SysReg<SysRegName::CFSR> {
public:
  Field<0, 7> MMFSR;
  Field<8, 15> BFSR;
  Field<16, 31> UFSR;
};

class SHCSR_Class : public SysReg<SysRegName::SHCSR> {
public:
  Bit<0> MEMFAULTACT;
  Bit<1> BUSFAULTACT;
  Bit<2> HARDFAULTACT;
  Bit<3> USGFAULTACT;
  Bit<4> SECUREFAULTACT;
  Bit<5> NMIACT;
  Bit<7> SVCALLACT;
  Bit<8> MONITORACT;
  Bit<10> PENDSVCACT;
  Bit<11> SYSTICKACT;
  Bit<12> USGFAULTPENDED;
  Bit<13> MEMFAULTPENDED;
  Bit<14> BUSFAULTPENDED;
  Bit<15> SVCALLPENDED;
  Bit<16> MEMFAULTENA;
  Bit<17> BUSFAULTENA;
  Bit<18> USGFAULTENA;
  Bit<19> SECUREFAULTENA;
  Bit<20> SECUREFAULTPENDED;
  Bit<21> HARDFAULTPENDED;
};

class MPU_CTRL_Class : public SysReg<SysRegName::MPU_CTRL> {
public:
  Bit<0> ENABLE;
  Bit<1> HFNMIENA;
  Bit<2> PRIVDEFENA;
};

class CPUID_Class : public SysReg<SysRegName::CPUID> {
public:
  Field<0, 3> Revision;
  Field<4, 15> PartNo;
  Field<16, 19> Architecture;
  Field<20, 23> Variant;
  Field<24, 31> Implementer;
};

class ICSR_Class : public SysReg<SysRegName::ICSR> {
public:
  Field<0, 8> VECTACTIVE;
  Bit<1> RETTOBASE;
  Field<12, 20> VECTPENDING;
  Bit<22> ISRPENDING;
  Bit<23> ISRPREEMPT;
  Bit<24> STTNS;
  Bit<25> PENDSTCLR;
  Bit<26> PENDSTSET;
  Bit<27> PENDSVCLR;
  Bit<30> PENDNMICLR;
  Bit<31> PENDNMISET;
};

class SYST_CSR_Class : public SysReg<SysRegName::SYST_CSR> {
public:
  Bit<0> ENABLE;
  Bit<1> TICKINT;
  Bit<2> CLKSOURCE;
  Bit<16> COUNTFLAG;
};

class ICTR_Class : public SysReg<SysRegName::ICTR> {
public:
  Field<0, 3> INTLINESNUM;
};

class CPACR_Class : public SysReg<SysRegName::CPACR> {
public:
  Field<0, 1> CP0;
  Field<2, 3> CP1;
  Field<4, 5> CP2;
  Field<6, 7> CP3;
  Field<8, 9> CP4;
  Field<10, 11> CP5;
  Field<12, 13> CP6;
  Field<14, 15> CP7;
  Field<20, 21> CP10;
  Field<22, 23> CP11;
};

class NSACR_Class : public SysReg<SysRegName::NSACR> {
public:
  Bit<0> CP0;
  Bit<1> CP1;
  Bit<2> CP2;
  Bit<3> CP3;
  Bit<4> CP4;
  Bit<5> CP5;
  Bit<6> CP6;
  Bit<7> CP7;
  Bit<10> CP10;
  Bit<11> CP11;
};

class FPCCR_Class : public SysReg<SysRegName::FPCCR> {
public:
  Bit<0> LSPACT;
  Bit<1> USER;
  Bit<3> THREAD;
  Bit<4> HFRDY;
  Bit<5> MMRDY;
  Bit<6> BFRDY;
  Bit<8> MONRDY;
  Bit<30> LSPEN;
  Bit<31> ASPEN;
};

ICTR_Class ICTR;
SYST_CSR_Class SYST_CSR;
SysReg<SysRegName::SYST_RVR> SYST_RVR;
SysReg<SysRegName::SYST_CVR> SYST_CVR;
SysReg<SysRegName::SYST_CALIB> SYST_CALIB;
CPUID_Class CPUID;
ICSR_Class ICSR;
SysReg<SysRegName::VTOR> VTOR;
CCR_Class CCR;
SHCSR_Class SHCSR;
CFSR_Class CFSR;
SysReg<SysRegName::HFSR> HFSR;
SysReg<SysRegName::MMFAR> MMFAR;
SysReg<SysRegName::BFAR> BFAR;
CPACR_Class CPACR;
NSACR_Class NSACR;
MPU_CTRL_Class MPU_CTRL;
SysReg<SysRegName::SFSR> SFSR;
SysReg<SysRegName::SFAR> SFAR;
FPCCR_Class FPCCR;
SysRegSet<SysRegSetName::NVIC_ICERn> NVIC_ICER;

} // namespace sysreg
} // namespace bootcode

#endif // BOOTCODE_SYSTEM_REGISTERS_M_H
