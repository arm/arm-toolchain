#!/usr/bin/env python3

# SPDX-FileCopyrightText: Copyright 2024-2025 Arm Limited and/or its affiliates <open-source-office@arm.com>

"""This script will generate a list of tests where the expected result in the
source files needs to be overridden via the lit command line or environment
variables.
It can also be used to track where downstream testing diverges from
upstream, and why."""

import argparse
import os
import re
import subprocess

from enum import Enum
from typing import Callable, NamedTuple, List


class NewResult(Enum):
    """Enum storing the potential new result a test."""

    XFAILED = "FAILED"  # Replace a failure with an expected failure.
    PASSED = "PASSED"  # Replace an unexpected pass with a pass.
    EXCLUDE = "EXCLUDE"  # Exclude a test, so that it is not run at all.


class XFail(NamedTuple):
    """Class to collect information about an xfail."""

    name: str  # Name to identify the xfail.
    testnames: List[str]  # The tests to include.
    result: NewResult  # The expected result.
    project: str  # Affected project.
    variants: List[str] = None  # Affected library variants, if applicable.
    conditional: Callable = None  # A function that will test whether an xfail applies.
    issue_link: str = None  # Optional link to a GitHub issue.
    description: str = None  # Optional field for notes.


def main():
    arg_parser = argparse.ArgumentParser(
        prog="xfailgen",
        description="A script that generates lit environment variables to xfail or filter tests.",
    )
    arg_parser.add_argument(
        "--variant",
        help="For library specific projects, the variant being tested.",
    )
    arg_parser.add_argument(
        "--libc",
        help="For library specific projects, the C library that was used.",
    )
    arg_parser.add_argument(
        "--clang",
        help="Path to clang for conditional testing.",
    )
    arg_parser.add_argument(
        "--project",
        required=True,
        help="Project to generate xfails for.",
    )
    arg_parser.add_argument(
        "--output-args",
        help="Write the test lists to a file with --xfail and --xfail-not"
        "parameters, which can be read directly by lit by prefixing with @.",
    )
    args = arg_parser.parse_args()

    # Test whether there is a multilib error from -frwpi
    def check_frwpi_error():
        test_args = [
            args.clang,
            "--print-multi-directory",
            "-target",
            "arm-none-eabi",
            "-frwpi",
        ]
        p = subprocess.run(test_args, capture_output=True, check=False)
        return p.returncode != 0

    # Test whether there is a multilib warning from -mcpu=cortex-r52
    def check_r52_warning():
        test_args = [
            args.clang,
            "--print-multi-directory",
            "-target",
            "arm-none-eabi",
            "-mcpu=cortex-r52",
            "-Werror",
        ]
        p = subprocess.run(test_args, capture_output=True, check=False)
        return p.returncode != 0

    def check_llvmlibc():
        return args.libc == "llvmlibc"

    def check_not_llvmlibc():
        return args.libc != "llvmlibc"

    xfails = [
        XFail(
            name="no frwpi",
            testnames=[
                "Driver/ropi-rwpi.c",
                "Preprocessor/arm-pic-predefines.c",
            ],
            result=NewResult.XFAILED,
            conditional=check_frwpi_error,
            project="clang",
            description="The multilib built by ATfE will generate a configuration error if -frwpi is used. Will pass if run before the multilib is installed.",
        ),
        XFail(
            name="no r52",
            testnames=[
                "Driver/arm-fpu-selection.s",
            ],
            result=NewResult.XFAILED,
            conditional=check_r52_warning,
            project="clang",
            description="If the installed default multilib does not have a library available for -mcpu=cortex-r52, this test will fail.",
        ),
        XFail(
            name="emulated crash signals",
            testnames=[
                "aarch64/emupac.c",
            ],
            result=NewResult.XFAILED,
            project="compiler-rt",
            variants=[
                "aarch64a",
                "aarch64a_be",
                "aarch64a_be_exn_rtti",
                "aarch64a_be_soft_nofp",
                "aarch64a_be_soft_nofp_exn_rtti",
                "aarch64a_exn_rtti",
                "aarch64a_exn_rtti_unaligned",
                "aarch64a_soft_nofp",
                "aarch64a_soft_nofp_exn_rtti",
                "aarch64a_unaligned",
                "aarch64r",
                "aarch64r_be",
                "aarch64r_be_exn_rtti",
                "aarch64r_be_soft_nofp",
                "aarch64r_be_soft_nofp_exn_rtti",
                "aarch64r_exn_rtti",
                "aarch64r_exn_rtti_unaligned",
                "aarch64r_soft_nofp",
                "aarch64r_soft_nofp_exn_rtti",
                "aarch64r_soft_nofp_exn_rtti_unaligned",
                "aarch64r_soft_nofp_unaligned",
                "aarch64r_unaligned",
            ],
            description="FVP and QEMU do not support crash signal handling",
        ),
        XFail(
            name="atomics part 1",
            testnames=[
                "libcxx/diagnostics/atomic.nodiscard.verify.cpp",
                "std/atomics/atomics.ref/address.pass.cpp",
                "std/atomics/atomics.ref/ctor.pass.cpp",
                "std/atomics/atomics.ref/deduction.pass.cpp",
                "std/atomics/atomics.ref/is_always_lock_free.pass.cpp",
                "std/atomics/atomics.types.generic/standard_layout.compile.pass.cpp",
                "std/atomics/atomics.types.operations/atomics.types.operations.req/dtor.pass.cpp",
                "std/atomics/types.pass.cpp",
            ],
            result=NewResult.XFAILED,
            project="libcxx",
            variants=[
                "armv4t_exn_rtti_size",
                "armv4t_size",
                "armv5te_exn_rtti_size",
                "armv5te_size",
                "armv6m_soft_nofp_exn_rtti_size",
                "armv6m_soft_nofp_size",
                "armebv6m_soft_nofp_size",
                "armebv6m_soft_nofp_exn_rtti_size",
            ],
            description="pmr missing or incomplete pstl",
        ),
        XFail(
            name="atomics part 2",
            testnames=[
                "libcxx/utilities/utility/mem.res/mem.poly.allocator.class/mem.poly.allocator.mem/construct_piecewise_pair.pass.cpp",
                "std/utilities/utility/mem.res/mem.poly.allocator.class/mem.poly.allocator.ctor/default.pass.cpp",
                "std/utilities/utility/mem.res/mem.poly.allocator.class/mem.poly.allocator.ctor/memory_resource_convert.pass.cpp",
                "std/utilities/utility/mem.res/mem.poly.allocator.class/mem.poly.allocator.eq/equal.pass.cpp",
                "std/utilities/utility/mem.res/mem.poly.allocator.class/mem.poly.allocator.eq/not_equal.pass.cpp",
                "std/utilities/utility/mem.res/mem.poly.allocator.class/mem.poly.allocator.mem/allocate.pass.cpp",
                "std/utilities/utility/mem.res/mem.poly.allocator.class/mem.poly.allocator.mem/allocate_deallocate_bytes.pass.cpp",
                "std/utilities/utility/mem.res/mem.poly.allocator.class/mem.poly.allocator.mem/allocate_deallocate_object.pass.cpp",
                "std/utilities/utility/mem.res/mem.poly.allocator.class/mem.poly.allocator.mem/construct_pair_const_lvalue_pair.pass.cpp",
                "std/utilities/utility/mem.res/mem.poly.allocator.class/mem.poly.allocator.mem/construct_pair_rvalue.pass.cpp",
                "std/utilities/utility/mem.res/mem.poly.allocator.class/mem.poly.allocator.mem/construct_pair_values.pass.cpp",
                "std/utilities/utility/mem.res/mem.poly.allocator.class/mem.poly.allocator.mem/construct_piecewise_pair.pass.cpp",
                "std/utilities/utility/mem.res/mem.poly.allocator.class/mem.poly.allocator.mem/construct_types.pass.cpp",
                "std/utilities/utility/mem.res/mem.poly.allocator.class/mem.poly.allocator.mem/deallocate.pass.cpp",
                "std/utilities/utility/mem.res/mem.poly.allocator.class/mem.poly.allocator.mem/new_delete_object.pass.cpp",
                "std/utilities/utility/mem.res/mem.res.global/default_resource.pass.cpp",
                "std/utilities/utility/mem.res/mem.res.monotonic.buffer/mem.res.monotonic.buffer.mem/allocate_exception_safety.pass.cpp",
                "std/utilities/utility/mem.res/mem.res.pool/mem.res.pool.mem/sync_deallocate_matches_allocate.pass.cpp",
                "std/utilities/utility/mem.res/mem.res.pool/mem.res.pool.mem/unsync_deallocate_matches_allocate.pass.cpp",
                "std/utilities/utility/mem.res/mem.res/mem.res.eq/equal.pass.cpp",
                "std/utilities/utility/mem.res/mem.res/mem.res.eq/not_equal.pass.cpp",
                "std/utilities/utility/mem.res/mem.res/mem.res.public/allocate.pass.cpp",
                "std/utilities/utility/mem.res/mem.res/mem.res.public/deallocate.pass.cpp",
                "std/utilities/utility/mem.res/mem.res/mem.res.public/dtor.pass.cpp",
                "std/utilities/utility/mem.res/mem.res/mem.res.public/is_equal.pass.cpp",
            ],
            result=NewResult.XFAILED,
            project="libcxx",
            variants=[
                "armebv6m_soft_nofp_size",
            ],
            description="pmr missing or incomplete pstl",
        ),
        XFail(
            name="atomics part 3",
            testnames=[
                "std/atomics/atomics.fences/atomic_signal_fence.pass.cpp",
                "std/atomics/atomics.fences/atomic_thread_fence.pass.cpp",
            ],
            result=NewResult.XFAILED,
            project="libcxx",
            variants=[
                "armv4t_exn_rtti_size",
                "armv4t_size",
                "armv5te_exn_rtti_size",
                "armv5te_size",
            ],
            description="pmr missing or incomplete pstl",
        ),
        XFail(
            name="atomics part 4",
            testnames=[
                "std/atomics/atomics.types.generic/cas_non_power_of_2.pass.cpp",
            ],
            result=NewResult.XFAILED,
            project="libcxx",
            variants=[
                "armv4t_exn_rtti_size",
                "armv4t_size",
                "armv5te_exn_rtti_size",
                "armv5te_size",
                "armv6m_soft_nofp_exn_rtti_size",
                "armv6m_soft_nofp_size",
                "armebv6m_soft_nofp_exn_rtti_size",
                "armebv6m_soft_nofp_size",
                "armv7m_hard_fpv4_sp_d16_exn_rtti_unaligned_size",
                "armv7m_hard_fpv4_sp_d16_size",
                "armv7m_hard_fpv4_sp_d16_unaligned_size",
                "armv7m_hard_fpv5_d16_exn_rtti_unaligned_size",
                "armv7m_hard_fpv5_d16_unaligned_size",
                "armv7m_soft_fpv4_sp_d16_exn_rtti_size",
                "armv7m_soft_fpv4_sp_d16_exn_rtti_unaligned_size",
                "armv7m_soft_fpv4_sp_d16_size",
                "armv7m_soft_fpv4_sp_d16_unaligned_size",
                "armv7m_soft_nofp_exn_rtti_unaligned_size",
                "armv7m_soft_nofp_size",
                "armv7m_soft_nofp_unaligned_size",
                "armebv7m_hard_fpv4_sp_d16_exn_rtti_size",
                "armebv7m_hard_fpv4_sp_d16_size",
                "armebv7m_soft_fpv4_sp_d16_exn_rtti_size",
                "armebv7m_soft_fpv4_sp_d16_size",
                "armebv7m_soft_nofp_exn_rtti_size",
                "armebv7m_soft_nofp_size",
                "armv8.1m.main_hard_fpdp_nomve_pacret_bti_exn_rtti_size",
                "armv8.1m.main_hard_fpdp_nomve_pacret_bti_exn_rtti_unaligned_size",
                "armv8.1m.main_hard_fpdp_nomve_pacret_bti_size",
                "armv8.1m.main_hard_fpdp_nomve_pacret_bti_unaligned_size",
                "armv8.1m.main_hard_fp_nomve_pacret_bti_exn_rtti_size",
                "armv8.1m.main_hard_fp_nomve_pacret_bti_exn_rtti_unaligned_size",
                "armv8.1m.main_hard_fp_nomve_pacret_bti_size",
                "armv8.1m.main_soft_nofp_nomve_pacret_bti_size",
                "armv8.1m.main_soft_nofp_nomve_pacret_bti_unaligned_size",
                "armv8.1m.main_soft_nofp_nomve_pacret_bti_exn_rtti_size",
                "armv8.1m.main_soft_nofp_nomve_pacret_bti_exn_rtti_unaligned_size",
                "armv8.1m.main_hard_fp_nomve_pacret_bti_unaligned_size",
                "armv8.1m.main_hard_nofp_mve_pacret_bti_size",
                "armv8.1m.main_hard_nofp_mve_pacret_bti_unaligned_size",
                "armv8.1m.main_hard_nofp_mve_pacret_bti_exn_rtti_size",
                "armv8.1m.main_hard_nofp_mve_pacret_bti_exn_rtti_unaligned_size",
            ],
            description="target lacks hardware CAS for non-power-of-two atomic object sizes",
        ),
        XFail(
            name="atomics clear padding on big-endian targets",
            testnames=[
                "libcxx/atomics/clear_padding.pass.cpp",
            ],
            result=NewResult.XFAILED,
            project="libcxx",
            variants=[
                "aarch64a_be",
                "aarch64a_be_exn_rtti",
                "aarch64a_be_soft_nofp",
                "aarch64a_be_soft_nofp_exn_rtti",
                "aarch64r_be",
                "aarch64r_be_exn_rtti",
                "aarch64r_be_soft_nofp",
                "aarch64r_be_soft_nofp_exn_rtti",
                "armebv6m_soft_nofp_exn_rtti_size",
                "armebv6m_soft_nofp_size",
                "armebv7a_hard_vfpv3_d16",
                "armebv7a_hard_vfpv3_d16_exn_rtti",
                "armebv7a_soft_nofp",
                "armebv7a_soft_nofp_exn_rtti",
                "armebv7a_soft_vfpv3_d16",
                "armebv7a_soft_vfpv3_d16_exn_rtti",
                "armebv7m_hard_fpv4_sp_d16_exn_rtti_size",
                "armebv7m_hard_fpv4_sp_d16_size",
                "armebv7m_soft_fpv4_sp_d16_exn_rtti_size",
                "armebv7m_soft_fpv4_sp_d16_size",
                "armebv7m_soft_nofp_exn_rtti_size",
                "armebv7m_soft_nofp_size",
            ],
            description="The atomic clear-padding test fails on big-endian targets.",
        ),
        XFail(
            name="old arm lock-free atomics",
            testnames=[
                "std/atomics/atomics.lockfree/is_always_lock_free.pass.cpp",
                "std/atomics/atomics.ref/assign.pass.cpp",
                "std/atomics/atomics.ref/bitwise_and_assign.pass.cpp",
                "std/atomics/atomics.ref/bitwise_or_assign.pass.cpp",
                "std/atomics/atomics.ref/bitwise_xor_assign.pass.cpp",
                "std/atomics/atomics.ref/compare_exchange_strong.pass.cpp",
                "std/atomics/atomics.ref/compare_exchange_weak.pass.cpp",
                "std/atomics/atomics.ref/convert.pass.cpp",
                "std/atomics/atomics.ref/exchange.pass.cpp",
                "std/atomics/atomics.ref/fetch_add.pass.cpp",
                "std/atomics/atomics.ref/fetch_and.pass.cpp",
                "std/atomics/atomics.ref/fetch_max.pass.cpp",
                "std/atomics/atomics.ref/fetch_min.pass.cpp",
                "std/atomics/atomics.ref/fetch_or.pass.cpp",
                "std/atomics/atomics.ref/fetch_sub.pass.cpp",
                "std/atomics/atomics.ref/fetch_xor.pass.cpp",
                "std/atomics/atomics.ref/increment_decrement.pass.cpp",
                "std/atomics/atomics.ref/load.pass.cpp",
                "std/atomics/atomics.ref/operator_minus_equals.pass.cpp",
                "std/atomics/atomics.ref/operator_plus_equals.pass.cpp",
                "std/atomics/atomics.ref/store.pass.cpp",
                "std/atomics/atomics.types.operations/atomics.types.operations.req/atomic_compare_exchange_strong.pass.cpp",
                "std/atomics/atomics.types.operations/atomics.types.operations.req/atomic_compare_exchange_strong_explicit.pass.cpp",
                "std/atomics/atomics.types.operations/atomics.types.operations.req/atomic_compare_exchange_weak.pass.cpp",
                "std/atomics/atomics.types.operations/atomics.types.operations.req/atomic_compare_exchange_weak_explicit.pass.cpp",
                "std/atomics/atomics.types.operations/atomics.types.operations.req/atomic_exchange.pass.cpp",
                "std/atomics/atomics.types.operations/atomics.types.operations.req/atomic_exchange_explicit.pass.cpp",
                "std/atomics/atomics.types.operations/atomics.types.operations.req/atomic_fetch_add.pass.cpp",
                "std/atomics/atomics.types.operations/atomics.types.operations.req/atomic_fetch_add_explicit.pass.cpp",
                "std/atomics/atomics.types.operations/atomics.types.operations.req/atomic_fetch_and.pass.cpp",
                "std/atomics/atomics.types.operations/atomics.types.operations.req/atomic_fetch_and_explicit.pass.cpp",
                "std/atomics/atomics.types.operations/atomics.types.operations.req/atomic_fetch_max.pass.cpp",
                "std/atomics/atomics.types.operations/atomics.types.operations.req/atomic_fetch_max_explicit.pass.cpp",
                "std/atomics/atomics.types.operations/atomics.types.operations.req/atomic_fetch_min.pass.cpp",
                "std/atomics/atomics.types.operations/atomics.types.operations.req/atomic_fetch_min_explicit.pass.cpp",
                "std/atomics/atomics.types.operations/atomics.types.operations.req/atomic_fetch_or.pass.cpp",
                "std/atomics/atomics.types.operations/atomics.types.operations.req/atomic_fetch_or_explicit.pass.cpp",
                "std/atomics/atomics.types.operations/atomics.types.operations.req/atomic_fetch_sub.pass.cpp",
                "std/atomics/atomics.types.operations/atomics.types.operations.req/atomic_fetch_sub_explicit.pass.cpp",
                "std/atomics/atomics.types.operations/atomics.types.operations.req/atomic_fetch_xor.pass.cpp",
                "std/atomics/atomics.types.operations/atomics.types.operations.req/atomic_fetch_xor_explicit.pass.cpp",
                "std/atomics/atomics.types.operations/atomics.types.operations.req/atomic_init.pass.cpp",
                "std/atomics/atomics.types.operations/atomics.types.operations.req/atomic_is_lock_free.pass.cpp",
                "std/atomics/atomics.types.operations/atomics.types.operations.req/atomic_load.pass.cpp",
                "std/atomics/atomics.types.operations/atomics.types.operations.req/atomic_load_explicit.pass.cpp",
                "std/atomics/atomics.types.operations/atomics.types.operations.req/atomic_store.pass.cpp",
                "std/atomics/atomics.types.operations/atomics.types.operations.req/atomic_store_explicit.pass.cpp",
                "std/atomics/atomics.types.operations/atomics.types.operations.req/ctor.pass.cpp",
            ],
            result=NewResult.XFAILED,
            project="libcxx",
            variants=[
                "armv4t_exn_rtti_size",
                "armv4t_size",
                "armv5te_exn_rtti_size",
                "armv5te_size",
                "armv6m_soft_nofp_exn_rtti_size",
                "armv6m_soft_nofp_size",
                "armebv6m_soft_nofp_exn_rtti_size",
                "armebv6m_soft_nofp_size",
            ],
            description="Old Arm targets rely on the atomic fallback library for these operations; the tests require hardware lock-free atomics.",
        ),
        XFail(
            name="alg.exponential",
            testnames=[
                "std/re/re.alg/re.alg.match/exponential.pass.cpp",
                "std/re/re.alg/re.alg.search/exponential.pass.cpp",
            ],
            result=NewResult.XFAILED,
            project="libcxx",
            variants=[
                "armv6m_soft_nofp_exn_rtti_size",
                "armv7m_hard_fpv4_sp_d16_exn_rtti_size",
                "armv7m_hard_fpv4_sp_d16_exn_rtti_unaligned_size",
                "armv7m_hard_fpv5_d16_exn_rtti_unaligned_size",
                "armv7m_soft_fpv4_sp_d16_exn_rtti_size",
                "armv7m_soft_fpv4_sp_d16_exn_rtti_unaligned_size",
                "armv7m_soft_nofp_exn_rtti_size",
                "armv7m_soft_nofp_exn_rtti_unaligned_size",
            ],
            description="test too large for embedded targets",
        ),
        XFail(
            name="demangle",
            testnames=[
                "test_demangle.pass.cpp",
            ],
            result=NewResult.PASSED,
            project="libcxx",
            variants=[
                "aarch64a_unaligned",
                "aarch64a_exn_rtti_unaligned",
                "aarch64a_soft_nofp",
                "aarch64a_soft_nofp_exn_rtti",
                "armebv6m_soft_nofp_exn_rtti_size",
                "armebv6m_soft_nofp_size",
                "armebv7a_hard_vfpv3_d16",
                "armebv7a_hard_vfpv3_d16_exn_rtti",
                "armebv7a_soft_nofp",
                "armebv7a_soft_nofp_exn_rtti",
                "armebv7a_soft_vfpv3_d16",
                "armebv7a_soft_vfpv3_d16_exn_rtti",
                "armebv7m_hard_fpv4_sp_d16_exn_rtti_size",
                "armebv7m_hard_fpv4_sp_d16_size",
                "armebv7m_soft_fpv4_sp_d16_exn_rtti_size",
                "armebv7m_soft_fpv4_sp_d16_size",
                "armebv7m_soft_nofp_exn_rtti_size",
                "armebv7m_soft_nofp_size",
                "armv4t_exn_rtti_size",
                "armv4t_size",
                "armv5te_exn_rtti_size",
                "armv5te_size",
                "armv6m_soft_nofp_exn_rtti_size",
                "armv6m_soft_nofp_size",
                "armv7a_hard_vfpv3_d16",
                "armv7a_hard_vfpv3_d16_exn_rtti",
                "armv7a_hard_vfpv3_d16_exn_rtti_unaligned",
                "armv7a_hard_vfpv3_d16_unaligned",
                "armv7a_soft_nofp",
                "armv7a_soft_nofp_exn_rtti",
                "armv7a_soft_nofp_exn_rtti_unaligned",
                "armv7a_soft_nofp_unaligned",
                "armv7a_soft_vfpv3_d16",
                "armv7a_soft_vfpv3_d16_exn_rtti",
                "armv7a_soft_vfpv3_d16_exn_rtti_unaligned",
                "armv7a_soft_vfpv3_d16_unaligned",
                "armv7m_hard_fpv4_sp_d16_exn_rtti_size",
                "armv7m_hard_fpv4_sp_d16_exn_rtti_unaligned_size",
                "armv7m_hard_fpv4_sp_d16_size",
                "armv7m_hard_fpv4_sp_d16_unaligned_size",
                "armv7m_hard_fpv5_d16_exn_rtti_unaligned_size",
                "armv7m_hard_fpv5_d16_unaligned_size",
                "armv7m_soft_fpv4_sp_d16_exn_rtti_size",
                "armv7m_soft_fpv4_sp_d16_exn_rtti_unaligned_size",
                "armv7m_soft_fpv4_sp_d16_size",
                "armv7m_soft_fpv4_sp_d16_unaligned_size",
                "armv7m_soft_nofp_exn_rtti_size",
                "armv7m_soft_nofp_exn_rtti_unaligned_size",
                "armv7m_soft_nofp_size",
                "armv7m_soft_nofp_unaligned_size",
                "armv7r_hard_vfpv3_d16",
                "armv7r_hard_vfpv3_d16_exn_rtti",
                "armv7r_hard_vfpv3_d16_exn_rtti_unaligned",
                "armv7r_hard_vfpv3_d16_unaligned",
                "armv7r_hard_vfpv3xd",
                "armv7r_hard_vfpv3xd_exn_rtti",
                "armv7r_hard_vfpv3xd_exn_rtti_unaligned",
                "armv7r_hard_vfpv3xd_unaligned",
                "armv7r_soft_nofp",
                "armv7r_soft_nofp_exn_rtti",
                "armv7r_soft_nofp_exn_rtti_unaligned",
                "armv7r_soft_nofp_unaligned",
                "armv7r_soft_vfpv3xd",
                "armv7r_soft_vfpv3xd_exn_rtti",
                "armv7r_soft_vfpv3xd_exn_rtti_unaligned",
                "armv7r_soft_vfpv3xd_unaligned",
                "armv8.1m.main_hard_fp_nomve_exn_rtti_unaligned",
                "armv8.1m.main_hard_fp_nomve_exn_rtti_unaligned_size",
                "armv8.1m.main_hard_fp_nomve_pacret_bti_exn_rtti_size",
                "armv8.1m.main_hard_fp_nomve_pacret_bti_exn_rtti_unaligned_size",
                "armv8.1m.main_hard_fp_nomve_pacret_bti_size",
                "armv8.1m.main_hard_fp_nomve_pacret_bti_unaligned_size",
                "armv8.1m.main_hard_fp_nomve_unaligned",
                "armv8.1m.main_hard_fp_nomve_unaligned_size",
                "armv8.1m.main_hard_fpdp_nomve_exn_rtti_unaligned",
                "armv8.1m.main_hard_fpdp_nomve_exn_rtti_unaligned_size",
                "armv8.1m.main_hard_fpdp_nomve_pacret_bti_exn_rtti_size",
                "armv8.1m.main_hard_fpdp_nomve_pacret_bti_exn_rtti_unaligned",
                "armv8.1m.main_hard_fpdp_nomve_pacret_bti_exn_rtti_unaligned_size",
                "armv8.1m.main_hard_fpdp_nomve_pacret_bti_size",
                "armv8.1m.main_hard_fpdp_nomve_pacret_bti_unaligned_size",
                "armv8.1m.main_hard_fpdp_nomve_unaligned",
                "armv8.1m.main_hard_fpdp_nomve_unaligned_size",
                "armv8.1m.main_hard_nofp_mve_exn_rtti_size",
                "armv8.1m.main_hard_nofp_mve_exn_rtti_unaligned",
                "armv8.1m.main_hard_nofp_mve_exn_rtti_unaligned_size",
                "armv8.1m.main_hard_nofp_mve_pacret_bti_exn_rtti_size",
                "armv8.1m.main_hard_nofp_mve_pacret_bti_exn_rtti_unaligned_size",
                "armv8.1m.main_hard_nofp_mve_pacret_bti_size",
                "armv8.1m.main_hard_nofp_mve_pacret_bti_unaligned_size",
                "armv8.1m.main_hard_nofp_mve_size",
                "armv8.1m.main_hard_nofp_mve_unaligned",
                "armv8.1m.main_hard_nofp_mve_unaligned_size",
                "armv8.1m.main_soft_nofp_nomve_exn_rtti_unaligned_size",
                "armv8.1m.main_soft_nofp_nomve_pacret_bti_exn_rtti_size",
                "armv8.1m.main_soft_nofp_nomve_pacret_bti_exn_rtti_unaligned",
                "armv8.1m.main_soft_nofp_nomve_pacret_bti_exn_rtti_unaligned_size",
                "armv8.1m.main_soft_nofp_nomve_pacret_bti_size",
                "armv8.1m.main_soft_nofp_nomve_pacret_bti_unaligned_size",
                "armv8.1m.main_soft_nofp_nomve_unaligned_size",
                "armv8m.main_hard_fp_exn_rtti_unaligned_size",
                "armv8m.main_hard_fp_unaligned_size",
                "armv8m.main_soft_nofp_exn_rtti_unaligned_size",
                "armv8m.main_soft_nofp_unaligned_size",
            ],
            description="Previously xfailed for being too large/slow, but is now passing.",
        ),
        XFail(
            name="demangle-fvp",
            testnames=[
                "test_demangle.pass.cpp",
            ],
            result=NewResult.XFAILED,
            project="libcxx",
            variants=[
                "aarch64a",
                "aarch64a_exn_rtti",
                "aarch64a_be",
                "aarch64a_be_exn_rtti",
                "aarch64a_be_soft_nofp",
                "aarch64a_be_soft_nofp_exn_rtti",
                "aarch64r",
                "aarch64r_unaligned",
                "aarch64r_soft_nofp",
                "aarch64r_soft_nofp_unaligned",
                "aarch64r_soft_nofp_exn_rtti",
                "aarch64r_soft_nofp_exn_rtti_unaligned",
                "aarch64r_be",
                "aarch64r_exn_rtti",
                "aarch64r_exn_rtti_unaligned",
                "aarch64r_be_exn_rtti",
                "aarch64r_be_soft_nofp",
                "aarch64r_be_soft_nofp_exn_rtti",
                "aarch64r_be_soft_nofp_exn_rtti_unaligned",
            ],
            description="FP literal test failure due to truncated long double exponent",
        ),
        XFail(
            name="uchar types",
            testnames=[
                "std/depr/depr.c.headers/uchar_h.compile.pass.cpp",
                "std/strings/c.strings/cuchar.compile.pass.cpp",
            ],
            result=NewResult.PASSED,
            project="libcxx",
            conditional=check_not_llvmlibc,
            description="More recent picolibc versions do now support char16_t and char32_t",
        ),
        XFail(
            name="picolibc_UID0_lookup",
            testnames=[
                "test-pwd.test",
                "test-grp.test",
            ],
            result=NewResult.EXCLUDE,
            project="picolibc",
            description="UID 0 lookup returning Null on MacOS",
        ),
        XFail(
            name="fstream assert",
            testnames=[
                "std/input.output/file.streams/fstreams/filebuf.virtuals/seekoff.pass.cpp",
                "std/input.output/file.streams/fstreams/filebuf.virtuals/setbuf.pass.cpp",
                "std/input.output/file.streams/fstreams/filebuf.virtuals/xsputn.pass.cpp",
                "std/input.output/file.streams/fstreams/ifstream.members/buffered_reads.pass.cpp",
                "std/input.output/file.streams/fstreams/ofstream.members/buffered_writes.pass.cpp",
            ],
            result=NewResult.XFAILED,
            project="libcxx",
            variants=[
                "armebv6m_soft_nofp_size",
                "armebv6m_soft_nofp_exn_rtti_size",
                "armebv7m_soft_nofp_size",
                "armebv7m_soft_nofp_exn_rtti_size",
                "armebv7m_soft_fpv4_sp_d16_size",
                "armebv7m_soft_fpv4_sp_d16_exn_rtti_size",
                "armebv7m_hard_fpv4_sp_d16_size",
                "armebv7m_hard_fpv4_sp_d16_exn_rtti_size",
                "armv8.1m.main_hard_fpdp_nomve_pacret_bti_unaligned_size",
                "armv8.1m.main_hard_fpdp_nomve_pacret_bti_exn_rtti_size",
                "armv8.1m.main_hard_fpdp_nomve_pacret_bti_exn_rtti_unaligned_size",
                "armv8.1m.main_hard_fpdp_nomve_pacret_bti_size",
                "armv8.1m.main_hard_fp_nomve_pacret_bti_size",
                "armv8.1m.main_hard_fp_nomve_pacret_bti_exn_rtti_size",
                "armv8.1m.main_hard_fp_nomve_pacret_bti_unaligned_size",
                "armv8.1m.main_hard_fp_nomve_pacret_bti_exn_rtti_unaligned_size",
                "armv8.1m.main_hard_nofp_mve_pacret_bti_exn_rtti_size",
                "armv8.1m.main_hard_nofp_mve_pacret_bti_exn_rtti_unaligned_size",
                "armv8.1m.main_hard_nofp_mve_pacret_bti_size",
                "armv8.1m.main_hard_nofp_mve_pacret_bti_unaligned_size",
                "armv8.1m.main_soft_nofp_nomve_pacret_bti_exn_rtti_size",
                "armv8.1m.main_soft_nofp_nomve_pacret_bti_exn_rtti_unaligned_size",
                "armv8.1m.main_soft_nofp_nomve_pacret_bti_size",
                "armv8.1m.main_soft_nofp_nomve_pacret_bti_unaligned_size",
            ],
            description="fstream assertion failures (seekoff/setbuf/xsputn/buffered IO.",
        ),
        XFail(
            name="128strconv part 1",
            testnames=[
                "std/strings/string.conversions/stold.pass.cpp",
                "std/strings/string.conversions/to_string.pass.cpp",
            ],
            result=NewResult.XFAILED,
            project="libcxx",
            variants=[
                "aarch64a",
                "aarch64a_exn_rtti",
                "aarch64a_unaligned",
                "aarch64a_soft_nofp",
                "aarch64a_soft_nofp_exn_rtti",
                "aarch64a_be",
                "aarch64a_be_exn_rtti",
                "aarch64a_be_soft_nofp",
                "aarch64a_be_soft_nofp_exn_rtti",
                "aarch64r",
                "aarch64r_unaligned",
                "aarch64r_exn_rtti",
                "aarch64r_exn_rtti_unaligned",
                "aarch64r_soft_nofp",
                "aarch64r_soft_nofp_exn_rtti",
                "aarch64r_soft_nofp_unaligned",
                "aarch64r_soft_nofp_exn_rtti_unaligned",
                "aarch64r_be",
                "aarch64r_be_exn_rtti",
                "aarch64r_be_soft_nofp",
                "aarch64r_be_soft_nofp_exn_rtti",
            ],
            conditional=check_not_llvmlibc,
            description="Broken conversion between 128-bit types and string.",
        ),
        XFail(
            name="128strconv part 2",
            testnames=[
                "std/input.output/iostream.format/output.streams/ostream.formatted/ostream.inserters.arithmetic/long_double.pass.cpp",
                "std/localization/locale.categories/category.monetary/locale.money.get/locale.money.get.members/get_long_double_overlong.pass.cpp",
                "std/localization/locale.categories/category.numeric/locale.nm.put/facet.num.put.members/put_long_double.pass.cpp",
            ],
            result=NewResult.XFAILED,
            project="libcxx",
            variants=[
                "aarch64a",
                "aarch64a_exn_rtti",
                "aarch64a_be",
                "aarch64a_be_exn_rtti",
                "aarch64a_be_soft_nofp",
                "aarch64a_be_soft_nofp_exn_rtti",
                "aarch64r_be",
                "aarch64r_be_exn_rtti",
                "aarch64r_be_soft_nofp",
                "aarch64r_be_soft_nofp_exn_rtti",
                "aarch64r",
                "aarch64r_unaligned",
                "aarch64r_exn_rtti",
                "aarch64r_exn_rtti_unaligned",
                "aarch64r_soft_nofp",
                "aarch64r_soft_nofp_unaligned",
                "aarch64r_soft_nofp_exn_rtti",
                "aarch64r_soft_nofp_exn_rtti_unaligned",
            ],
            conditional=check_not_llvmlibc,
            description="Broken conversion between 128-bit types and string.",
        ),
        XFail(
            name="128strconv part 3",
            testnames=[
                "std/localization/locale.categories/category.numeric/locale.nm.put/facet.num.put.members/put_long_double.hex.pass.cpp",
            ],
            result=NewResult.XFAILED,
            project="libcxx",
            variants=[
                "aarch64a",
                "aarch64a_exn_rtti",
                "aarch64a_be",
                "aarch64a_be_exn_rtti",
                "aarch64a_exn_rtti",
                "aarch64r_be",
                "aarch64r_be_exn_rtti",
                "aarch64r",
                "aarch64r_unaligned",
                "aarch64r_exn_rtti",
                "aarch64r_exn_rtti_unaligned",
                "aarch64r_soft_nofp",
                "aarch64r_soft_nofp_unaligned",
                "aarch64r_soft_nofp_exn_rtti",
                "aarch64r_soft_nofp_exn_rtti_unaligned",
            ],
            conditional=check_not_llvmlibc,
            description="Broken conversion between 128-bit types and string.",
        ),
        XFail(
            name="regex exponential abort",
            testnames=[
                "std/re/re.alg/re.alg.match/exponential.pass.cpp",
                "std/re/re.alg/re.alg.search/exponential.pass.cpp",
            ],
            result=NewResult.XFAILED,
            project="libcxx",
            variants=[
                "aarch64a_exn_rtti",
                "aarch64a_exn_rtti_unaligned",
                "aarch64a_soft_nofp_exn_rtti",
                "aarch64a_be_exn_rtti",
                "aarch64a_be_soft_nofp_exn_rtti",
                "aarch64r_exn_rtti",
                "aarch64r_exn_rtti_unaligned",
                "aarch64r_soft_nofp_exn_rtti",
                "aarch64r_soft_nofp_exn_rtti_unaligned",
                "aarch64r_be_exn_rtti",
                "aarch64r_be_soft_nofp_exn_rtti",
                "armv8m.main_soft_nofp_exn_rtti_unaligned_size",
            ],
            description="std::regex exponential match/search tests abort at runtime (exit status 134).",
        ),
        XFail(
            name="sqrt precision on softnofp",
            testnames=[
                "std/numerics/c.math/cmath.pass.cpp",
            ],
            result=NewResult.XFAILED,
            project="libcxx",
            variants=[
                "aarch64a_soft_nofp",
                "aarch64a_soft_nofp_exn_rtti",
                "aarch64a_be_soft_nofp",
                "aarch64a_be_soft_nofp_exn_rtti",
                "aarch64r_soft_nofp",
                "aarch64r_soft_nofp_unaligned",
                "aarch64r_soft_nofp_exn_rtti",
                "aarch64r_soft_nofp_exn_rtti_unaligned",
                "aarch64r_be_soft_nofp",
                "aarch64r_be_soft_nofp_exn_rtti",
            ],
            description="std::sqrt with long double gives incorrect results in soft nofp builds.",
        ),
        XFail(
            name="semihosting tick frequency on Corstone-310 ",
            testnames=[
                "std/time/time.clock/time.clock.file/now.pass.cpp",
                "std/time/time.clock/time.clock.hires/now.pass.cpp",
                "std/time/time.clock/time.clock.system/now.pass.cpp",
            ],
            result=NewResult.XFAILED,
            project="libcxx",
            variants=[
                "armebv6m_soft_nofp_exn_rtti_size",
                "armebv6m_soft_nofp_size",
                "armebv7m_hard_fpv4_sp_d16_exn_rtti_size",
                "armebv7m_hard_fpv4_sp_d16_size",
                "armebv7m_soft_fpv4_sp_d16_exn_rtti_size",
                "armebv7m_soft_fpv4_sp_d16_size",
                "armebv7m_soft_nofp_exn_rtti_size",
                "armebv7m_soft_nofp_size",
                "armv8.1m.main_hard_fp_nomve_pacret_bti_exn_rtti_size",
                "armv8.1m.main_hard_fp_nomve_pacret_bti_exn_rtti_unaligned_size",
                "armv8.1m.main_hard_fpdp_nomve_pacret_bti_exn_rtti_size",
                "armv8.1m.main_hard_fpdp_nomve_pacret_bti_exn_rtti_unaligned_size",
                "armv8.1m.main_hard_fp_nomve_pacret_bti_size",
                "armv8.1m.main_hard_fp_nomve_pacret_bti_unaligned_size",
                "armv8.1m.main_hard_fpdp_nomve_pacret_bti_size",
                "armv8.1m.main_hard_fpdp_nomve_pacret_bti_unaligned_size",
                "armv8.1m.main_hard_nofp_mve_pacret_bti_exn_rtti_size",
                "armv8.1m.main_hard_nofp_mve_pacret_bti_exn_rtti_unaligned_size",
                "armv8.1m.main_hard_nofp_mve_pacret_bti_size",
                "armv8.1m.main_hard_nofp_mve_pacret_bti_unaligned_size",
                "armv8.1m.main_soft_nofp_nomve_pacret_bti_exn_rtti_size",
                "armv8.1m.main_soft_nofp_nomve_pacret_bti_exn_rtti_unaligned_size",
                "armv8.1m.main_soft_nofp_nomve_pacret_bti_size",
                "armv8.1m.main_soft_nofp_nomve_pacret_bti_unaligned_size",
            ],
            description="semihosting tick frequency issues in Corstone-310 FVP (SDDKW-94045).",
        ),
        XFail(
            name="Sync Fault On AArch64 FVP",
            testnames=[
                "unwind_leaffunction.pass.cpp",
            ],
            result=NewResult.XFAILED,
            project="libcxx",
            variants=[
                "aarch64a",
                "aarch64a_exn_rtti",
                "aarch64a_be",
                "aarch64a_be_exn_rtti",
                "aarch64a_be_soft_nofp",
                "aarch64a_be_soft_nofp_exn_rtti",
                "aarch64r",
                "aarch64r_unaligned",
                "aarch64r_soft_nofp",
                "aarch64r_soft_nofp_unaligned",
                "aarch64r_soft_nofp_exn_rtti",
                "aarch64r_soft_nofp_exn_rtti_unaligned",
                "aarch64r_exn_rtti",
                "aarch64r_exn_rtti_unaligned",
                "aarch64r_be",
                "aarch64r_be_exn_rtti",
                "aarch64r_be_soft_nofp",
                "aarch64r_be_soft_nofp_exn_rtti",
            ],
            description="A synchronous fault during execution on AArch64 FVP.",
        ),
        XFail(
            name="Hard Fault On Armv8.1m.main",
            testnames=[
                "unw_resume.pass.cpp",
            ],
            result=NewResult.XFAILED,
            project="libcxx",
            variants=[
                "armv8.1m.main_hard_fp_nomve_pacret_bti_exn_rtti_size",
                "armv8.1m.main_hard_fp_nomve_pacret_bti_exn_rtti_unaligned_size",
                "armv8.1m.main_hard_fp_nomve_pacret_bti_size",
                "armv8.1m.main_hard_fp_nomve_pacret_bti_unaligned_size",
                "armv8.1m.main_hard_fpdp_nomve_pacret_bti_exn_rtti_size",
                "armv8.1m.main_hard_fpdp_nomve_pacret_bti_exn_rtti_unaligned_size",
                "armv8.1m.main_hard_fpdp_nomve_pacret_bti_size",
                "armv8.1m.main_hard_fpdp_nomve_pacret_bti_unaligned_size",
                "armv8.1m.main_hard_nofp_mve_pacret_bti_exn_rtti_size",
                "armv8.1m.main_hard_nofp_mve_pacret_bti_exn_rtti_unaligned_size",
                "armv8.1m.main_hard_nofp_mve_pacret_bti_size",
                "armv8.1m.main_hard_nofp_mve_pacret_bti_unaligned_size",
                "armv8.1m.main_soft_nofp_nomve_pacret_bti_exn_rtti_size",
                "armv8.1m.main_soft_nofp_nomve_pacret_bti_exn_rtti_unaligned_size",
                "armv8.1m.main_soft_nofp_nomve_pacret_bti_size",
                "armv8.1m.main_soft_nofp_nomve_pacret_bti_unaligned_size",
            ],
            description="A hard fault is observed during execution on Corstone-310 FVP targeting Armv8.1-M.",
        ),
        XFail(
            name="picolibc_sys_seek",
            testnames=[
                "semihost-seek.test",
                "test-fread-fwrite.test",
                "test-posix-io.test",
            ],
            result=NewResult.XFAILED,
            project="picolibc",
            variants=[
                "armebv6m_soft_nofp_exn_rtti_size",
                "armebv6m_soft_nofp_size",
                "armebv7m_hard_fpv4_sp_d16_exn_rtti_size",
                "armebv7m_hard_fpv4_sp_d16_size",
                "armebv7m_soft_fpv4_sp_d16_exn_rtti_size",
                "armebv7m_soft_fpv4_sp_d16_size",
                "armebv7m_soft_nofp_exn_rtti_size",
                "armebv7m_soft_nofp_size",
                "armv8.1m.main_hard_fp_nomve_pacret_bti_exn_rtti_size",
                "armv8.1m.main_hard_fp_nomve_pacret_bti_exn_rtti_unaligned_size",
                "armv8.1m.main_hard_fp_nomve_pacret_bti_size",
                "armv8.1m.main_hard_fp_nomve_pacret_bti_unaligned_size",
                "armv8.1m.main_hard_fpdp_nomve_pacret_bti_exn_rtti_size",
                "armv8.1m.main_hard_fpdp_nomve_pacret_bti_exn_rtti_unaligned_size",
                "armv8.1m.main_hard_fpdp_nomve_pacret_bti_size",
                "armv8.1m.main_hard_fpdp_nomve_pacret_bti_unaligned_size",
                "armv8.1m.main_hard_nofp_mve_pacret_bti_exn_rtti_size",
                "armv8.1m.main_hard_nofp_mve_pacret_bti_exn_rtti_unaligned_size",
                "armv8.1m.main_hard_nofp_mve_pacret_bti_size",
                "armv8.1m.main_hard_nofp_mve_pacret_bti_unaligned_size",
                "armv8.1m.main_soft_nofp_nomve_pacret_bti_exn_rtti_size",
                "armv8.1m.main_soft_nofp_nomve_pacret_bti_exn_rtti_unaligned_size",
                "armv8.1m.main_soft_nofp_nomve_pacret_bti_size",
                "armv8.1m.main_soft_nofp_nomve_pacret_bti_unaligned_size",
            ],
            description="SYS_SEEK returns wrong value when using FVPs (SDDKW-25808).",
        ),
        XFail(
            name="picolibc_rateInHz",
            testnames=[
                "semihost-gettimeofday.test",
                "test-stat.test",
                "test-posix-time.test",
            ],
            result=NewResult.XFAILED,
            project="picolibc",
            variants=[
                "armebv6m_soft_nofp_exn_rtti_size",
                "armebv6m_soft_nofp_size",
                "armebv7m_hard_fpv4_sp_d16_exn_rtti_size",
                "armebv7m_hard_fpv4_sp_d16_size",
                "armebv7m_soft_fpv4_sp_d16_exn_rtti_size",
                "armebv7m_soft_fpv4_sp_d16_size",
                "armebv7m_soft_nofp_exn_rtti_size",
                "armebv7m_soft_nofp_size",
                "armv8.1m.main_hard_fp_nomve_pacret_bti_exn_rtti_size",
                "armv8.1m.main_hard_fp_nomve_pacret_bti_exn_rtti_unaligned_size",
                "armv8.1m.main_hard_fp_nomve_pacret_bti_size",
                "armv8.1m.main_hard_fp_nomve_pacret_bti_unaligned_size",
                "armv8.1m.main_hard_fpdp_nomve_pacret_bti_exn_rtti_size",
                "armv8.1m.main_hard_fpdp_nomve_pacret_bti_exn_rtti_unaligned_size",
                "armv8.1m.main_hard_fpdp_nomve_pacret_bti_size",
                "armv8.1m.main_hard_fpdp_nomve_pacret_bti_unaligned_size",
                "armv8.1m.main_hard_nofp_mve_pacret_bti_exn_rtti_size",
                "armv8.1m.main_hard_nofp_mve_pacret_bti_exn_rtti_unaligned_size",
                "armv8.1m.main_hard_nofp_mve_pacret_bti_size",
                "armv8.1m.main_hard_nofp_mve_pacret_bti_unaligned_size",
                "armv8.1m.main_soft_nofp_nomve_pacret_bti_exn_rtti_size",
                "armv8.1m.main_soft_nofp_nomve_pacret_bti_exn_rtti_unaligned_size",
                "armv8.1m.main_soft_nofp_nomve_pacret_bti_size",
                "armv8.1m.main_soft_nofp_nomve_pacret_bti_unaligned_size",
            ],
            description="rateInHz port not connected in Corstone-310 FVP (SDDKW-95688).",
        ),
        XFail(
            name="string push back",
            testnames=[
                "std/strings/basic.string/string.modifiers/string_append/push_back.pass.cpp",
            ],
            result=NewResult.XFAILED,
            project="libcxx",
            variants=[
                "armv7m_hard_fpv5_d16_exn_rtti_unaligned_size",
                "armv7m_hard_fpv5_d16_unaligned_size",
            ],
            description="push_back crashes on a basic_string with an oversized value type",
        ),
        XFail(
            name="picolibc_serial_test",
            testnames=[
                "test-hello-raw.test",
                "test-hello-raw-no-flash.test",
            ],
            result=NewResult.EXCLUDE,
            project="picolibc",
            variants=[
                "aarch64a_exn_rtti_unaligned",
                "aarch64a_soft_nofp",
                "aarch64a_soft_nofp_exn_rtti",
                "aarch64a_unaligned",
            ],
            description="The test expects serial port activity to end the test and times out without it.",
        ),
        XFail(
            name="llvmlibc missing signal.h",
            testnames=[
                "extensions/libcxx/depr/depr.c.headers/extern_c.pass.cpp",
                "libcxx/assertions/default_verbose_abort.pass.cpp",
                "std/depr/depr.c.headers/stdint_h.pass.cpp",
                "std/language.support/cstdint/cstdint.syn/cstdint.pass.cpp",
            ],
            result=NewResult.XFAILED,
            project="libcxx",
            variants=[
                "aarch64a_exn_rtti",
                "armv7m_hard_fpv4_sp_d16_exn_rtti_size",
                "armv7m_soft_nofp_exn_rtti_size",
            ],
            conditional=check_llvmlibc,
            description="LLVM libc does not provide signal.h/csignal; the stdint/cstdint tests fail because they include csignal transitively.",
        ),
        XFail(
            name="llvmlibc missing file IO support",
            testnames=[
                "libcxx/input.output/file.streams/fstreams/fstream.close.pass.cpp",
                "libcxx/input.output/file.streams/fstreams/nodiscard.verify.cpp",
                "libcxx/input.output/filesystems/convert_file_time.pass.cpp",
                "std/input.output/file.streams/c.files/cstdio.pass.cpp",
                "std/input.output/file.streams/fstreams/filebuf.assign/member_swap.pass.cpp",
                "std/input.output/file.streams/fstreams/filebuf.assign/move_assign.pass.cpp",
                "std/input.output/file.streams/fstreams/filebuf.assign/nonmember_swap.pass.cpp",
                "std/input.output/file.streams/fstreams/filebuf.assign/nonmember_swap_min.pass.cpp",
                "std/input.output/file.streams/fstreams/filebuf.cons/default.pass.cpp",
                "std/input.output/file.streams/fstreams/filebuf.cons/move.pass.cpp",
                "std/input.output/file.streams/fstreams/filebuf.members/close.pass.cpp",
                "std/input.output/file.streams/fstreams/filebuf.members/open_path.pass.cpp",
                "std/input.output/file.streams/fstreams/filebuf.virtuals/pbackfail.pass.cpp",
                "std/input.output/file.streams/fstreams/filebuf.virtuals/seekoff.pass.cpp",
                "std/input.output/file.streams/fstreams/filebuf.virtuals/setbuf.pass.cpp",
                "std/input.output/file.streams/fstreams/filebuf.virtuals/xsputn.pass.cpp",
                "std/input.output/file.streams/fstreams/filebuf/types.pass.cpp",
                "std/input.output/file.streams/fstreams/fstream.assign/member_swap.pass.cpp",
                "std/input.output/file.streams/fstreams/fstream.assign/move_assign.pass.cpp",
                "std/input.output/file.streams/fstreams/fstream.assign/nonmember_swap.pass.cpp",
                "std/input.output/file.streams/fstreams/fstream.cons/default.pass.cpp",
                "std/input.output/file.streams/fstreams/fstream.cons/move.pass.cpp",
                "std/input.output/file.streams/fstreams/fstream.cons/path.pass.cpp",
                "std/input.output/file.streams/fstreams/fstream.cons/string.pass.cpp",
                "std/input.output/file.streams/fstreams/fstream.members/close.pass.cpp",
                "std/input.output/file.streams/fstreams/fstream.members/open_path.pass.cpp",
                "std/input.output/file.streams/fstreams/fstream.members/open_string.pass.cpp",
                "std/input.output/file.streams/fstreams/fstream.members/rdbuf.pass.cpp",
                "std/input.output/file.streams/fstreams/fstream/types.pass.cpp",
                "std/input.output/file.streams/fstreams/ifstream.assign/member_swap.pass.cpp",
                "std/input.output/file.streams/fstreams/ifstream.assign/move_assign.pass.cpp",
                "std/input.output/file.streams/fstreams/ifstream.assign/nonmember_swap.pass.cpp",
                "std/input.output/file.streams/fstreams/ifstream.cons/default.pass.cpp",
                "std/input.output/file.streams/fstreams/ifstream.cons/move.pass.cpp",
                "std/input.output/file.streams/fstreams/ifstream.cons/path.pass.cpp",
                "std/input.output/file.streams/fstreams/ifstream.cons/pointer.pass.cpp",
                "std/input.output/file.streams/fstreams/ifstream.cons/string.pass.cpp",
                "std/input.output/file.streams/fstreams/ifstream.members/buffered_reads.pass.cpp",
                "std/input.output/file.streams/fstreams/ifstream.members/close.pass.cpp",
                "std/input.output/file.streams/fstreams/ifstream.members/open_path.pass.cpp",
                "std/input.output/file.streams/fstreams/ifstream.members/open_pointer.pass.cpp",
                "std/input.output/file.streams/fstreams/ifstream.members/open_string.pass.cpp",
                "std/input.output/file.streams/fstreams/ifstream.members/rdbuf.pass.cpp",
                "std/input.output/file.streams/fstreams/ifstream.members/xsgetn.pass.cpp",
                "std/input.output/file.streams/fstreams/ifstream/types.pass.cpp",
                "std/input.output/file.streams/fstreams/ofstream.assign/member_swap.pass.cpp",
                "std/input.output/file.streams/fstreams/ofstream.assign/move_assign.pass.cpp",
                "std/input.output/file.streams/fstreams/ofstream.assign/nonmember_swap.pass.cpp",
                "std/input.output/file.streams/fstreams/ofstream.cons/default.pass.cpp",
                "std/input.output/file.streams/fstreams/ofstream.cons/move.pass.cpp",
                "std/input.output/file.streams/fstreams/ofstream.cons/path.pass.cpp",
                "std/input.output/file.streams/fstreams/ofstream.cons/string.pass.cpp",
                "std/input.output/file.streams/fstreams/ofstream.members/buffered_writes.pass.cpp",
                "std/input.output/file.streams/fstreams/ofstream.members/close.pass.cpp",
                "std/input.output/file.streams/fstreams/ofstream.members/open_path.pass.cpp",
                "std/input.output/file.streams/fstreams/ofstream.members/open_string.pass.cpp",
                "std/input.output/file.streams/fstreams/ofstream.members/rdbuf.pass.cpp",
                "std/input.output/file.streams/fstreams/ofstream/types.pass.cpp",
            ],
            result=NewResult.XFAILED,
            project="libcxx",
            variants=[
                "aarch64a_exn_rtti",
                "armv7m_hard_fpv4_sp_d16_exn_rtti_size",
                "armv7m_soft_nofp_exn_rtti_size",
            ],
            conditional=check_llvmlibc,
            description="LLVM libc for bare-metal does not provide FILE and filesystem support required by libc++ file I/O tests.",
        ),
        XFail(
            name="llvmlibc system_error category behavior",
            testnames=[
                "std/diagnostics/syserr/syserr.syserr/syserr.syserr.members/ctor_error_code.pass.cpp",
                "std/diagnostics/syserr/syserr.syserr/syserr.syserr.members/ctor_error_code_const_char_pointer.pass.cpp",
                "std/diagnostics/syserr/syserr.syserr/syserr.syserr.members/ctor_error_code_string.pass.cpp",
                "std/diagnostics/syserr/syserr.syserr/syserr.syserr.members/ctor_int_error_category.pass.cpp",
                "std/diagnostics/syserr/syserr.syserr/syserr.syserr.members/ctor_int_error_category_const_char_pointer.pass.cpp",
                "std/diagnostics/syserr/syserr.syserr/syserr.syserr.members/ctor_int_error_category_string.pass.cpp",
            ],
            result=NewResult.XFAILED,
            project="libcxx",
            variants=[
                "aarch64a_exn_rtti",
                "armv7m_hard_fpv4_sp_d16_exn_rtti_size",
                "armv7m_soft_nofp_exn_rtti_size",
            ],
            conditional=check_llvmlibc,
            description="LLVM libc system_error category and strerror text behavior differs from hosted libc++ expectations.",
        ),
        XFail(
            name="llvmlibc embedded executor stdin/runtime failures",
            testnames=[
                "std/containers/unord/unord.multiset/local_iterators.pass.cpp",
                "std/input.output/iostream.objects/narrow.stream.objects/cin.readmany.sh.cpp",
                "std/input.output/iostream.objects/narrow.stream.objects/cin.sh.cpp",
                "std/input.output/iostream.objects/narrow.stream.objects/cin.sync_with_stdio.sh.cpp",
            ],
            result=NewResult.EXCLUDE,
            project="libcxx",
            variants=[
                "aarch64a_exn_rtti",
                "armv7m_hard_fpv4_sp_d16_exn_rtti_size",
                "armv7m_soft_nofp_exn_rtti_size",
            ],
            conditional=check_llvmlibc,
            description="Runtime tests depend on stdin/runtime behavior that hangs with semihosting, thus EXCLUDED.",
        ),
        XFail(
            name="llvmlibc aarch64 missing long double atan2",
            testnames=[
                "std/numerics/complex.number/cmplx.over/arg.pass.cpp",
                "std/numerics/complex.number/complex.value.ops/arg.pass.cpp",
            ],
            result=NewResult.XFAILED,
            project="libcxx",
            variants=[
                "aarch64a_exn_rtti",
            ],
            conditional=check_llvmlibc,
            description="AArch64 LLVM libc uses long double for these complex arg tests and is missing atan2l.",
        ),
        XFail(
            name="llvmlibc missing quick_exit support",
            testnames=[
                "std/language.support/support.start.term/quick_exit.pass.cpp",
            ],
            result=NewResult.XFAILED,
            project="libcxx",
            variants=[
                "aarch64a_exn_rtti",
                "armv7m_hard_fpv4_sp_d16_exn_rtti_size",
                "armv7m_soft_nofp_exn_rtti_size",
            ],
            conditional=check_llvmlibc,
            description="LLVM libc does not provide at_quick_exit required by libc++ quick_exit tests.",
        ),
        XFail(
            name="llvmlibc disabled scanf floating point support",
            testnames=[
                "std/localization/locale.categories/category.monetary/locale.money.get/locale.money.get.members/get_long_double_overlong.pass.cpp",
            ],
            result=NewResult.XFAILED,
            project="libcxx",
            variants=[
                "aarch64a_exn_rtti",
                "armv7m_hard_fpv4_sp_d16_exn_rtti_size",
                "armv7m_soft_nofp_exn_rtti_size",
            ],
            conditional=check_llvmlibc,
            description="LLVM libc bare-metal disables scanf floating-point conversions with LIBC_CONF_SCANF_DISABLE_FLOAT, but libc++ money_get<long double> uses it.",
        ),
        XFail(
            name="llvmlibc stdlib upstream xfails that pass in ATfE",
            testnames=[
                "std/depr/depr.c.headers/stdlib_h.pass.cpp",
                "std/language.support/support.runtime/cstdlib.pass.cpp",
            ],
            result=NewResult.PASSED,
            project="libcxx",
            variants=[
                "aarch64a_exn_rtti",
                "armv7m_hard_fpv4_sp_d16_exn_rtti_size",
                "armv7m_soft_nofp_exn_rtti_size",
            ],
            conditional=check_llvmlibc,
            description="These tests are annotated with LLVM-LIBC-FIXME upstream for a missing system declaration, but ATfE's LLVM libc headers provide it.",
        ),
        XFail(
            name="llvmlibc executor stderr upstream xfail that passes in ATfE",
            testnames=[
                "selftest/dsl/dsl.sh.py",
            ],
            result=NewResult.PASSED,
            project="libcxx",
            variants=[
                "aarch64a_exn_rtti",
                "armv7m_hard_fpv4_sp_d16_exn_rtti_size",
                "armv7m_soft_nofp_exn_rtti_size",
            ],
            conditional=check_llvmlibc,
            description="This selftest is annotated with LLVM-LIBC-FIXME upstream for stderr/stdout conflation, but ATfE's executors route stderr separately.",
        ),
        XFail(
            name="variadic vector type arguments non-hermetic",
            testnames=[
                "src/__support/libc.test.src.__support.arg_list_test.__build__",
            ],
            result=NewResult.XFAILED,
            project="llvmlibc",
            variants=[
                "armv8.1m.main_hard_nofp_mve_exn_rtti_size",
                "armv8.1m.main_hard_nofp_mve_exn_rtti_unaligned",
                "armv8.1m.main_hard_nofp_mve_exn_rtti_unaligned_size",
                "armv8.1m.main_hard_nofp_mve_size",
                "armv8.1m.main_hard_nofp_mve_unaligned",
                "armv8.1m.main_hard_nofp_mve_unaligned_size",
            ],
            description="Clang ARM AAPCS mislowers variadic vector type arguments (LLVMAENG-6240)",
        ),
    ]

    tests_to_xfail = []
    tests_to_upass = []
    tests_to_exclude = []

    for xfail in xfails:
        if args.project != xfail.project:
            continue
        if xfail.variants is not None:
            if args.variant is None:
                raise ValueError(
                    f"--variant must be specified for project {args.project}"
                )
            if args.variant not in xfail.variants:
                continue
        if xfail.conditional is not None:
            if not xfail.conditional():
                continue
        if xfail.result == NewResult.XFAILED:
            tests_to_xfail.extend(xfail.testnames)
        elif xfail.result == NewResult.PASSED:
            tests_to_upass.extend(xfail.testnames)
        elif xfail.result == NewResult.EXCLUDE:
            tests_to_exclude.extend(xfail.testnames)

    tests_to_xfail.sort()
    tests_to_upass.sort()
    tests_to_exclude.sort()

    if args.output_args:
        os.makedirs(os.path.dirname(args.output_args), exist_ok=True)
        with open(args.output_args, "w", encoding="utf-8") as f:
            if len(tests_to_xfail) > 0:
                # --xfail and --xfail-not expect a comma separated list of test names.
                f.write("--xfail=")
                f.write(";".join(tests_to_xfail))
                f.write("\n")
            if len(tests_to_upass) > 0:
                f.write("--xfail-not=")
                f.write(";".join(tests_to_upass))
                f.write("\n")
            if len(tests_to_exclude) > 0:
                # --filter-out expects a regular expression to match any test names.
                escaped_testnames = [
                    re.escape(testname) for testname in tests_to_exclude
                ]
                f.write("--filter-out=")
                f.write("|".join(escaped_testnames))
                f.write("\n")
        print(f"xfail list written to {args.output_args}")
    else:
        if len(tests_to_xfail) > 0:
            print("xfailed tests:")
            for testname in tests_to_xfail:
                print(testname)
        if len(tests_to_upass) > 0:
            print("xfail removed from tests:")
            for testname in tests_to_upass:
                print(testname)
        if len(tests_to_exclude) > 0:
            print("excluded tests:")
            for testname in tests_to_exclude:
                print(testname)


if __name__ == "__main__":
    main()
