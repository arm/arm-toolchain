#!/usr/bin/env python3

#
# Copyright (c) 2023-2026, Arm Limited and affiliates.
# Part of the Arm Toolchain project, under the Apache License v2.0 with LLVM Exceptions.
# See https://llvm.org/LICENSE.txt for license information.
# SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
#

"""Compress a file to Deflate format (RFC 1951), ensuring that the
output data does not contain any block using dynamic Huffman trees.

This restricted version of the format can be selected by an option in
zlib, and allows a much smaller runtime decoder than full Deflate.

"""

import argparse
import collections
import struct
import sys
import zlib


def main():
    parser = argparse.ArgumentParser(
        description=__doc__,
        formatter_class=argparse.RawDescriptionHelpFormatter,
    )
    parser.add_argument("infile", help="File to compress.")
    parser.add_argument("-o", "--output", required=True, help="Output file name.")
    args = parser.parse_args()

    # Read the input file.
    with open(args.infile, "rb") as fh:
        data = fh.read()

    # Compress the file using Zlib, which permits us to pass the
    # Z_FIXED flag forbidding dynamic Huffman trees. However, turn the
    # compression level up to 9, which makes the search for LZ77
    # matches more thorough.
    zo = zlib.compressobj(level=9, strategy=zlib.Z_FIXED)
    compressed = zo.compress(data)
    compressed += zo.flush()

    # Zlib outputs in the ZLIB format (RFC 1950), which is Deflate
    # wrapped in a small header and trailer. Strip those off to
    # recover the bare Deflate data, so that the decoder has less to
    # do at run time.
    compressed = zlib_to_deflate(compressed)

    # Write to the output file.
    with open(args.output, "wb") as fh:
        fh.write(compressed)


def zlib_to_deflate(data):
    "Remove the header from ZLIB (RFC 1950) data, leaving bare Deflate."

    # The header is always two bytes long. Unpack it as a 16-bit
    # integer and check that no fields in it have unexpected values.
    (header,) = struct.unpack_from(">H", data, 0)
    assert header % 31 == 0, "zlib header checksum fail"
    assert (header & 0xF000) <= 0x7000, "unexpectedly high window size"
    assert (header & 0x0F00) == 0x0800, "non-Deflate algorithm"
    assert (header & 0x0020) == 0, "unsupported dictionary present"

    # We're satisfied. Strip off the header, and also the 4-byte
    # checksum at the end.
    #
    # (For general ZLIB, the header might not always be 2 bytes,
    # because a dictionary id field might be present. But we've
    # eliminated that possibility here, by checking that the
    # dictionary flag is not set.)
    return data[2:-4]


if __name__ == "__main__":
    main()
