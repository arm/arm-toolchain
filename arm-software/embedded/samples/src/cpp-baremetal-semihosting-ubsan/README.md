# Bare-metal UBSan sample

This sample shows how to use 
[UndefinedBehaviorSanitizer (UBSan)](https://clang.llvm.org/docs/UndefinedBehaviorSanitizer.html)
to detect undefined behavior in C++ code at run-time.

UBSan has two supported modes of operation: trap and minimal runtime, 
see `Makefile` for the respective command line options.

The default `make build` uses the minimal runtime mode. Running `make run`
will then display a message indicating the undefined behavior was
detected, e.g:
```
UBSAN: add-overflow (recovered)

C++ UBSan sample
```
Alternatively, `make build-trap` followed by `make run` will instead invoke
the trap mode, so that the same exits with a hardfault and partial register
dump, e.g:
```
ARM fault: hardfault
        R0:   0x00000001
        R1:   0x20000418
        R2:   0x20000018
        R3:   0x00000000
        R12:  0x00000000
        LR:   0x000031c7
        PC:   0x00003008
        XPSR: 0x01000000
make: *** [Makefile:26: run] Error 1
```
_Note: The format of the dump may differ depending on the C library used._
