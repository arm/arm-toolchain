#!/usr/bin/env python3

"""
Identifies and extracts header files that are common across multiple multilib variant directories.

This script scans all variant folders within a multilib target directory
(e.g., `arm-none-eabi/variant1/include`, `arm-none-eabi/variant2/include`, etc.) and compares all
`.h` files. If the same file (by name and content) appears in multiple variants, it will be moved
to a shared include directory at:

    <CMAKE_BINARY_DIR>/multilib-optimised/<target>/include/

for the following multilib targets:
- arm-none-eabi
- aarch64-none-elf

Arguments:
    <CMAKE_BINARY_DIR>/multilib Path to the CMake build directory containing non optmised multilib.
    eg: build/multilib-builds/multilib/picolibc-build/multilib
    <CMAKE_BINARY_DIR>/multilib-optimised  Path to the CMake build directory where optimised multilib should be generated..
    eg: build/multilib-builds/multilib/picolibc-build/multilib-optimised

This is useful to reduce duplication in the toolchain by centralising common headers
that are shared across architecture variants.
"""
import argparse
import os
import filecmp
import shutil

# Define the multilib target dirs which want to process
MULTILIB_TARGET_DIRS = ["arm-none-eabi", "aarch64-none-elf"]


def files_are_identical(f1, f2):
    return filecmp.cmp(f1, f2, shallow=False)


def collect_variant_include_paths(input_target_dir):
    """
    Extracts each multilib variant and its corresponding include path from the non-optimised multilib directory.
    Stores the results to enable later comparison of header contents across different non-optimised multilib variant
    include paths.

    """
    variant_include_path = {}
    for variant in os.listdir(input_target_dir):
        include_path = os.path.join(input_target_dir, variant, "include")
        if os.path.isdir(include_path):
            variant_include_path[variant] = include_path
    return variant_include_path


def extract_common_headers_for_targets(args):
    if os.path.exists(args.multilib_optimised_dir):
        shutil.rmtree(args.multilib_optimised_dir)

    for target in MULTILIB_TARGET_DIRS:
        input_target_dir = os.path.join(
            os.path.abspath(args.multilib_non_optimised_dir), target
        )
        output_target_dir = os.path.join(
            os.path.abspath(args.multilib_optimised_dir), target
        )
        output_include_dir = os.path.join(output_target_dir, "include")

        if not os.path.isdir(input_target_dir):
            print(
                f"Skipping extracting the common headers for {target}: input path {input_target_dir} not found"
            )
            continue

        variant_includes = collect_variant_include_paths(input_target_dir)
        if len(variant_includes) < 2:
            print(
                f"Skipping extracting the common headers for {target}: not enough variants to compare.At least two variants must be enabled for the multilib header optimisation phase to proceed."
            )
            # The script always creates the multilib-optimised folder, even when there's only one variant and no
            # optimization is applied. In that case, multilib-optimised will just contain a copy of the
            # single variant from the non-optimised multilib directory.
            if os.path.exists(args.multilib_non_optimised_dir):
                shutil.copytree(args.multilib_non_optimised_dir, args.multilib_optimised_dir)
            return

        # Creating the common include headers for each target
        os.makedirs(output_include_dir, exist_ok=True)

        # Step 1: compare first two variants and extract the common headers into the targets common include directory
        base_dir = list(variant_includes.values())[0]
        compare_dir = list(variant_includes.values())[1]
        for root, sub_dirs, header_files in os.walk(base_dir):
            sub_dir_root = os.path.relpath(root, base_dir)
            for header in header_files:
                h1 = os.path.join(base_dir, sub_dir_root, header)
                h2 = os.path.join(compare_dir, sub_dir_root, header)
                if os.path.exists(h2) and files_are_identical(h1, h2):
                    out_dir = os.path.join(output_include_dir, sub_dir_root)
                    os.makedirs(out_dir, exist_ok=True)
                    shutil.copy2(h1, os.path.join(out_dir, header))

        # Step 2: Compare all the variants with the new common include. Any headers that do not match
        # and do not exit in common include should retain in their respective variant specific directories.
        for variant, include_path in variant_includes.items():
            for root, sub_dirs, header_files in os.walk(include_path):
                sub_dir_root = os.path.relpath(root, include_path)
                for header in header_files:
                    variant_header = os.path.join(include_path, sub_dir_root, header)
                    common_header = os.path.join(
                        output_include_dir, sub_dir_root, header
                    )
                    if not os.path.exists(common_header) or not files_are_identical(
                        variant_header, common_header
                    ):
                        out_dir = os.path.join(
                            os.path.abspath(args.multilib_optimised_dir),
                            target,
                            variant,
                            sub_dir_root,
                            "include",
                        )
                        os.makedirs(out_dir, exist_ok=True)
                        shutil.copy2(variant_header, os.path.join(out_dir, header))

        # Step3: For each variant, the lib and share directories should be copied from the non-optimised multilib
        # directory as it is.
        for variant in variant_includes:
            remaining_dirs = ["lib", "share"]
            for folder in remaining_dirs:
                src_dir = os.path.join(input_target_dir, variant, folder)
                dst_dir = os.path.join(output_target_dir, variant, folder)
                if os.path.exists(src_dir):
                    # If destination exists, remove it first
                    if os.path.exists(dst_dir):
                        shutil.rmtree(dst_dir)
                    os.makedirs(os.path.dirname(dst_dir), exist_ok=True)
                    shutil.copytree(src_dir, dst_dir)
                else:
                    print(f"Warning: {src_dir} does not exist and will be skipped.")

        # Step4: Copy multilib.yaml file as it is from the non-optimised multilib directoy.
        src_yaml = os.path.join(args.multilib_non_optimised_dir, "multilib.yaml")
        dst_yaml = os.path.join(args.multilib_optimised_dir, "multilib.yaml")
        if os.path.exists(src_yaml):
            shutil.copy2(src_yaml, dst_yaml)
        else:
            raise FileNotFoundError(f"Source yaml '{src_yaml}' does not exist.")


def main():
    parser = argparse.ArgumentParser(
        description=__doc__,
        formatter_class=argparse.RawDescriptionHelpFormatter,
    )
    parser.add_argument(
        "multilib_non_optimised_dir",
        help="CMake binary directory containing the non-optimised multilib headers",
    )
    parser.add_argument(
        "multilib_optimised_dir",
        help="CMake binary directory where the optimised multilib headers should be generated",
    )
    args = parser.parse_args()

    extract_common_headers_for_targets(args)


if __name__ == "__main__":
    main()
