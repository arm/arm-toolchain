// This file ensures the compressed data ends up in rodata and we have a known address to provide the decompressor
.section .rodata
.globl compressed_rw_start
compressed_rw_start: .incbin "rw-compressed.bin"