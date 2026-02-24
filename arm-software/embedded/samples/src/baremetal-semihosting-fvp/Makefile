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

# Check if the sample is run from ATfE source tree, thus the files are available
IN_TREE_FVP_CONFIG_DIR:=../../../fvp/config
ifneq ($(wildcard $(IN_TREE_FVP_CONFIG_DIR)),)
    FVP_CONFIG_DIR:=$(IN_TREE_FVP_CONFIG_DIR)
else
    FVP_CONFIG_DIR:=.
endif

IN_TREE_FVP_CONFIG_NAME:=../../../fvp/config/cortex-m85.cfg
ifneq ($(wildcard $(IN_TREE_FVP_CONFIG_NAME)),)
    FVP_CONFIG_NAME:=$(IN_TREE_FVP_CONFIG_NAME)
else
    FVP_CONFIG_NAME:=cortex-m85.cfg
endif

IN_TREE_FVP_CONFIG_NAME_AARCH64:=../../../fvp/config/v8a-aarch64.cfg
ifneq ($(wildcard $(IN_TREE_FVP_CONFIG_NAME_AARCH64)),)
    FVP_CONFIG_NAME_AARCH64:=$(IN_TREE_FVP_CONFIG_NAME_AARCH64)
else
    FVP_CONFIG_NAME_AARCH64:=v8a-aarch64.cfg
endif

IN_TREE_LIT_EXEC_FVP:=../../../arm-runtimes/test-support/lit-exec-fvp.py
ifneq ($(wildcard $(IN_TREE_LIT_EXEC_FVP)),)
    LIT_EXEC_FVP:=$(IN_TREE_LIT_EXEC_FVP)
else
    LIT_EXEC_FVP:=lit-exec-fvp.py
endif

IN_TREE_RUN_FVP:=../../../arm-runtimes/test-support/run_fvp.py
ifneq ($(wildcard $(IN_TREE_RUN_FVP)),)
    RUN_FVP:=$(IN_TREE_RUN_FVP)
else
    RUN_FVP:=run_fvp.py
endif

# From https://github.com/arm/arm-toolchain/blob/arm-software/arm-software/embedded/arm-multilib/json/variants/armebv6m_soft_nofp_size.json
# see BOOT_FLASH_ADDRESS, BOOT_FLASH_SIZE, etc properties.
# For A-profile example, see https://github.com/arm/arm-toolchain/blob/arm-software/arm-software/embedded/arm-multilib/json/variants/aarch64a_be.json
# For R-profile example, see https://github.com/arm/arm-toolchain/blob/arm-software/arm-software/embedded/arm-multilib/json/variants/aarch64r_be.json
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
	# Download helper files and scripts to current folder if missing:
	# - Not available in tree, and
	# - Not yet downloaded to the current folder.
	@if [ ! -f $(LIT_EXEC_FVP) ]; then \
		wget https://raw.githubusercontent.com/arm/arm-toolchain/refs/heads/arm-software/arm-software/embedded/arm-runtimes/test-support/lit-exec-fvp.py; \
	fi
	@if [ ! -f $(RUN_FVP) ]; then \
		wget https://raw.githubusercontent.com/arm/arm-toolchain/refs/heads/arm-software/arm-software/embedded/arm-runtimes/test-support/run_fvp.py; \
	fi
	@if [ ! -f $(FVP_CONFIG_NAME) ]; then \
		wget https://raw.githubusercontent.com/arm/arm-toolchain/refs/heads/arm-software/arm-software/embedded/fvp/config/cortex-m85.cfg; \
	fi
	@if [ ! -f $(FVP_CONFIG_NAME_AARCH64) ]; then \
		wget https://raw.githubusercontent.com/arm/arm-toolchain/refs/heads/arm-software/arm-software/embedded/fvp/config/v8a-aarch64.cfg; \
	fi

run: setup hello.elf
# From https://github.com/arm/arm-toolchain/blob/arm-software/arm-software/embedded/arm-multilib/json/variants/armebv6m_soft_nofp_size.json
# see FVP_MODEL and FVP_CONFIG properties.
	python3 $(LIT_EXEC_FVP) --verbose \
		--fvp-install-dir=$(FVP_INSTALL_DIR) \
		--fvp-config-dir=$(FVP_CONFIG_DIR) \
		--fvp-model=corstone-310 \
		--fvp-config=cortex-m85 \
		hello.elf

# AArch64 build and run command

AARCH64_FVP_MEMORY_MAP=\
	-Wl,--defsym=__boot_flash=0x80000000 \
	-Wl,--defsym=__boot_flash_size=0x8000 \
	-Wl,--defsym=__flash=0x80008000 \
	-Wl,--defsym=__flash_size=0xff8000 \
	-Wl,--defsym=__ram=0x81000000 \
	-Wl,--defsym=__ram_size=0x1000000 \
	-Wl,--defsym=__stack_size=8K

hello-aarch64.elf: *.c
	$(BIN_PATH)/clang $(CFG_FILE) $(AARCH64_TARGET) $(CRT_SEMIHOST) -Wl,--pic-veneer -mno-unaligned-access -g $(AARCH64_FVP_MEMORY_MAP) -T $(LIBC_LD_FILE) -o hello-aarch64.elf $^

build-aarch64: hello-aarch64.elf

run-aarch64: setup build-aarch64
	python3 $(LIT_EXEC_FVP) --verbose \
		--fvp-install-dir=$(FVP_INSTALL_DIR) \
		--fvp-config-dir=$(FVP_CONFIG_DIR) \
		--fvp-model=aem-a \
		--fvp-config=v8a-aarch64 \
		hello-aarch64.elf

clean:
	rm -f *.elf
	rm -f lit-exec-fvp.py
	rm -f run_fvp.py
	rm -f cortex-m85.cfg
	rm -f v8a-aarch64.cfg
	rm -f :semihosting-features
	rm -fr __pycache__

.PHONY: clean setup run run-aarch64
