# Bare-metal semihosting FVP sample

This sample shows how to use semihosting with a
[Corstone IoT FVP (Fixed Virtual Platform)](https://developer.arm.com/Tools%20and%20Software/Fixed%20Virtual%20Platforms/IoT%20FVPs).

This sample is supported on Linux only. It relies on Corstone-310 FVP to be
installed, you can use the
[`get_fvps.sh`](https://github.com/arm/arm-toolchain/blob/arm-software/arm-software/embedded/fvp/get_fvps.sh)
script from the ATfE repository to install this and other FVPs.

The sample requires the `FVP_INSTALL_DIR` variable to be set in the environment
or on the make command line, to find the FVP. By default, it assumes that the
ATfE source tree is checked out and the FVPs are installed using the
`get_fvps.sh` script above.

The sample makes use of ATfE multilib variants
[JSON files](https://github.com/arm/arm-toolchain/tree/arm-software/arm-software/embedded/arm-multilib/json/variants)
to get example settings for the memory map, FVP model and required configuration
files.

The sample also downloads and uses ATfE multilib testing Python wrapper scripts
from ATfE repository to construct the full FVP command line. The actual command
line is printed before the invocation so that it can be inspected and reused.

Running the sample using `make run` should print a message:
```
Hello World!
```
