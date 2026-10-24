/* Copyright (c) 2023-2026, Arm Limited and affiliates. 
 * 
 * Part of the Arm Toolchain project, under the Apache License v2.0 with LLVM Exceptions.
 * See https://llvm.org/LICENSE.txt for license information.
 * SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception */

/* ld script for the inner (execution-time) link of compression demo
 *
 * This ld script puts code and RO data at 0x3000 and RW data at 0x20000000,
 * and sets __data_size to 0 to prevent crt0 startup code from populating RAM,
 * expecting that the outer link will set everything up.
 *
 * However, one difference is that we must force the program header
 * table to contain precisely two PT_LOAD segments, so that elf2bin
 * will extract the data for use in the outer link in exactly the two
 * binary chunks we expect. If we don't do that, lld will potentially
 * split up one or the other segment into two pieces. It does this if
 * it has to insert alignment padding (e.g. between code and RO data),
 * apparently preferring not to specify arbitrary values for the
 * intervening bytes.
 */

INCLUDE memory_map.ld

PHDRS
{
    ro PT_LOAD;
    rw PT_LOAD;
    tls PT_TLS;
}

/* We must explicitly direct each section into the appropriate program header(s)
 * out of the ones defined above. */

SECTIONS
{
  .text __flash : { *(.text .text.*) } :ro
  PROVIDE(__data_start = ADDR(.data));
	PROVIDE(__data_source = LOADADDR(.data));
  .data __ram : { *(.data .data.*) } :rw
  .tdata : { *(.tdata .tdata.* .gnu.linkonce.td.*) } :rw :tls
  PROVIDE( __data_end = . );
  .tbss : { *(.tbss .tbss.* .gnu.linkonce.tb.*) *(.tcommon) } :rw :tls
  PROVIDE( __tls_align = MAX(ALIGNOF(.tdata), ALIGNOF(.tbss)) );
  PROVIDE( __data_size = __data_end - __data_start );
  .bss : { *(.bss .bss.*) } :rw
  __bss_end = .;
  PROVIDE( __bss_start = ADDR(.tbss) );
  PROVIDE( __arm32_tls_tcb_offset = MAX(8, __tls_align) );
  PROVIDE( __bss_size = __bss_end - __bss_start );
}

PROVIDE(__tls_base = ADDR(.tdata));

/*  Reserve the top 1 KiB of RAM for the stack. */
PROVIDE(__stack = __ram + __ram_size);
PROVIDE(__stack_size = 0x400);

/* The heap occupies free RAM between BSS and the stack reservation. */
PROVIDE(__heap_start = ALIGN(__bss_end, 8));
PROVIDE(__heap_end = __stack - __stack_size);
PROVIDE(__heap_size = __heap_end - __heap_start);

/* The decompressor will initialise .data so tell picolibc to not copy anything into .data's runtime address */
PROVIDE(__data_size = 0);
