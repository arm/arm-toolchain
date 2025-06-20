//
// Copyright (c) 2025, Arm Limited and affiliates.
//
// Part of the Arm Toolchain project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//

// Definitions of a-profile system registers

#ifndef BOOTCODE_SYSTEM_REGISTERS_A_H
#define BOOTCODE_SYSTEM_REGISTERS_A_H

#include "system_registers_common.h"
#include <arm_acle.h>

namespace bootcode {
namespace sysreg {

// Registers that only have an EL0 version.
#define EL0_REGNAMES REGNAME(PMCCFILTR, "p15:0:c14:c15:7")

// Registers that only have an EL1 version.
#define EL1_REGNAMES                                                           \
  REGNAME(CLIDR, "p15:1:c0:c0:1")                                              \
  REGNAME(CSSELR, "p15:2:c0:c0:0")                                             \
  REGNAME(CCSIDR, "p15:1:c0:c0:0")                                             \
  REGNAME(ID_AA64PFR1, "NO_REGISTER")                                          \
  REGNAME(ID_DFR0, "p15:0:c0:c1:2")                                            \
  REGNAME(GCR, "NO_REGISTER")                                                  \
  REGNAME(DACR, "p15:0:c3:c0:0")                                               \
  REGNAME(CPACR, "p15:0:c1:c0:2")                                              \
  REGNAME(APIAKeyLo, "NO_REGISTER")                                            \
  REGNAME(APIAKeyHi, "NO_REGISTER")                                            \
  REGNAME(APIBKeyLo, "NO_REGISTER")                                            \
  REGNAME(APIBKeyHi, "NO_REGISTER")                                            \
  REGNAME(APDAKeyLo, "NO_REGISTER")                                            \
  REGNAME(APDAKeyHi, "NO_REGISTER")                                            \
  REGNAME(APDBKeyLo, "NO_REGISTER")                                            \
  REGNAME(APDBKeyHi, "NO_REGISTER")                                            \
  REGNAME(APGAKeyLo, "NO_REGISTER")                                            \
  REGNAME(APGAKeyHi, "NO_REGISTER")

// Registers that (in AArch64) have both an EL2 and an EL3 version. When these
// are accessed we choose the appropriate version based on CurrentEL.
#define EL23_REGNAMES                                                          \
  REGNAME(SCTLR, "p15:0:c1:c0:0")                                              \
  REGNAME(VBAR, "p15:0:c12:c0:0")                                              \
  REGNAME(ESR, "NO_REGISTER")                                                  \
  REGNAME(ELR, "NO_REGISTER")                                                  \
  REGNAME(FAR, "NO_REGISTER")                                                  \
  REGNAME(CPTR, "p15:4:c1:c1:2")                                               \
  REGNAME(TTBR0, "p15:0:c2:c0:0")                                              \
  REGNAME(MAIR, "p15:0:c10:c2:0")                                              \
  REGNAME(TCR, "p15:0:c2:c0:2")

enum class SysRegName {
#define REGNAME(X, Y) X,
  EL0_REGNAMES EL1_REGNAMES EL23_REGNAMES
#undef REGNAME
};

template <SysRegName Name> class SysReg : public SysRegBase<SysReg<Name>> {
public:
  // The system register read/write intrinsics need a string literal argument,
  // so we have to specialize the read/write functions for each register name.
  static unsigned long read();
  static void write(unsigned long val);
  __attribute__((always_inline)) SysReg &operator=(unsigned long val) {
    write(val);
    return *this;
  }
};

#ifdef __ARM_ARCH_ISA_A64

#define REGNAME(X, Y)                                                          \
  template <>                                                                  \
  __attribute__((always_inline)) inline unsigned long                          \
  SysReg<SysRegName::X>::read() {                                              \
    return __arm_rsr64(#X "_EL0");                                             \
  }                                                                            \
  template <>                                                                  \
  __attribute__((always_inline)) inline void SysReg<SysRegName::X>::write(     \
      unsigned long val) {                                                     \
    __arm_wsr64(#X "_EL0", val);                                               \
  }
EL0_REGNAMES
#undef REGNAME

#define REGNAME(X, Y)                                                          \
  template <>                                                                  \
  __attribute__((always_inline)) inline unsigned long                          \
  SysReg<SysRegName::X>::read() {                                              \
    return __arm_rsr64(#X "_EL1");                                             \
  }                                                                            \
  template <>                                                                  \
  __attribute__((always_inline)) inline void SysReg<SysRegName::X>::write(     \
      unsigned long val) {                                                     \
    __arm_wsr64(#X "_EL1", val);                                               \
  }
EL1_REGNAMES
#undef REGNAME

#define REGNAME(X, Y)                                                          \
  template <>                                                                  \
  __attribute__((always_inline)) inline unsigned long                          \
  SysReg<SysRegName::X>::read() {                                              \
    if (__arm_rsr("CurrentEL") == 3 << 2)                                      \
      return __arm_rsr64(#X "_EL3");                                           \
    else                                                                       \
      return __arm_rsr64(#X "_EL2");                                           \
  }                                                                            \
  template <>                                                                  \
  __attribute__((always_inline)) inline void SysReg<SysRegName::X>::write(     \
      unsigned long val) {                                                     \
    if (__arm_rsr("CurrentEL") == 3 << 2)                                      \
      __arm_wsr64(#X "_EL3", val);                                             \
    else                                                                       \
      __arm_wsr64(#X "_EL2", val);                                             \
  }
EL23_REGNAMES
#undef REGNAME

#else

#define REGNAME(X, Y)                                                          \
  template <>                                                                  \
  __attribute__((always_inline)) inline unsigned long                          \
  SysReg<SysRegName::X>::read() {                                              \
    return __arm_rsr(Y);                                                       \
  }                                                                            \
  template <>                                                                  \
  __attribute__((always_inline)) inline void SysReg<SysRegName::X>::write(     \
      unsigned long val) {                                                     \
    __arm_wsr(Y, val);                                                         \
  }
EL0_REGNAMES
EL1_REGNAMES
EL23_REGNAMES
#undef REGNAME

#endif

class SCTLR_Class : public SysReg<SysRegName::SCTLR> {
public:
  Bit<0> M;
  Bit<1> A;
  Bit<2> C;
  Bit<3> SA;
  Bit<6> nAA;
#if __ARM_ARCH_PROFILE == 'R'
  Bit<11> Z;
#else
  Bit<11> EOS;
#endif
  Bit<12> I;
  Bit<13> EnDB;
  Bit<19> WXN;
  Bit<21> IESB;
  Bit<22> EIS;
  Bit<25> EE;
  Bit<27> EnDA;
  Bit<30> EnIB;
  Bit<31> EnIA;
  Bit<36> BT;
  Bit<37> ITFSB;
  Field<40, 41> TCF;
  Bit<43> ATA;
  Bit<44> DSSBS;
  Bit<51> TMT;
  Bit<53> TME;
  Bit<61> NMI;
  Bit<62> SPINTMASK;
};

class CLIDR_Class : public SysReg<SysRegName::CLIDR> {
public:
  Field<0, 2> Ctype1;
  Field<3, 5> Ctype2;
  Field<6, 8> Ctype3;
  Field<9, 11> Ctype4;
  Field<12, 14> Ctype5;
  Field<15, 17> Ctype6;
  Field<18, 20> Ctype7;
  Field<21, 23> LoUIS;
  Field<24, 26> LoC;
  Field<27, 29> LoUU;
  Field<30, 32> ICB;
  Field<33, 46> Ttype;

  __attribute__((always_inline)) unsigned long Ctype(unsigned level) {
    unsigned long val = *this;
    return (val >> (3 * level)) & 0x7;
  }
};

class CCSIDR_Class : public SysReg<SysRegName::CCSIDR> {
public:
  Field<0, 2> LineSize;
  Field<3, 12> Associativity;
  Field<13, 27> NumSets;
};

class CPTR_Class : public SysReg<SysRegName::CPTR> {
public:
  Bit<8> EZ;
  Bit<10> TFP;
  Bit<12> ESM;
  Bit<20> TTA;
  Bit<30> TAM;
  Bit<31> TCPAC;
};

class GCR_Class : public SysReg<SysRegName::GCR> {
public:
  Field<0, 15> Exclude;
  Bit<16> RRND;
};

class DACR_Class : public SysReg<SysRegName::DACR> {
public:
  Field<0, 1> D0;
  Field<2, 3> D1;
  Field<4, 5> D2;
  Field<6, 7> D3;
  Field<8, 9> D4;
  Field<10, 11> D5;
  Field<12, 13> D6;
  Field<14, 15> D7;
  Field<16, 17> D8;
  Field<18, 19> D9;
  Field<20, 21> D10;
  Field<22, 23> D11;
  Field<24, 25> D12;
  Field<26, 27> D13;
  Field<28, 29> D14;
  Field<30, 31> D15;
};

class CPACR_Class : public SysReg<SysRegName::CPACR> {
public:
  Field<20, 21> CP10;
  Field<22, 23> CP11;
  Bit<28> TRCDIS;
  Bit<31> ASEDIS;
};

class PMCCFILTR_Class : public SysReg<SysRegName::PMCCFILTR> {
public:
  Bit<20> RLH;
  Bit<21> RLU;
  Bit<22> RLK;
  Bit<23> T;
  Bit<24> SH;
  Bit<26> M;
  Bit<27> NSH;
  Bit<28> NSU;
  Bit<29> NSK;
  Bit<30> U;
  Bit<31> P;
};

class ID_DFR0_Class : public SysReg<SysRegName::ID_DFR0> {
public:
  Field<0, 3> CopDbg;
  Field<4, 7> CopSDbg;
  Field<8, 11> MMapDbg;
  Field<12, 15> CopTrc;
  Field<16, 19> MMapTrc;
  Field<20, 23> MProfDbg;
  Field<24, 27> PerfMon;
  Field<28, 31> TraceFilt;
};

SCTLR_Class SCTLR;
CLIDR_Class CLIDR;
CCSIDR_Class CCSIDR;
CPTR_Class CPTR;
GCR_Class GCR;
DACR_Class DACR;
CPACR_Class CPACR;
PMCCFILTR_Class PMCCFILTR;
ID_DFR0_Class ID_DFR0;
SysReg<SysRegName::VBAR> VBAR;
SysReg<SysRegName::ESR> ESR;
SysReg<SysRegName::ELR> ELR;
SysReg<SysRegName::FAR> FAR;
SysReg<SysRegName::CSSELR> CSSELR;
SysReg<SysRegName::TTBR0> TTBR0;
SysReg<SysRegName::MAIR> MAIR;
SysReg<SysRegName::ID_AA64PFR1> ID_AA64PFR1;
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

#endif // BOOTCODE_SYSTEM_REGISTERS_A_H
