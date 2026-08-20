# Bare-metal semihosting exceptions sample

This sample shows and tests that C++ exceptions and corresponding
library variant selection work correctly.

Running the sample using `make run` should compile and run two files,
`hello.hex` and `hello-exn.hex`.
`hello.hex`, compiled without exceptions, will print a message indicating the
exception was not caught:
```
No exceptions.
```
Whereas `hello-exn.hex` compiled with exceptions, will catch it:
```
Exception caught.
```
