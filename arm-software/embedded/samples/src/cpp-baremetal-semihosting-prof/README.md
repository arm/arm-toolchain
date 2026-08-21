# Code profiling and coverage sample

This sample shows how to use instrumentation to emit profile data
and use it to show code coverage.

Use `make run` to build with instrumentation, run and collect the raw profile data,
then output a visualization of the code coverage, e.g:
```
    1|       |/* Copyright (c) 2023-2025, Arm Limited and affiliates.
    2|       | *
    3|       | * Part of the Arm Toolchain project, under the Apache License v2.0 with LLVM Exceptions.
    4|       | * See https://llvm.org/LICENSE.txt for license information.
    5|       | * SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception */
    6|       |
    7|       |#include <iostream>
    8|       |#include <vector>
    9|       |
   10|       |int main()
   11|      1|{
   12|      1|    std::vector vec {1, 2, 3, 4, 5};
   13|       |
   14|      5|    for (auto num: vec) {
   15|      5|        std::cout << num << " ";
   16|      5|    }
   17|      1|    std::cout << std::endl;
   18|       |
   19|      1|    return 0;
   20|      1|}
```
