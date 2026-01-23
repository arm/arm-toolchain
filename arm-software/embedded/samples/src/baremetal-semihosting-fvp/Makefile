#
# Copyright (c) 2020-2026, Arm Limited and affiliates.
# Part of the Arm Toolchain project, under the Apache License v2.0 with LLVM Exceptions.
# See https://llvm.org/LICENSE.txt for license information.
# SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
#

include ../../Makefile.conf

# Needs FVPs installed, see https://github.com/arm/arm-toolchain/blob/arm-software/arm-software/embedded/fvp/get_fvps.sh
# By default, assume the script is running in ATfE source tree with get_fvps.sh script run to install FVPs
# in the default location.
ifndef FVP_INSTALL_DIR
    FVP_INSTALL_DIR=../../../fvp/install
endif

# From https://github.com/arm/arm-toolchain/blob/arm-software/arm-software/embedded/arm-multilib/json/variants/armebv6m_soft_nofp_size.json#L12
# For A-profile example, see https://github.com/arm/arm-toolchain/blob/arm-software/arm-software/embedded/arm-multilib/json/variants/aarch64a_be.json#L12
# For R-profile example, see https://github.com/arm/arm-toolchain/blob/arm-software/arm-software/embedded/arm-multilib/json/variants/aarch64r_be.json#L12
CORSTONE_FVP_MEMORY_MAP=\
-Wl,--defsym=__boot_flash=0x01000000 \
-Wl,--defsym=__boot_flash_size=2M \
-Wl,--defsym=__flash=0x60000000 \
-Wl,--defsym=__flash_size=0x1000000 \
-Wl,--defsym=__ram=0x61000000 \
-Wl,--defsym=__ram_size=0x1000000 \
-Wl,--defsym=__stack_size=4K

build: hello.elf

hello.elf: *.c
	$(BIN_PATH)/clang $(CFG_FILE) $(MICROBIT_OPTIONS) $(CRT_SEMIHOST) $(MATH_LIB) -g $(CORSTONE_FVP_MEMORY_MAP) -T $(LIBC_LD_FILE) -o hello.elf $^

setup:
	# Download helper files and scripts if missing
	@if [ ! -f lit-exec-fvp.py ]; then \
		wget https://raw.githubusercontent.com/arm/arm-toolchain/refs/heads/arm-software/arm-software/embedded/arm-runtimes/test-support/lit-exec-fvp.py; \
	fi
	@if [ ! -f run_fvp.py ]; then \
		wget https://raw.githubusercontent.com/arm/arm-toolchain/refs/heads/arm-software/arm-software/embedded/arm-runtimes/test-support/run_fvp.py; \
	fi
	@if [ ! -f cortex-m85.cfg ]; then \
		wget https://raw.githubusercontent.com/arm/arm-toolchain/refs/heads/arm-software/arm-software/embedded/fvp/config/cortex-m85.cfg; \
	fi

run: setup hello.elf
# From https://github.com/arm/arm-toolchain/blob/arm-software/arm-software/embedded/arm-multilib/json/variants/armebv6m_soft_nofp_size.json#L10
# In case of ATfE tree checkout, set --fvp-config-dir=$(FVP_INSTALL_DIR)/../config to use existing config files.
	python3 lit-exec-fvp.py --verbose --fvp-install-dir=$(FVP_INSTALL_DIR) --fvp-config-dir=. \
--fvp-model=corstone-310 \
--fvp-config=cortex-m85 \
hello.elf

clean:
	rm -f *.elf
	rm -f lit-exec-fvp.py
	rm -f run_fvp.py
	rm -f cortex-m85.cfg
	rm -f :semihosting-features
	rm -fr __pycache__

.PHONY: clean run debug
