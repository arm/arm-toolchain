//
// Copyright (c) 2025, Arm Limited and affiliates.
//
// Part of the Arm Toolchain project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//

// Definitions common to both a-profile and m-profile system registers

#ifndef BOOTCODE_SYSTEM_REGISTERS_COMMON_H
#define BOOTCODE_SYSTEM_REGISTERS_COMMON_H

namespace bootcode {
namespace sysreg {

// Common base class of the architecture-specific SysReg classes.
// C is the SysReg class, and is expected to implement the read and write
// methods.
template <class C> class SysRegBase {
public:
  __attribute__((always_inline)) operator unsigned long() { return C::read(); }

  template <int start, int end> class Field {
  public:
    __attribute__((always_inline)) operator unsigned long() {
      unsigned long reg = C::read();
      reg >>= start;
      reg &= (1UL << (end - start + 1)) - 1;
      return reg;
    }

    __attribute__((always_inline)) Field &operator=(unsigned long val) {
      unsigned long reg = C::read();
      unsigned long mask = ((1UL << (end - start + 1)) - 1) << start;
      reg &= ~mask;
      reg |= mask & (val << start);
      C::write(reg);
      return *this;
    }
  };

  template <int idx> class Bit : public Field<idx, idx> {
  public:
    __attribute__((always_inline)) Bit &operator=(unsigned long val) {
      Field<idx, idx>::operator=(val);
      return *this;
    }
  };
};

} // namespace sysreg
} // namespace bootcode

#endif // BOOTCODE_SYSTEM_REGISTERS_COMMON_H
