#!/usr/bin/env python3

# SPDX-FileCopyrightText: Copyright 2024-2025 Arm Limited and/or its affiliates <open-source-office@arm.com>

"""This script will generate a list of tests where the expected result in the
source files needs to be overridden via the lit command line or environment
variables.
It can also be used to track where downstream testing diverges from
upstream, and why."""

import argparse
import os
import subprocess

from enum import Enum
from typing import Callable, NamedTuple, List


class NewResult(Enum):
    """Enum storing the potential new result a test."""

    XFAILED = "FAILED"  # Replace a failure with an expected failure.
    PASSED = "PASSED"  # Replace an unexpected pass with a pass.


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
        "--xfails_file",
        help="Save the test list to a file, instead of outputting.",
    )
    arg_parser.add_argument(
        "--xfails_not_file",
        help="Save the test list to a file, instead of outputting.",
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

    xfails = [
        XFail(
            name="no frwpi",
            testnames=[
                "Driver/ropi-rwpi.c",
                "Preprocessor/arm-pic-predefines.c",
            ],
            result=NewResult.XFAILED,
            conditional=check_frwpi_error,
            project="llvm",
            description="The multilib built by ATfE will generate a configuration error if -frwpi is used. Will pass if run before the multilib is installed.",
        ),
        XFail(
            name="no r52",
            testnames=[
                "Driver/arm-fpu-selection.s",
            ],
            result=NewResult.XFAILED,
            conditional=check_r52_warning,
            project="llvm",
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
                "extensions/libcxx/atomics/atomics.flag/init_bool.pass.cpp",
                "libcxx/diagnostics/atomic.nodiscard.verify.cpp",
                "libcxx/thread/thread.stoptoken/intrusive_shared_ptr.pass.cpp",
                "libcxx/utilities/utility/mem.res/mem.res.monotonic.buffer/mem.res.monotonic.buffer.mem/allocate_from_underaligned_buffer.pass.cpp",
                "libcxx/utilities/utility/mem.res/mem.res.monotonic.buffer/mem.res.monotonic.buffer.mem/allocate_in_geometric_progression.pass.cpp",
                "libcxx/utilities/utility/mem.res/mem.res.pool/unsynchronized_buffer.pass.cpp",
                "std/atomics/atomics.flag/atomic_flag_clear.pass.cpp",
                "std/atomics/atomics.flag/atomic_flag_clear_explicit.pass.cpp",
                "std/atomics/atomics.flag/atomic_flag_test.pass.cpp",
                "std/atomics/atomics.flag/atomic_flag_test_and_set.pass.cpp",
                "std/atomics/atomics.flag/atomic_flag_test_and_set_explicit.pass.cpp",
                "std/atomics/atomics.flag/atomic_flag_test_explicit.pass.cpp",
                "std/atomics/atomics.flag/clear.pass.cpp",
                "std/atomics/atomics.flag/default.pass.cpp",
                "std/atomics/atomics.flag/init.pass.cpp",
                "std/atomics/atomics.flag/test_and_set.pass.cpp",
                "std/atomics/atomics.general/replace_failure_order.pass.cpp",
                "std/atomics/atomics.ref/address.pass.cpp",
                "std/atomics/atomics.ref/ctor.pass.cpp",
                "std/atomics/atomics.ref/deduction.pass.cpp",
                "std/atomics/atomics.ref/is_always_lock_free.pass.cpp",
                "std/atomics/atomics.types.generic/address.pass.cpp",
                "std/atomics/atomics.types.generic/bool.pass.cpp",
                "std/atomics/atomics.types.generic/standard_layout.compile.pass.cpp",
                "std/atomics/atomics.types.operations/atomics.types.operations.req/atomic_var_init.pass.cpp",
                "std/atomics/atomics.types.operations/atomics.types.operations.req/dtor.pass.cpp",
                "std/atomics/types.pass.cpp",
                "std/containers/container.adaptors/flat.map/flat.map.cons/deduct_pmr.pass.cpp",
                "std/containers/container.adaptors/flat.map/flat.map.cons/pmr.pass.cpp",
                "std/containers/container.adaptors/flat.multimap/flat.multimap.cons/deduct_pmr.pass.cpp",
                "std/containers/container.adaptors/flat.multimap/flat.multimap.cons/pmr.pass.cpp",
                "std/containers/container.adaptors/flat.multiset/flat.multiset.cons/deduct_pmr.pass.cpp",
                "std/containers/container.adaptors/flat.multiset/flat.multiset.cons/pmr.pass.cpp",
                "std/containers/container.adaptors/flat.set/flat.set.cons/deduct_pmr.pass.cpp",
                "std/containers/container.adaptors/flat.set/flat.set.cons/pmr.pass.cpp",
                "std/input.output/string.streams/istringstream/istringstream.members/str.allocator_propagation.pass.cpp",
                "std/input.output/string.streams/ostringstream/ostringstream.members/str.allocator_propagation.pass.cpp",
                "std/input.output/string.streams/stringstream/stringstream.members/str.allocator_propagation.pass.cpp",
                "std/utilities/utility/mem.res/mem.poly.allocator.class/mem.poly.allocator.class.general/equality.pass.cpp",
                "std/utilities/utility/mem.res/mem.poly.allocator.class/mem.poly.allocator.mem/construct_pair.pass.cpp",
                "std/utilities/utility/mem.res/mem.poly.allocator.class/mem.poly.allocator.mem/construct_piecewise_pair_evil.pass.cpp",
                "std/utilities/utility/mem.res/mem.poly.allocator.class/mem.poly.allocator.mem/destroy.pass.cpp",
                "std/utilities/utility/mem.res/mem.poly.allocator.class/mem.poly.allocator.mem/resource.pass.cpp",
                "std/utilities/utility/mem.res/mem.poly.allocator.class/mem.poly.allocator.mem/select_on_container_copy_construction.pass.cpp",
                "std/utilities/utility/mem.res/mem.res.aliases/header_deque_synop.pass.cpp",
                "std/utilities/utility/mem.res/mem.res.aliases/header_deque_synop2.pass.cpp",
                "std/utilities/utility/mem.res/mem.res.aliases/header_forward_list_synop.pass.cpp",
                "std/utilities/utility/mem.res/mem.res.aliases/header_list_synop.pass.cpp",
                "std/utilities/utility/mem.res/mem.res.aliases/header_list_synop2.pass.cpp",
                "std/utilities/utility/mem.res/mem.res.aliases/header_map_synop.pass.cpp",
                "std/utilities/utility/mem.res/mem.res.aliases/header_map_synop2.pass.cpp",
                "std/utilities/utility/mem.res/mem.res.aliases/header_regex_synop.pass.cpp",
                "std/utilities/utility/mem.res/mem.res.aliases/header_set_synop.pass.cpp",
                "std/utilities/utility/mem.res/mem.res.aliases/header_set_synop2.pass.cpp",
                "std/utilities/utility/mem.res/mem.res.aliases/header_string_synop.pass.cpp",
                "std/utilities/utility/mem.res/mem.res.aliases/header_string_synop2.pass.cpp",
                "std/utilities/utility/mem.res/mem.res.aliases/header_unordered_map_synop.pass.cpp",
                "std/utilities/utility/mem.res/mem.res.aliases/header_unordered_map_synop2.pass.cpp",
                "std/utilities/utility/mem.res/mem.res.aliases/header_unordered_set_synop.pass.cpp",
                "std/utilities/utility/mem.res/mem.res.aliases/header_unordered_set_synop2.pass.cpp",
                "std/utilities/utility/mem.res/mem.res.aliases/header_vector_synop.pass.cpp",
                "std/utilities/utility/mem.res/mem.res.aliases/header_vector_synop2.pass.cpp",
                "std/utilities/utility/mem.res/mem.res.global/new_delete_resource.pass.cpp",
                "std/utilities/utility/mem.res/mem.res.global/null_memory_resource.pass.cpp",
                "std/utilities/utility/mem.res/mem.res.monotonic.buffer/mem.res.monotonic.buffer.ctor/with_default_resource.pass.cpp",
                "std/utilities/utility/mem.res/mem.res.monotonic.buffer/mem.res.monotonic.buffer.ctor/without_buffer.pass.cpp",
                "std/utilities/utility/mem.res/mem.res.monotonic.buffer/mem.res.monotonic.buffer.mem/allocate_deallocate.pass.cpp",
                "std/utilities/utility/mem.res/mem.res.monotonic.buffer/mem.res.monotonic.buffer.mem/allocate_from_initial_buffer.pass.cpp",
                "std/utilities/utility/mem.res/mem.res.monotonic.buffer/mem.res.monotonic.buffer.mem/allocate_from_zero_sized_buffer.pass.cpp",
                "std/utilities/utility/mem.res/mem.res.monotonic.buffer/mem.res.monotonic.buffer.mem/allocate_in_geometric_progression.pass.cpp",
                "std/utilities/utility/mem.res/mem.res.monotonic.buffer/mem.res.monotonic.buffer.mem/allocate_overaligned_request.pass.cpp",
                "std/utilities/utility/mem.res/mem.res.monotonic.buffer/mem.res.monotonic.buffer.mem/allocate_with_initial_size.pass.cpp",
                "std/utilities/utility/mem.res/mem.res.monotonic.buffer/mem.res.monotonic.buffer.mem/equality.pass.cpp",
                "std/utilities/utility/mem.res/mem.res.monotonic.buffer/mem.res.monotonic.buffer.mem/release_reset_initial_status.pass.cpp",
                "std/utilities/utility/mem.res/mem.res.pool/mem.res.pool.ctor/ctor_does_not_allocate.pass.cpp",
                "std/utilities/utility/mem.res/mem.res.pool/mem.res.pool.ctor/sync_with_default_resource.pass.cpp",
                "std/utilities/utility/mem.res/mem.res.pool/mem.res.pool.ctor/unsync_with_default_resource.pass.cpp",
                "std/utilities/utility/mem.res/mem.res.pool/mem.res.pool.mem/equality.pass.cpp",
                "std/utilities/utility/mem.res/mem.res.pool/mem.res.pool.mem/sync_allocate.pass.cpp",
                "std/utilities/utility/mem.res/mem.res.pool/mem.res.pool.mem/sync_allocate_overaligned_request.pass.cpp",
                "std/utilities/utility/mem.res/mem.res.pool/mem.res.pool.mem/sync_allocate_reuse_blocks.pass.cpp",
                "std/utilities/utility/mem.res/mem.res.pool/mem.res.pool.mem/unsync_allocate.pass.cpp",
                "std/utilities/utility/mem.res/mem.res.pool/mem.res.pool.mem/unsync_allocate_overaligned_request.pass.cpp",
                "std/utilities/utility/mem.res/mem.res.pool/mem.res.pool.mem/unsync_allocate_reuse_blocks.pass.cpp",
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
                "armv4t_exn_rtti_size",
                "armv5te_exn_rtti_size",
                "armv6m_soft_nofp_exn_rtti_size",
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
            name="at_exit",
            testnames=[
                "std/language.support/support.start.term/quick_exit.pass.cpp",
            ],
            result=NewResult.XFAILED,
            project="libcxx",
            description="at_quick_exit symbol is not found",
        ),
        XFail(
            name="demangle",
            testnames=[
                "test_demangle.pass.cpp",
            ],
            result=NewResult.PASSED,
            project="libcxx",
            description="Previously xfailed for being too large/slow, but is now passing.",
        ),
        XFail(
            name="uchar types",
            testnames=[
                "std/depr/depr.c.headers/uchar_h.compile.pass.cpp",
                "std/strings/c.strings/cuchar.compile.pass.cpp",
            ],
            result=NewResult.PASSED,
            project="libcxx",
            description="More recent picolibc versions do now support char16_t and char32_t",
        ),
        XFail(
            name="picolibc_sys_seek",
            testnames=[
                "semihost-seek.test",
                "test-fread-fwrite.test",
                "posix-io.test",
            ],
            result=NewResult.XFAILED,
            project="picolibc",
            variants=[
                "aarch64a_be_exn_rtti",
                "aarch64a_be_soft_nofp_exn_rtti",
                "aarch64a_be_soft_nofp",
                "aarch64a_be",
                "aarch64a_exn_rtti",
                "aarch64a",
                "aarch64r_be_exn_rtti",
                "aarch64r_be_soft_nofp_exn_rtti",
                "aarch64r_be_soft_nofp",
                "aarch64r_be",
                "aarch64r_exn_rtti_unaligned",
                "aarch64r_exn_rtti",
                "aarch64r_soft_nofp_exn_rtti_unaligned",
                "aarch64r_soft_nofp_exn_rtti",
                "aarch64r_soft_nofp_unaligned",
                "aarch64r_soft_nofp",
                "aarch64r_unaligned",
                "aarch64r",
                "armebv6m_soft_nofp_exn_rtti_size",
                "armebv6m_soft_nofp_size",
                "armebv7a_hard_vfpv3_d16_exn_rtti",
                "armebv7a_hard_vfpv3_d16",
                "armebv7a_soft_nofp_exn_rtti",
                "armebv7a_soft_nofp",
                "armebv7a_soft_vfpv3_d16_exn_rtti",
                "armebv7a_soft_vfpv3_d16",
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
            description="rateInHz port not connected in Corstone-310 FVP (SDDKW-94045).",
        ),
    ]

    tests_to_xfail = []
    tests_to_upass = []

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
        # TODO: allow tests to be skipped and not run at all.
        # This can be done through the LIT_FILTER environment variable.
        # Unlike the xfail variables, this takes a regex, so an expression
        # will need to be constructed to cover the tests.

    tests_to_xfail.sort()
    tests_to_upass.sort()

    # Save to files for easy consumption by other scripts, or print for users.
    if args.xfails_file:
        os.makedirs(os.path.dirname(args.xfails_file), exist_ok=True)
        with open(args.xfails_file, "w", encoding="utf-8") as f:
            for testname in tests_to_xfail:
                f.write(testname + "\n")
    else:
        print("LIT_XFAIL=" + ";".join(tests_to_xfail))
    if args.xfails_not_file:
        os.makedirs(os.path.dirname(args.xfails_not_file), exist_ok=True)
        with open(args.xfails_not_file, "w", encoding="utf-8") as f:
            for testname in tests_to_upass:
                f.write(testname + "\n")
    else:
        print("LIT_XFAIL_NOT=" + ";".join(tests_to_upass))


if __name__ == "__main__":
    main()
