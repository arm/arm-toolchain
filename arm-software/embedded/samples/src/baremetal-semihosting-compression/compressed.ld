/* Copyright (c) 2023-2026, Arm Limited and affiliates. 
 * 
 * Part of the Arm Toolchain project, under the Apache License v2.0 with LLVM Exceptions.
 * See https://llvm.org/LICENSE.txt for license information.
 * SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception */

/* ld script for the outermost (load-time) link of compression demo */

INCLUDE memory_map.ld

PHDRS
{
    ro PT_LOAD;
}

SECTIONS
{
  /*
   * Our .text section begins with the read-only PT_LOAD segment from
   * the runtime link (which in turn contains both code and RO data), at
   * exactly the same start address. Therefore, it needs no copying or
   * modification at load time.
   *
   * After that we include everything needed by the load-time image:
   * the entry point, the decompressor, and the compressed RW data.
   */
   . = __flash;
  .text : {
    KEEP(ro.o(.text))
    *(.text)
    rw-compressed.o(.rodata)
  } :ro

  /*
   * Discard the .ARM.exidx section from the decompressor object file,
   * which we don't need. If you don't do this, then lld defaults to
   * putting that at the very start of .text, causing the inner
   * image's RO segment to appear at the wrong offset.
   */
  /DISCARD/ : {
    *(.ARM.exidx)
  }
}

/* Address that we want to decompress the RW data to. */
PROVIDE(decompressed_rw_start = __ram);

/* Starting value for the stack pointer. */
PROVIDE(__stack = __ram + __ram_size);
