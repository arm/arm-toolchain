/* Copyright (c) 2023-2025, Arm Limited and affiliates. 
 *
 * Part of the Arm Toolchain project, under the Apache License v2.0 with LLVM Exceptions.
 * See https://llvm.org/LICENSE.txt for license information.
 * SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception */

#include <iostream>

class Base {
  public:
    virtual ~Base() {}
    virtual void print_type() {
      std::cout << "Base" << std::endl;
    }
};

class Good: public Base {
  public:
    void print_type() override {
      std::cout << "Good" << std::endl;
    }
};

class Bad { // not derived from Base
  public:
    virtual ~Bad() {}
    virtual void print_type() {
      std::cout << "Bad" << std::endl;
    }
};

int main(void) {
  Base* base_ptr = reinterpret_cast<Good*>(new Bad());
  base_ptr->print_type();

  std::cout << "C++ CFI sample" << std::endl;

  return 0;
}
