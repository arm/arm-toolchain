#!/usr/bin/env python3

#
# Copyright (c) 2022-2025, Arm Limited and affiliates.
#
# Part of the Arm Toolchain project, under the Apache License v2.0 with LLVM Exceptions.
# See https://llvm.org/LICENSE.txt for license information.
# SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
#

"""Library code to make ELF files suitable for testing elf2bin.

The main exported function is makeelf(), which constructs an ELF file
based on its parameters and writes it to disk given a filename.

"""

import argparse
import binascii
import collections
import struct


class ElfHdr:
    def __init__(self, bigend):
        self.e_ident = list(b"\x7FELF") + [0] * 12
        self.e_ident[5] = 2 if bigend else 1
        self.e_type = 2  # ET_EXEC
        self.e_machine = 0
        self.e_version = self.e_ident[6] = 1  # EV_CURRENT
        self.e_entry = 0
        self.e_phoff = 0
        self.e_shoff = 0
        self.e_flags = 0
        self.e_ehsize = 0
        self.e_phentsize = 0
        self.e_phnum = 0
        self.e_shentsize = 0
        self.e_shnum = 0
        self.e_shstrndx = 0

    def size(self):
        return struct.calcsize(self.fmt)

    def format(self):
        return struct.pack(
            self.fmt,
            bytes(self.e_ident),
            self.e_type,
            self.e_machine,
            self.e_version,
            self.e_entry,
            self.e_phoff,
            self.e_shoff,
            self.e_flags,
            self.e_ehsize,
            self.e_phentsize,
            self.e_phnum,
            self.e_shentsize,
            self.e_shnum,
            self.e_shstrndx,
        )


class ElfHdr32(ElfHdr):
    def __init__(self, bigend):
        super().__init__(bigend)
        self.e_ident[4] = 1  # 32-bit
        self.e_machine = 40  # EM_ARM
        self.fmt = (">" if bigend else "<") + "16sHHLLLLLHHHHHH"
        self.e_ehsize = self.size()


class ElfHdr64(ElfHdr):
    def __init__(self, bigend):
        super().__init__(bigend)
        self.e_ident[4] = 2  # 64-bit
        self.e_machine = 183  # EM_AARCH64
        self.fmt = (">" if bigend else "<") + "16sHHLQQQLHHHHHH"
        self.e_ehsize = self.size()


class ProgHdr:
    def __init__(self, bigend):
        self.p_type = 0
        self.p_offset = 0
        self.p_vaddr = 0
        self.p_paddr = 0
        self.p_filesz = 0
        self.p_memsz = 0
        self.p_flags = 0
        self.p_align = 0

    def size(self):
        return struct.calcsize(self.fmt)


class ProgHdr32(ProgHdr):
    def __init__(self, bigend):
        super().__init__(bigend)
        self.fmt = (">" if bigend else "<") + "LLLLLLLL"

    def format(self):
        return struct.pack(
            self.fmt,
            self.p_type,
            self.p_offset,
            self.p_vaddr,
            self.p_paddr,
            self.p_filesz,
            self.p_memsz,
            self.p_flags,
            self.p_align,
        )


class ProgHdr64(ProgHdr):
    def __init__(self, bigend):
        super().__init__(bigend)
        self.fmt = (">" if bigend else "<") + "LLQQQQQQ"

    def format(self):
        return struct.pack(
            self.fmt,
            self.p_type,
            self.p_flags,
            self.p_offset,
            self.p_vaddr,
            self.p_paddr,
            self.p_filesz,
            self.p_memsz,
            self.p_align,
        )


class Segment:
    def __init__(self, data, pad):
        self.data = data

    def size(self):
        return len(self.data)

    def format(self):
        return self.data


pt_names = {
    "PT_NULL": 0,
    "PT_LOAD": 1,
    "PT_DYNAMIC": 2,
    "PT_INTERP": 3,
    "PT_NOTE": 4,
    "PT_SHLIB": 5,
    "PT_PHDR": 6,
    "PT_TLS": 7,
}


class SegmentDesc:
    """Description of an ELF segment, used as a parameter to makeelf()."""

    def __init__(self, segtype, paddr, data, pad=0, vaddr=None):
        """Constructor for SegmentDesc.

        `segtype` is the numeric value of the type of the segment,
        e.g. 1 for PT_LOAD. `paddr` is the segment's physical address.
        `vaddr` is its virtual address; you can leave it out (or pass
        it as None) to make that the same as `paddr`. `data` is a
        bytes-like object giving the initialized data of the segment;
        `pad` is the length in bytes of uninitialized padding
        following that data.

        """
        if vaddr is None:
            vaddr = paddr
        self.segtype = segtype
        self.paddr = paddr
        self.data = data
        self.pad = pad
        self.vaddr = vaddr

    def __repr__(self):
        return (
            f"SegmentDesc(segtype={self.segtype!r}, "
            f"paddr={self.paddr:#x}, data={self.data!r}, "
            f"pad={self.pad!r}, vaddr={self.vaddr:#x})"
        )


def read_description_file(fh):
    """Read an image description in the format described above."""

    entry = 0
    segments = []

    for line in fh:
        # Strip trailing newlines (forgiving Windows line endings if
        # found), and discard empty lines and comments.
        line = line.rstrip("\r\n")
        if len(line) == 0 or line.startswith("#"):
            continue

        # Split line into words.
        words = line.split(" ")

        # Handle "entry" line, setting the entry point.
        if words[0] == "entry":
            entry = int(words[1], 0)
            continue

        # Otherwise, we expect that the line has at least three words:
        # segment type, start address and hex-encoded data.
        pt, addr, data = words[:3]
        try:
            pt = pt_names[pt]  # is the segment type a symbolic name?
        except KeyError:
            pt = int(pt)  # otherwise, parse it as an integer
        if "/" in addr:
            paddr, vaddr = (int(part, 0) for part in addr.split("/"))
        else:
            paddr = vaddr = int(addr, 0)
        data = binascii.unhexlify(data)  # and decode the hex data

        # If a fourth word appears on the line, interpret it as the
        # size of zero padding after the segment data. Otherwise that's zero.
        pad = 0 if len(words) == 3 else int(words[3], 0)

        # Done. Append the segment description to the array we'll return.
        segments.append(SegmentDesc(pt, paddr, data, pad, vaddr))

    return segments, entry


def makeelf(outfile, bigend, sixtyfour, segments, entry=None):
    """Make an ELF image with no section header table and write it to `outfile`.

    The ELF file is little-endian if `bigend` is False, or big-endian
    if it is `True`. It is 32-bit if `sixtyfour` is False, or 64-bit
    if it is `True`. `segments` gives the list of desired segments,
    and `entry` gives the image's entry point address, if it is not None.

    """
    ehclass = ElfHdr64 if sixtyfour else ElfHdr32
    phclass = ProgHdr64 if sixtyfour else ProgHdr32

    fileitems = []

    offset = 0

    header = ehclass(bigend)
    fileitems.append(header)
    offset += header.size()

    header.e_phoff = offset
    header.e_phnum = len(segments)
    header.e_phentsize = phclass(bigend).size()
    header.e_entry = entry if entry else 0
    offset += header.e_phnum * header.e_phentsize
    ph = []
    segs = []
    for desc in segments:
        seg = Segment(desc.data, desc.pad)
        phent = phclass(bigend)
        phent.p_type = desc.segtype
        phent.p_offset = offset
        phent.p_vaddr = desc.vaddr
        phent.p_paddr = desc.paddr
        phent.p_filesz = len(desc.data)
        phent.p_memsz = len(desc.data) + desc.pad
        ph.append(phent)
        segs.append(seg)
        offset += seg.size()

    fileitems.extend(ph)
    fileitems.extend(segs)

    def write_to(fh):
        for item in fileitems:
            fh.write(item.format())

    # Allow outfile to be a file name or a file handle
    if isinstance(outfile, str):
        with open(outfile, "wb") as fh:
            write_to(fh)
    else:
        write_to(outfile)


def main():
    explanation = """\
Make a minimal segment-only ELF file from a text description.

This program expects an input text file describing one segment per
line, such as this example:

entry 0x1238
PT_LOAD 0x1234 000102030405060708090A0B0C0D0E0F
PT_LOAD 0x12345678 101112131415161718191A1B1C1D1E1F 32
PT_LOAD 0x12345678/0xABCDEF00 101112131415161718191A1B1C1D1E1F 32

Each segment line contains the segment type (symbolic or numeric), the
base address, and the hex data to be stored in the file.

If the base address field contains '/' then it will be treated as two
different addresses, to be put in the p_paddr and p_vaddr fields
respectively.

An optional fourth field indicates the number of zero bytes of padding
to specify after the segment data, by making its p_memsz bigger than
p_filesz.

A line beginning 'entry' specifies the entry point of the file.

"""

    opener = lambda mode: lambda fname: lambda: argparse.FileType(mode)(fname)
    parser = argparse.ArgumentParser(
        formatter_class=argparse.RawDescriptionHelpFormatter,
        description=explanation,
    )
    parser.add_argument(
        "infile",
        type=opener("r"),
        nargs="?",
        default=opener("r")("-"),
        help="Input file.",
    )
    parser.add_argument(
        "-o", "--output", type=opener("wb"), required=True, help="Output file."
    )
    parser.add_argument("-b", "--bi", action="store_true", help="Big-endian")
    parser.add_argument(
        "-s", "--sixtyfour", action="store_true", help="64-bit"
    )
    args = parser.parse_args()

    with args.infile() as fh:
        segments, entry = read_description_file(fh)

    with args.output() as fh:
        makeelf(fh, args.bi, args.sixtyfour, segments, entry)


if __name__ == "__main__":
    main()
