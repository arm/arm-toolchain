# Bare-metal CFI sample

This sample shows how to use 
[Control Flow Integrity (CFI)](https://clang.llvm.org/docs/ControlFlowIntegrity.html)
sanitizer to detect certain kinds of undefined behavior 
that can subvert the control flow of C++ code at run-time.

The default target `make build` enables CFI, so that `make run` will encounter
a hard fault during execution and display a partial register dump, e.g:
```
ARM fault: hardfault
        R0:   0x00000001
        R1:   0x20000d3c
        R2:   0x2000093c
        R3:   0x00000000
        R12:  0x00000000
        LR:   0x00011407
        PC:   0x000115ec
        XPSR: 0x01000000
make: [Makefile:28: run] Error 1 (ignored)
```
_Note: The format of the dump may differ depending on the C library used._

Running `make build-no-cfi` followed by `make run` will demonstrate the
behavior without CFI, displaying a message as the undefined behaviour is not
caught:
```
Bad
C++ CFI sample
```
