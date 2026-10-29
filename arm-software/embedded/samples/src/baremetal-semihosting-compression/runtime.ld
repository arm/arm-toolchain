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
    boot PT_LOAD;
    ro PT_LOAD;
    rw PT_LOAD;
    tls PT_TLS;
}

/* We must explicitly direct each section into the appropriate program header(s)
 * out of the ones defined above. */

SECTIONS
{
  /* This is required to run the uncompressed version but won't be included in
   * the compressed image */
  .boot_flash : {
    KEEP (*(.text.init.enter))
  } :boot

  /* We'll be extracting the binary contents from this point forward */
  
  . =  __flash;
  .text : { *(.text .text.*) } :ro
  .rodata : { *(.rodata .rodata.* ) } : ro

  . =  __ram;
  PROVIDE(__data_start = ADDR(.data));
	PROVIDE(__data_source = LOADADDR(.data));
  .data : { *(.data .data.*) } :rw
  .tdata : { *(.tdata .tdata.* .gnu.linkonce.td.*) } :rw :tls
  PROVIDE( __data_end = . );
  PROVIDE( __data_size = __data_end - __data_start );
  .tbss : { *(.tbss .tbss.* .gnu.linkonce.tb.*) *(.tcommon) } :rw :tls
  PROVIDE( __tls_end = . );
  PROVIDE( __tbss_end = . );

  .bss : {
    . = ALIGN(__tls_align);
    PROVIDE( __tls_tail_extra_start = . );
		KEEP (*(.tls_tail_extra .tls_tail_extra.*))
		PROVIDE( __tls_tail_extra_end = . );

    PROVIDE( __non_tls_bss_start = . );
    *(.bss .bss.*)

    /* Align the heap */
    . = ALIGN(8);
    __bss_end = .;
  } :rw
  PROVIDE( __bss_start = ADDR(.tbss) );
  PROVIDE( __arm32_tls_tcb_offset = MAX(8, __tls_align) );
  PROVIDE( __bss_size = __bss_end - __bss_start );

  /DISCARD/ : {
    *(.note .note.*)
    *(.eh_frame .eh_frame.*)
    *(.ARM.extab* .gnu.linkonce.armextab.*)
    *(.ARM.exidx*)
  }
}

PROVIDE( __tls_first = SIZEOF(.tdata) ? ADDR(.tdata) : ADDR(.tbss) );
PROVIDE( __tls_base = ADDR(.tdata));
PROVIDE( __tls_align = MAX(ALIGNOF(.tdata), ALIGNOF(.tbss)) );
PROVIDE( __tls_size = (__tls_tail_extra_size ? ALIGN(__tls_end, __tls_align) + __tls_tail_extra_size : __tls_end) - __tls_base );
PROVIDE( __tls_tail_extra_size = __tls_tail_extra_end - __tls_tail_extra_start );

/* The heap occupies free RAM between BSS and the stack reservation. */
PROVIDE(__heap_start = ALIGN(__bss_end, 8));
PROVIDE(__heap_end = __stack - __stack_size);
PROVIDE(__heap_size = __heap_end - __heap_start);

/* Symbols expected by llvmlibc */
PROVIDE(__llvm_libc_heap_limit = __heap_end);
PROVIDE(_end = __heap_start);

/*  Reserve the top 1 KiB of RAM for the stack. */
PROVIDE(__stack = __ram + __ram_size);
PROVIDE(__stack_size = 0x400);
