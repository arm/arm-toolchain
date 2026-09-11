#!/usr/bin/env python3

"""
Identifies and extracts header files that are common across multiple multilib variant directories.

This script scans all variant folders within a multilib target directory
(e.g., `arm-none-eabi/variant1/include`, `arm-none-eabi/variant2/include`, etc.) and compares all
headers by relative path and content. Minor variants with reduced header sets are skipped during
common header discovery, so they do not reduce the common header set. If a header path has matching
content in a majority of non-minor variants, that content group is moved to a shared include directory at:

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
that are shared across architecture variants. Variants with different content retain a local copy,
which is found first because the variant include directory is searched before the target include
directory. Minor variants are checked against the selected common headers in a second pass and only
keep local copies for headers that are missing from, or differ from, the common include directory.
"""

import argparse
import hashlib
import os
import shutil

# Define the multilib target dirs which want to process
MULTILIB_TARGET_DIRS = ["arm-none-eabi", "aarch64-none-elf"]
PRIMARY_VARIANT_GROUP = "stdlibs"


def file_content_hash(path):
    content_hash = hashlib.sha256()
    with open(path, "rb") as file:
        for chunk in iter(lambda: file.read(1024 * 1024), b""):
            content_hash.update(chunk)
    return content_hash.hexdigest()


def collect_variant_include_paths(input_target_dir):
    """
    Extracts each multilib variant and its corresponding include path from the non-optimised multilib directory.
    Stores the results to enable later comparison of header contents across different non-optimised multilib variant
    include paths.

    """
    variant_include_paths = {}
    for variant in sorted(os.listdir(input_target_dir)):
        variant_include_path = os.path.join(input_target_dir, variant, "include")
        if os.path.isdir(variant_include_path):
            variant_include_paths[variant] = variant_include_path
    return variant_include_paths


def copy_header(src, output_include_dir, header_name):
    dst = os.path.join(output_include_dir, header_name)
    os.makedirs(os.path.dirname(dst), exist_ok=True)
    shutil.copy2(src, dst)


def group_headers_by_name_and_content_hash(variant_includes):
    # {
    #     header_name: {
    #         content_hash: [
    #             (variant_name, variant_header_path),
    #             ...
    #         ],
    #         other_content_hash: [
    #             (variant_name, variant_header_path),
    #             ...
    #         ]
    #     }
    # }
    # Variants with the same header name and identical file contents end up in
    # the same content_hash list.
    headers = {}
    for variant, variant_include_path in variant_includes.items():
        for root, sub_dirs, header_files in os.walk(variant_include_path):
            sub_dirs.sort()
            for header in sorted(header_files):
                variant_header_path = os.path.join(root, header)
                header_name = os.path.relpath(variant_header_path, variant_include_path)
                content_hash = file_content_hash(variant_header_path)
                # Create or update the nested entry for each header, then record
                # which variant provides this exact header content.
                headers.setdefault(header_name, {}).setdefault(content_hash, []).append(
                    (variant, variant_header_path)
                )
    return headers


def parse_yaml_scalar(value):
    return value.strip().strip("\"'")


def collect_variant_groups(multilib_yaml):
    # Navigate multilib.yaml, only paying attention to the Variants: section
    # and skipping other sections. Inside Variants:, remember each Dir until
    # its matching Group is found, then record "target/variant" -> "group_name".
    variant_groups = {}
    current_dir = None
    in_variants = False

    with open(multilib_yaml, encoding="utf-8") as file:
        for line in file:
            stripped = line.strip()
            if not stripped or stripped.startswith("#"):
                continue

            if line[0] not in (" ", "-") and stripped.endswith(":"):
                in_variants = stripped == "Variants:"
                current_dir = None
                continue

            if not in_variants:
                continue

            if stripped.startswith("- Dir:"):
                current_dir = parse_yaml_scalar(stripped.split(":", 1)[1])
                continue

            if stripped.startswith("- Error:"):
                current_dir = None
                continue

            if current_dir and stripped.startswith("Group:"):
                variant_groups[current_dir] = parse_yaml_scalar(
                    stripped.split(":", 1)[1]
                )
                current_dir = None

    return variant_groups


def split_variant_includes_by_group(variant_includes, variant_groups, target):
    """
    Split variant include paths by their multilib.yaml Group.

    Variants in PRIMARY_VARIANT_GROUP are treated as primary/base variants and
    drive common header selection. All other grouped variants are treated as
    minor variants and are checked against the selected common headers later.
    """
    primary_includes = {
        variant: variant_include_path
        for variant, variant_include_path in variant_includes.items()
        if variant_groups.get(f"{target}/{variant}") == PRIMARY_VARIANT_GROUP
    }
    minor_includes = {
        variant: variant_include_path
        for variant, variant_include_path in variant_includes.items()
        if variant_groups.get(f"{target}/{variant}") != PRIMARY_VARIANT_GROUP
    }

    missing_groups = [
        f"{target}/{variant}"
        for variant in variant_includes
        if f"{target}/{variant}" not in variant_groups
    ]
    if missing_groups:
        raise KeyError(
            "Missing Group entries in multilib.yaml for variants: "
            + ", ".join(missing_groups)
        )

    if not primary_includes:
        raise ValueError(
            f"No variants in target '{target}' belong to '{PRIMARY_VARIANT_GROUP}'."
        )

    return primary_includes, minor_includes


def select_common_header_group(content_hash_groups, primary_variant_count):
    # Select a header content group only when it appears in a strict majority
    # of primary variants. For example:
    # - 4 of 5 variants have the same header: common
    # - 3 of 5 variants have the same header: common
    # - 2 of 4 variants have the same header: not common
    # - 1 of 3 variants has the header: not common
    # Start with the content hash that appears in the largest number of variants.
    common_group = max(
        sorted(content_hash_groups.items()),
        key=lambda item: len(item[1]),
    )
    common_group_size = len(common_group[1])
    # Require the most common content hash to appear in a strict majority of primary variants.
    if primary_variant_count > 1 and common_group_size * 2 <= primary_variant_count:
        return None
    return common_group


def generate_common_headers(
    variant_includes, variant_groups, target, output_target_dir
):
    output_include_dir = os.path.join(output_target_dir, "include")
    os.makedirs(output_include_dir, exist_ok=False)

    primary_includes, minor_includes = split_variant_includes_by_group(
        variant_includes, variant_groups, target
    )
    primary_variant_count = len(primary_includes)
    headers = group_headers_by_name_and_content_hash(primary_includes)

    for header_name in sorted(headers):
        content_hash_groups = headers[header_name]
        common_group = select_common_header_group(
            content_hash_groups, primary_variant_count
        )
        common_content_hash = None

        if common_group:
            common_content_hash, common_entries = common_group
            copy_header(common_entries[0][1], output_include_dir, header_name)

        for content_hash, entries in content_hash_groups.items():
            if content_hash == common_content_hash:
                continue

            # Any primary header content hash that was not selected as common
            # must remain local to the variants that use that header content.
            for variant, variant_header_path in entries:
                variant_include_dir = os.path.join(
                    output_target_dir, variant, "include"
                )
                copy_header(variant_header_path, variant_include_dir, header_name)

    minor_headers = group_headers_by_name_and_content_hash(minor_includes)
    for header_name in sorted(minor_headers):
        common_header = os.path.join(output_include_dir, header_name)
        common_content_hash = (
            file_content_hash(common_header) if os.path.exists(common_header) else None
        )

        for content_hash, entries in minor_headers[header_name].items():
            if content_hash == common_content_hash:
                continue

            for variant, variant_header_path in entries:
                variant_include_dir = os.path.join(
                    output_target_dir, variant, "include"
                )
                copy_header(variant_header_path, variant_include_dir, header_name)


def extract_common_headers_for_targets(args):
    if os.path.exists(args.multilib_optimised_dir):
        shutil.rmtree(args.multilib_optimised_dir)

    if os.path.isdir(args.multilib_non_optimised_dir):
        existing_target_dirs = [
            dir_name
            for dir_name in MULTILIB_TARGET_DIRS
            if os.path.isdir(os.path.join(args.multilib_non_optimised_dir, dir_name))
        ]
        if not existing_target_dirs:
            raise Exception(
                f"Error: Expected to find either arm-none-eabi or aarch64-none-elf in '{args.multilib_non_optimised_dir}', but folder is empty."
            )
        src_yaml = os.path.join(args.multilib_non_optimised_dir, "multilib.yaml")
        if not os.path.exists(src_yaml):
            raise FileNotFoundError(f"Source yaml '{src_yaml}' does not exist.")
        variant_groups = collect_variant_groups(src_yaml)
    else:
        raise FileNotFoundError(
            f"Error: Expected folder '{args.multilib_non_optimised_dir}' does not exist"
        )

    for target in MULTILIB_TARGET_DIRS:
        input_target_dir = os.path.join(
            os.path.abspath(args.multilib_non_optimised_dir), target
        )
        output_target_dir = os.path.join(
            os.path.abspath(args.multilib_optimised_dir), target
        )

        if not os.path.isdir(input_target_dir):
            print(
                f"Skipping extracting the common headers for {target}: input path {input_target_dir} not found"
            )
            continue

        variant_includes = collect_variant_include_paths(input_target_dir)
        if len(variant_includes) < 2:
            print(
                f"Skipping extracting the common headers for {target}: not enough variants to compare. "
                "At least two variants must be enabled for the multilib header optimisation phase to proceed."
            )
            # The script always creates the multilib-optimised folder, even when there's only one variant and no
            # optimization is applied. In that case, multilib-optimised will just contain a copy of the
            # single variant from the non-optimised multilib directory.
            if os.path.exists(input_target_dir):
                shutil.copytree(
                    input_target_dir, output_target_dir, dirs_exist_ok=False
                )
            continue

        # Select common headers from the base variants first. Reduced minor
        # variants are checked against the common include directory afterwards.
        generate_common_headers(
            variant_includes, variant_groups, target, output_target_dir
        )

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
    shutil.copy2(src_yaml, dst_yaml)


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
