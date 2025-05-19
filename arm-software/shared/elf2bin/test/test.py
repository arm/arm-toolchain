#!/usr/bin/env python3

# Test suite for elf2bin
#
# Copyright (c) 2022-2025, Arm Limited and affiliates.
#
# Part of the Arm Toolchain project, under the Apache License v2.0 with LLVM Exceptions.
# See https://llvm.org/LICENSE.txt for license information.
# SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
#

import argparse
import contextlib
import inspect
import os
import shlex
import subprocess
import sys
import tempfile
import unittest

from binascii import unhexlify

from makeelf import makeelf, SegmentDesc

elf2bin_path = None


def to_vhx(data):
    return "".join("{:02X}\n".format(byte) for byte in data)


class Base(unittest.TestCase):
    def __init__(self, *args, **kws):
        super().__init__(*args, **kws)
        self.next_index = {}

    @contextlib.contextmanager
    def tempsubdir(self):
        """Return a subdirectory of the main tempdir, named after the calling
        method so that it's easy to inspect the temp files."""
        st = inspect.stack()
        methodname = st[2].function
        classname = type(self).__name__
        dirbase = f"{classname}.{methodname}"
        index = self.next_index.get(dirbase, 0)
        self.next_index[dirbase] = index + 1
        dirname = f"{dirbase}.{index:d}"
        path = os.path.join(tempdir, dirname)
        os.mkdir(path)

        # Temporarily change directory to the subdir, yield to run the
        # code wrapped in this context, and change back to the
        # original directory after the code finishes.
        if hasattr(os, "fchdir") and hasattr(os, "O_PATH"):
            # On Unix, we can do this in a way that doesn't depend on
            # string-based pathnames, and therefore is robust against
            # the path structure of the filesystem changing while
            # we're in the middle of working.
            #
            # For example, if an overzealous cleanup of /tmp (either
            # manual or automatic) were to delete our whole working
            # directory and someone else were to create a new one
            # under the same name, then storing os.getcwd() as a
            # string and os.chdir()ing to it afterwards would land us
            # in the _new_ directory, and we'd start overwriting files
            # belonging to some other process.
            #
            # A race-safe approach is to make a file descriptor
            # pointing at the inode of our cwd, and use fchdir() to
            # change back to it. Then we get back to the directory we
            # were really in, even if it's not linked from the same
            # place any more (or even at all).
            here = os.open(".", os.O_PATH)
            try:
                os.chdir(path)
                yield
            finally:
                os.fchdir(here)
                os.close(here)
        else:
            # Fallback for Windows: if O_PATH and fchdir aren't
            # available, do this the pedestrian way. On Windows that's
            # probably safe anyway, because the more aggressive file
            # locking policy means that as long as this process is in
            # a subdir of our original tempdir, other processes
            # _can't_ mess about with the pathname leading to it.
            here = os.getcwd()
            try:
                os.chdir(path)
                yield
            finally:
                os.chdir(here)

    def elf2bin(self, *args, expect_error=False):
        cmd = [elf2bin_path] + list(args)
        with open("commands.txt", "a") as fh:
            print(" ".join(map(shlex.quote, cmd)), file=fh)
        p = subprocess.Popen(
            cmd, stdout=subprocess.PIPE, stderr=subprocess.PIPE
        )
        out, err = p.communicate(b"")
        p.wait()
        if not expect_error:
            self.assertEqual(err.decode("UTF-8"), "")
            self.assertEqual(p.returncode, 0)
        else:
            self.assertNotEqual(p.returncode, 0)
            return err.decode("UTF-8").rstrip("\r\n")


class ihex(Base):
    def testBasic(self):
        """Test basic ihex output, with two widely separated segments, and
        also demonstrate the 16-byte limit in each hex line.
        """
        for bigend in False, True:
            for sixtyfour in False, True:
                with self.tempsubdir():
                    makeelf(
                        "input.elf",
                        bigend,
                        sixtyfour,
                        [
                            SegmentDesc(1, 0x1234, bytes(range(20))),
                            SegmentDesc(1, 0x123456, bytes(range(20, 24))),
                        ],
                        0x1238,
                    )
                    self.elf2bin("--ihex", "-o", "output.hex", "input.elf")
                    with open("output.hex") as fh:
                        hexdata = fh.read()
                    self.assertEqual(
                        hexdata,
                        """\
:10123400000102030405060708090A0B0C0D0E0F32
:041244001011121360
:020000040012E8
:04345600141516171C
:0400000500001238AD
:00000001FF
""",
                    )

    def testOffsetWrap(self):
        "Check what happens when we wrap from one 16-bit top half to the next."
        with self.tempsubdir():
            makeelf(
                "input.elf",
                False,
                False,
                [SegmentDesc(1, 0xFFF0, bytes(range(32)))],
            )
            self.elf2bin("--ihex", "-o", "output.hex", "input.elf")
            with open("output.hex") as fh:
                hexdata = fh.read()
            self.assertEqual(
                hexdata,
                """\
:10FFF000000102030405060708090A0B0C0D0E0F89
:020000040001F9
:10000000101112131415161718191A1B1C1D1E1F78
:0400000500000000F7
:00000001FF
""",
            )

        with self.tempsubdir():
            makeelf(
                "input.elf",
                True,
                True,
                [SegmentDesc(1, 0xFFF5, bytes(range(32)))],
            )
            self.elf2bin("--ihex", "-o", "output.hex", "input.elf")
            with open("output.hex") as fh:
                hexdata = fh.read()
            self.assertEqual(
                hexdata,
                """\
:10FFF500000102030405060708090A0B0C0D0E0F84
:020000040001F9
:10000500101112131415161718191A1B1C1D1E1F73
:0400000500000000F7
:00000001FF
""",
            )

    def testDataRange(self):
        "Test range checking of data addresses."
        with self.tempsubdir():
            makeelf(
                "input.elf",
                False,
                True,
                [SegmentDesc(1, 0xFFFFFFFF, bytes(range(16)))],
            )
            self.elf2bin("--ihex", "-o", "output.hex", "input.elf")

        with self.tempsubdir():
            makeelf(
                "input.elf",
                False,
                True,
                [SegmentDesc(1, 0x100000000, bytes(range(16)))],
            )
            err = self.elf2bin(
                "--ihex", "-o", "output.hex", "input.elf", expect_error=True
            )
            self.assertEqual(
                err,
                "elf2bin: input.elf: data address does not " "fit in 32 bits",
            )

    def testEntryPointRange(self):
        "Test range checking of data addresses."
        with self.tempsubdir():
            makeelf(
                "input.elf",
                False,
                True,
                [SegmentDesc(1, 0, bytes(range(16)))],
                0xFFFFFFFF,
            )
            self.elf2bin("--ihex", "-o", "output.hex", "input.elf")

        with self.tempsubdir():
            makeelf(
                "input.elf",
                False,
                True,
                [SegmentDesc(1, 0, bytes(range(16)))],
                0x100000000,
            )
            err = self.elf2bin(
                "--ihex", "-o", "output.hex", "input.elf", expect_error=True
            )
            self.assertEqual(
                err,
                "elf2bin: input.elf: entry point does not " "fit in 32 bits",
            )


class srec(Base):
    def testBasic(self):
        "Test basic srec output, using the same test case as ihex."
        for bigend in False, True:
            for sixtyfour in False, True:
                with self.tempsubdir():
                    makeelf(
                        "input.elf",
                        bigend,
                        sixtyfour,
                        [
                            SegmentDesc(1, 0x1234, bytes(range(20))),
                            SegmentDesc(1, 0x123456, bytes(range(20, 24))),
                        ],
                        0x1238,
                    )
                    self.elf2bin("--srec", "-o", "output.hex", "input.elf")
                    with open("output.hex") as fh:
                        hexdata = fh.read()
                    self.assertEqual(
                        hexdata,
                        """\
S31500001234000102030405060708090A0B0C0D0E0F2C
S30900001244101112135A
S309001234561415161704
S70500001238B0
""",
                    )

    def testDataRange(self):
        "Test range checking of data addresses."
        with self.tempsubdir():
            makeelf(
                "input.elf",
                False,
                True,
                [SegmentDesc(1, 0xFFFFFFFF, bytes(range(16)))],
            )
            self.elf2bin("--srec", "-o", "output.hex", "input.elf")

        with self.tempsubdir():
            makeelf(
                "input.elf",
                False,
                True,
                [SegmentDesc(1, 0x100000000, bytes(range(16)))],
            )
            err = self.elf2bin(
                "--srec", "-o", "output.hex", "input.elf", expect_error=True
            )
            self.assertEqual(
                err,
                "elf2bin: input.elf: data address does not " "fit in 32 bits",
            )

    def testEntryPointRange(self):
        "Test range checking of data addresses."
        with self.tempsubdir():
            makeelf(
                "input.elf",
                False,
                True,
                [SegmentDesc(1, 0, bytes(range(16)))],
                0xFFFFFFFF,
            )
            self.elf2bin("--srec", "-o", "output.hex", "input.elf")

        with self.tempsubdir():
            makeelf(
                "input.elf",
                False,
                True,
                [SegmentDesc(1, 0, bytes(range(16)))],
                0x100000000,
            )
            err = self.elf2bin(
                "--srec", "-o", "output.hex", "input.elf", expect_error=True
            )
            self.assertEqual(
                err,
                "elf2bin: input.elf: entry point does not " "fit in 32 bits",
            )


class bin(Base):
    def testOneSegment(self):
        "Test basic binary output with a single segment."
        segment_contents = bytes(range(20))

        for bigend in False, True:
            for sixtyfour in False, True:
                with self.tempsubdir():
                    makeelf(
                        "input.elf",
                        bigend,
                        sixtyfour,
                        [SegmentDesc(1, 0x1234, segment_contents)],
                    )
                    self.elf2bin("--bin", "-o", "output.bin", "input.elf")
                    with open("output.bin", "rb") as fh:
                        bindata = fh.read()
                    self.assertEqual(bindata, segment_contents)

    def testMultiSegments(self):
        "Test multiple segments via -O."
        segment1_contents = bytes(range(20))
        segment2_contents = bytes(range(20, 32))

        for bigend in False, True:
            for sixtyfour in False, True:
                with self.tempsubdir():
                    makeelf(
                        "input.elf",
                        bigend,
                        sixtyfour,
                        [
                            SegmentDesc(1, 0x1234, segment1_contents),
                            SegmentDesc(1, 0x123456, segment2_contents),
                        ],
                    )

                    self.elf2bin("--bin", "-O", "output-%a.bin", "input.elf")
                    with open("output-1234.bin", "rb") as fh:
                        bindata = fh.read()
                    self.assertEqual(bindata, segment1_contents)
                    with open("output-123456.bin", "rb") as fh:
                        bindata = fh.read()
                    self.assertEqual(bindata, segment2_contents)


class bincombined(Base):
    def testBasic(self):
        "Test basic bincombined output."
        segment1_contents = bytes(range(0x10))
        segment2_contents = bytes(range(0x10, 0x20))

        for bigend in False, True:
            for sixtyfour in False, True:
                with self.tempsubdir():
                    makeelf(
                        "input.elf",
                        bigend,
                        sixtyfour,
                        [
                            SegmentDesc(1, 0x1234, segment1_contents),
                            SegmentDesc(1, 0x1264, segment2_contents),
                        ],
                    )

                    self.elf2bin(
                        "--bincombined", "-o", "output.bin", "input.elf"
                    )
                    with open("output.bin", "rb") as fh:
                        bindata = fh.read()
                    self.assertEqual(
                        bindata,
                        segment1_contents + b"\0" * 0x20 + segment2_contents,
                    )

    def testReversed(self):
        "Test having the segments in the wrong order."
        segment1_contents = bytes(range(0x10))
        segment2_contents = bytes(range(0x10, 0x20))

        with self.tempsubdir():
            makeelf(
                "input.elf",
                False,
                True,
                [
                    SegmentDesc(1, 0x1264, segment2_contents),
                    SegmentDesc(1, 0x1234, segment1_contents),
                ],
            )

            self.elf2bin("--bincombined", "-o", "output.bin", "input.elf")
            with open("output.bin", "rb") as fh:
                bindata = fh.read()
            self.assertEqual(
                bindata, segment1_contents + b"\0" * 0x20 + segment2_contents
            )

    def testOverlapOK(self):
        "Test overlap detection doesn't trigger on exactly-abutting segments."

        with self.tempsubdir():
            makeelf(
                "input.elf",
                False,
                True,
                [
                    SegmentDesc(1, 0x1000, b"\x00" * 0x100),
                    SegmentDesc(1, 0x1100, b"\x01" * 0x100),
                ],
            )
            self.elf2bin("--bincombined", "-o", "output.bin", "input.elf")

    def testOverlapFail(self):
        "Test overlap detection does trigger when segments overlap."

        with self.tempsubdir():
            makeelf(
                "input.elf",
                False,
                True,
                [
                    SegmentDesc(1, 0x1000, b"\x00" * 0x100),
                    SegmentDesc(1, 0x10FF, b"\x01" * 0x100),
                ],
            )
            err = self.elf2bin(
                "--bincombined",
                "-o",
                "output.bin",
                "input.elf",
                expect_error=True,
            )
            self.assertEqual(
                err,
                "elf2bin: input.elf: segments at address ranges "
                "[0x1000,0x1100) and [0x10ff,0x11ff) overlap",
            )

    def testOverlapMemsz(self):
        "Test overlap detection also spots overlap of the p_memsz padding."

        with self.tempsubdir():
            makeelf(
                "input.elf",
                False,
                True,
                [
                    SegmentDesc(1, 0x1000, b"\x00" * 0x80, 0x80),
                    SegmentDesc(1, 0x10FF, b"\x01" * 0x100),
                ],
            )
            err = self.elf2bin(
                "--bincombined",
                "-o",
                "output.bin",
                "input.elf",
                expect_error=True,
            )
            self.assertEqual(
                err,
                "elf2bin: input.elf: segments at address ranges "
                "[0x1000,0x1100) and [0x10ff,0x11ff) overlap",
            )

    def testBaseAddr(self):
        "Test the ability to specify a lower base address."
        segment1_contents = bytes(range(0x10))
        segment2_contents = bytes(range(0x10, 0x20))

        with self.tempsubdir():
            makeelf(
                "input.elf",
                False,
                False,
                [
                    SegmentDesc(1, 0x1234, segment1_contents),
                    SegmentDesc(1, 0x1264, segment2_contents),
                ],
            )
            self.elf2bin(
                "--bincombined",
                "--base",
                "0x1200",
                "-o",
                "output.bin",
                "input.elf",
            )
            with open("output.bin", "rb") as fh:
                bindata = fh.read()
            self.assertEqual(
                bindata,
                b"\0" * 0x34
                + segment1_contents
                + b"\0" * 0x20
                + segment2_contents,
            )

            # Should be OK to restate the base address we were going
            # to use anyway
            self.elf2bin(
                "--bincombined",
                "--base",
                "0x1234",
                "-o",
                "output.bin",
                "input.elf",
            )
            with open("output.bin", "rb") as fh:
                bindata = fh.read()
            self.assertEqual(
                bindata, segment1_contents + b"\0" * 0x20 + segment2_contents
            )

            # But even one byte higher is an error
            err = self.elf2bin(
                "--bincombined",
                "--base",
                "0x1235",
                "-o",
                "output.bin",
                "input.elf",
                expect_error=True,
            )
            self.assertEqual(
                err,
                "elf2bin: input.elf: first segment is at "
                "address 0x1234, below the specified base address "
                "0x1235",
            )


class vhx(Base):
    def testOneSegment(self):
        "Test basic vhx output with a single segment."
        segment_contents = bytes(range(20))

        for bigend in False, True:
            for sixtyfour in False, True:
                with self.tempsubdir():
                    makeelf(
                        "input.elf",
                        bigend,
                        sixtyfour,
                        [SegmentDesc(1, 0x1234, segment_contents)],
                    )
                    self.elf2bin("--vhx", "-o", "output.hex", "input.elf")
                    with open("output.hex") as fh:
                        hexdata = fh.read()
                    self.assertEqual(hexdata, to_vhx(segment_contents))

    def testMultiSegments(self):
        "Test multiple segments via -O."
        segment1_contents = bytes(range(20))
        segment2_contents = bytes(range(20, 32))

        for bigend in False, True:
            for sixtyfour in False, True:
                with self.tempsubdir():
                    makeelf(
                        "input.elf",
                        bigend,
                        sixtyfour,
                        [
                            SegmentDesc(1, 0x1234, segment1_contents),
                            SegmentDesc(1, 0x123456, segment2_contents),
                        ],
                    )

                    self.elf2bin("--vhx", "-O", "output-%a.hex", "input.elf")
                    with open("output-1234.hex") as fh:
                        hexdata = fh.read()
                    self.assertEqual(hexdata, to_vhx(segment1_contents))
                    with open("output-123456.hex") as fh:
                        hexdata = fh.read()
                    self.assertEqual(hexdata, to_vhx(segment2_contents))

    def testCombined(self):
        "Test basic vhxcombined output."
        segment1_contents = bytes(range(0x10))
        segment2_contents = bytes(range(0x10, 0x20))

        for bigend in False, True:
            for sixtyfour in False, True:
                with self.tempsubdir():
                    makeelf(
                        "input.elf",
                        bigend,
                        sixtyfour,
                        [
                            SegmentDesc(1, 0x1234, segment1_contents),
                            SegmentDesc(1, 0x1264, segment2_contents),
                        ],
                    )

                    self.elf2bin(
                        "--vhxcombined", "-o", "output.hex", "input.elf"
                    )
                    with open("output.hex") as fh:
                        hexdata = fh.read()
                    self.assertEqual(
                        hexdata,
                        to_vhx(
                            segment1_contents
                            + b"\0" * 0x20
                            + segment2_contents
                        ),
                    )


class banks(Base):
    def testParameters(self):
        "Test the 'width' and 'banks' parameters."
        input_data = bytes(range(32))

        for mod in [2, 4, 8, 16]:
            for width in [1, 2, 4, 8]:
                if width >= mod:
                    continue
                banks = mod // width

                with self.tempsubdir():
                    makeelf(
                        "input.elf",
                        False,
                        True,
                        [SegmentDesc(1, 0x1000, input_data)],
                    )
                    self.elf2bin(
                        "--bin",
                        "-O",
                        "output-%b.bin",
                        "input.elf",
                        "--banks",
                        f"{width}x{banks}",
                    )

                    for bank in range(banks):
                        with open(f"output-{bank}.bin", "rb") as fh:
                            bindata = fh.read()
                        self.assertEqual(
                            bindata,
                            bytes(
                                b
                                for i, b in enumerate(input_data)
                                if (b % mod)
                                in range(bank * width, (bank + 1) * width)
                            ),
                        )

    def testFormats(self):
        "Test that bank interleaving works for all four bin/vhx formats."
        segment_contents1 = bytes(range(0x10))
        segment_contents2 = bytes(range(0x10, 0x20))

        with self.tempsubdir():
            makeelf(
                "input.elf",
                False,
                True,
                [
                    SegmentDesc(1, 0x1000, segment_contents1),
                    SegmentDesc(1, 0x1020, segment_contents2),
                ],
            )

            self.elf2bin(
                "--bin",
                "-O",
                "binmulti-%a-%b.bin",
                "input.elf",
                "--banks",
                f"4x2",
            )
            with open(f"binmulti-1000-0.bin", "rb") as fh:
                bindata = fh.read()
                self.assertEqual(bindata, b"\x00\x01\x02\x03\x08\x09\x0a\x0b")
            with open(f"binmulti-1000-1.bin", "rb") as fh:
                bindata = fh.read()
                self.assertEqual(bindata, b"\x04\x05\x06\x07\x0c\x0d\x0e\x0f")
            with open(f"binmulti-1020-0.bin", "rb") as fh:
                bindata = fh.read()
                self.assertEqual(bindata, b"\x10\x11\x12\x13\x18\x19\x1a\x1b")
            with open(f"binmulti-1020-1.bin", "rb") as fh:
                bindata = fh.read()
                self.assertEqual(bindata, b"\x14\x15\x16\x17\x1c\x1d\x1e\x1f")

            self.elf2bin(
                "--bincombined",
                "-O",
                "bincombined-%b.bin",
                "input.elf",
                "--banks",
                f"4x2",
            )
            with open(f"bincombined-0.bin", "rb") as fh:
                bindata = fh.read()
                self.assertEqual(
                    bindata,
                    b"\x00\x01\x02\x03\x08\x09\x0a\x0b"
                    b"\x00\x00\x00\x00\x00\x00\x00\x00"
                    b"\x10\x11\x12\x13\x18\x19\x1a\x1b",
                )
            with open(f"bincombined-1.bin", "rb") as fh:
                bindata = fh.read()
                self.assertEqual(
                    bindata,
                    b"\x04\x05\x06\x07\x0c\x0d\x0e\x0f"
                    b"\x00\x00\x00\x00\x00\x00\x00\x00"
                    b"\x14\x15\x16\x17\x1c\x1d\x1e\x1f",
                )

            self.elf2bin(
                "--vhx",
                "-O",
                "vhxmulti-%a-%b.hex",
                "input.elf",
                "--banks",
                f"4x2",
            )
            with open(f"vhxmulti-1000-0.hex", "r") as fh:
                hexdata = fh.read()
                self.assertEqual(hexdata, "00\n01\n02\n03\n08\n09\n0A\n0B\n")
            with open(f"vhxmulti-1000-1.hex", "r") as fh:
                hexdata = fh.read()
                self.assertEqual(hexdata, "04\n05\n06\n07\n0C\n0D\n0E\n0F\n")
            with open(f"vhxmulti-1020-0.hex", "r") as fh:
                hexdata = fh.read()
                self.assertEqual(hexdata, "10\n11\n12\n13\n18\n19\n1A\n1B\n")
            with open(f"vhxmulti-1020-1.hex", "r") as fh:
                hexdata = fh.read()
                self.assertEqual(hexdata, "14\n15\n16\n17\n1C\n1D\n1E\n1F\n")

            self.elf2bin(
                "--vhxcombined",
                "-O",
                "vhxcombined-%b.hex",
                "input.elf",
                "--banks",
                f"4x2",
            )
            with open(f"vhxcombined-0.hex", "r") as fh:
                hexdata = fh.read()
                self.assertEqual(
                    hexdata,
                    "00\n01\n02\n03\n08\n09\n0A\n0B\n"
                    "00\n00\n00\n00\n00\n00\n00\n00\n"
                    "10\n11\n12\n13\n18\n19\n1A\n1B\n",
                )
            with open(f"vhxcombined-1.hex", "r") as fh:
                hexdata = fh.read()
                self.assertEqual(
                    hexdata,
                    "04\n05\n06\n07\n0C\n0D\n0E\n0F\n"
                    "00\n00\n00\n00\n00\n00\n00\n00\n"
                    "14\n15\n16\n17\n1C\n1D\n1E\n1F\n",
                )


class format(Base):
    def testMultiFile(self):
        "Test detection of file-written-multiple-times errors."
        with self.tempsubdir():
            makeelf(
                "one.elf",
                False,
                False,
                [SegmentDesc(1, 0x1234, b"a"), SegmentDesc(1, 0x123456, b"b")],
            )
            makeelf(
                "two.elf",
                True,
                True,
                [SegmentDesc(1, 0x1234, b"c"), SegmentDesc(1, 0x234567, b"d")],
            )

            # Expect that we can't give an output specification that
            # lands both segments of one object in the same output file
            err = self.elf2bin(
                "--bin", "-o", "output.bin", "one.elf", expect_error=True
            )
            self.assertEqual(
                err,
                "elf2bin: output file 'output.bin' "
                "would be written more than once by this command",
            )

            # Same for two files and a single output
            err = self.elf2bin(
                "--ihex",
                "-o",
                "output.hex",
                "one.elf",
                "two.elf",
                expect_error=True,
            )
            self.assertEqual(
                err,
                "elf2bin: output file 'output.hex' "
                "would be written more than once by this command",
            )

            # Or for binary files that overlap a base address
            err = self.elf2bin(
                "--bin",
                "-O",
                "%a.bin",
                "one.elf",
                "two.elf",
                expect_error=True,
            )
            self.assertEqual(
                err,
                "elf2bin: output file '1234.bin' "
                "would be written more than once by this command",
            )

            # But it's OK to generate two hex files using just %f
            self.elf2bin("--ihex", "-O", "%f.hex", "one.elf", "two.elf")

            # And it's OK to generate two bin files from one object
            # using just %a
            self.elf2bin("--bin", "-O", "%a.hex", "one.elf")

    def testHex(self):
        "Test formatting of hex numbers."
        with self.tempsubdir():
            makeelf(
                "input.elf",
                False,
                True,
                [SegmentDesc(1, 0x0123456789ABCDEF, b"a")],
            )
            self.elf2bin("--bin", "-O", "output-%a.bin", "input.elf")
            self.assertTrue(os.path.exists("output-123456789abcdef.bin"))

        with self.tempsubdir():
            makeelf(
                "input.elf",
                False,
                True,
                [SegmentDesc(1, 0x0123456789ABCDEF, b"a")],
            )
            self.elf2bin("--bin", "-O", "output-%A.bin", "input.elf")
            self.assertTrue(os.path.exists("output-123456789ABCDEF.bin"))

        with self.tempsubdir():
            makeelf("input.elf", False, True, [SegmentDesc(1, 0, b"a")])
            self.elf2bin("--bin", "-O", "output-%a.bin", "input.elf")
            self.assertTrue(os.path.exists("output-0.bin"))

    def testFilename(self):
        "Test formatting of parts of the input file name."
        with self.tempsubdir():
            makeelf("input.elf", False, True, [SegmentDesc(1, 0, b"a")])
            self.elf2bin("--bin", "-O", "out-%f.bin", "./input.elf")
            self.assertTrue(os.path.exists("out-input.bin"))

        with self.tempsubdir():
            makeelf("input.elf", False, True, [SegmentDesc(1, 0, b"a")])
            self.elf2bin("--bin", "-O", "out-%F.bin", "./input.elf")
            self.assertTrue(os.path.exists("out-input.elf.bin"))

        if os.name == "nt":
            # Also check backslash as a path separator
            with self.tempsubdir():
                makeelf("input.elf", False, True, [SegmentDesc(1, 0, b"a")])
                self.elf2bin("--bin", "-O", "out-%f.bin", ".\\input.elf")
                self.assertTrue(os.path.exists("out-input.bin"))

            # And a plain drive-letter prefix
            with self.tempsubdir():
                makeelf("input.elf", False, True, [SegmentDesc(1, 0, b"a")])
                fullpath = os.path.abspath("input.elf")
                assert fullpath[1] == ":"  # expect this to start with a drive
                testpath = fullpath[:2] + "input.elf"
                self.elf2bin("--bin", "-O", "out-%f.bin", testpath)
                self.assertTrue(os.path.exists("out-input.bin"))

            # And make sure those work with %F as well
            with self.tempsubdir():
                makeelf("input.elf", False, True, [SegmentDesc(1, 0, b"a")])
                self.elf2bin("--bin", "-O", "out-%F.bin", ".\\input.elf")
                self.assertTrue(os.path.exists("out-input.elf.bin"))
            with self.tempsubdir():
                makeelf("input.elf", False, True, [SegmentDesc(1, 0, b"a")])
                fullpath = os.path.abspath("input.elf")
                assert fullpath[1] == ":"  # expect this to start with a drive
                testpath = fullpath[:2] + "input.elf"
                self.elf2bin("--bin", "-O", "out-%F.bin", testpath)
                self.assertTrue(os.path.exists("out-input.elf.bin"))

        else:
            # If we're _not_ on Windows, make sure \ and : are treated
            # as ordinary filename characters
            with self.tempsubdir():
                makeelf("i\\p:t.elf", False, True, [SegmentDesc(1, 0, b"a")])
                self.elf2bin("--bin", "-O", "out-%f.bin", "i\\p:t.elf")
                self.assertTrue(os.path.exists("out-i\\p:t.bin"))
            with self.tempsubdir():
                makeelf("i\\p:t.elf", False, True, [SegmentDesc(1, 0, b"a")])
                self.elf2bin("--bin", "-O", "out-%F.bin", "i\\p:t.elf")
                self.assertTrue(os.path.exists("out-i\\p:t.elf.bin"))

    def testPercent(self):
        "Test the %% escape to generate a single %."
        with self.tempsubdir():
            makeelf("input.elf", False, True, [SegmentDesc(1, 0, b"a")])
            self.elf2bin("--bin", "-O", "%%.bin", "input.elf")
            self.assertTrue(os.path.exists("%.bin"))


class segselect(Base):
    def testBin(self):
        "Test --segments with multi-file binary output."
        segment_contents1 = bytes(range(0x10))
        segment_contents2 = bytes(range(0x10, 0x20))
        segment_contents3 = bytes(range(0x20, 0x30))

        with self.tempsubdir():
            makeelf(
                "input.elf",
                False,
                True,
                [
                    SegmentDesc(1, 0x1000, segment_contents1),
                    SegmentDesc(1, 0x1020, segment_contents2),
                    SegmentDesc(1, 0x1040, segment_contents3),
                ],
            )

            self.elf2bin(
                "--bin",
                "--segments",
                "0x1000,0x1040",
                "-O",
                "output-%a.bin",
                "input.elf",
            )
            with open(f"output-1000.bin", "rb") as fh:
                bindata = fh.read()
                self.assertEqual(bindata, segment_contents1)
            with open(f"output-1040.bin", "rb") as fh:
                bindata = fh.read()
                self.assertEqual(bindata, segment_contents3)
            self.assertFalse(os.path.exists("output-1020.bin"))

    def testBinOne(self):
        "Test --segments with binary output, selecting just one segment."
        segment_contents1 = bytes(range(0x10))
        segment_contents2 = bytes(range(0x10, 0x20))
        segment_contents3 = bytes(range(0x20, 0x30))

        with self.tempsubdir():
            makeelf(
                "input.elf",
                False,
                True,
                [
                    SegmentDesc(1, 0x1000, segment_contents1),
                    SegmentDesc(1, 0x1020, segment_contents2),
                    SegmentDesc(1, 0x1040, segment_contents3),
                ],
            )

            # The point is to check that now we can just use -o,
            # because we're writing only one file.
            self.elf2bin(
                "--bin",
                "--segments",
                "0x1020",
                "-o",
                "output.bin",
                "input.elf",
            )
            with open(f"output.bin", "rb") as fh:
                bindata = fh.read()
                self.assertEqual(bindata, segment_contents2)

    def testVhxCombined(self):
        "Test --segments with single-file VHX output."
        segment_contents1 = bytes(range(0x10))
        segment_contents2 = bytes(range(0x10, 0x20))
        segment_contents3 = bytes(range(0x20, 0x30))

        with self.tempsubdir():
            makeelf(
                "input.elf",
                False,
                True,
                [
                    SegmentDesc(1, 0x1000, segment_contents1),
                    SegmentDesc(1, 0x1020, segment_contents2),
                    SegmentDesc(1, 0x1040, segment_contents3),
                ],
            )

            self.elf2bin(
                "--vhxcombined",
                "--segments",
                "0x1000,0x1040",
                "-o",
                "output.hex",
                "input.elf",
            )
            with open(f"output.hex") as fh:
                hexdata = fh.read()
                self.assertEqual(
                    hexdata,
                    to_vhx(
                        segment_contents1 + b"\0" * 0x30 + segment_contents3
                    ),
                )

    def testIHex(self):
        "Test --segments with ihex mode."
        segment_contents1 = bytes(range(0x10))
        segment_contents2 = bytes(range(0x10, 0x20))
        segment_contents3 = bytes(range(0x20, 0x30))

        with self.tempsubdir():
            makeelf(
                "input.elf",
                False,
                True,
                [
                    SegmentDesc(1, 0x1000, segment_contents1),
                    SegmentDesc(1, 0x1020, segment_contents2),
                    SegmentDesc(1, 0x1040, segment_contents3),
                ],
            )

            self.elf2bin(
                "--ihex",
                "--segments",
                "0x1000,0x1040",
                "-o",
                "output.hex",
                "input.elf",
            )
            with open(f"output.hex") as fh:
                hexdata = fh.read()
            self.assertEqual(
                hexdata,
                """\
:10100000000102030405060708090A0B0C0D0E0F68
:10104000202122232425262728292A2B2C2D2E2F28
:0400000500000000F7
:00000001FF
""",
            )

    def testSRec(self):
        "Test --segments with srec mode."
        segment_contents1 = bytes(range(0x10))
        segment_contents2 = bytes(range(0x10, 0x20))
        segment_contents3 = bytes(range(0x20, 0x30))

        with self.tempsubdir():
            makeelf(
                "input.elf",
                False,
                True,
                [
                    SegmentDesc(1, 0x1000, segment_contents1),
                    SegmentDesc(1, 0x1020, segment_contents2),
                    SegmentDesc(1, 0x1040, segment_contents3),
                ],
            )

            self.elf2bin(
                "--srec",
                "--segments",
                "0x1000,0x1040",
                "-o",
                "output.hex",
                "input.elf",
            )
            with open(f"output.hex") as fh:
                hexdata = fh.read()
            self.assertEqual(
                hexdata,
                """\
S31500001000000102030405060708090A0B0C0D0E0F62
S31500001040202122232425262728292A2B2C2D2E2F22
S70500000000FA
""",
            )


class zi(Base):
    def testHex(self):
        "Test --zi with the complex hex output modes."
        segment_contents1 = bytes(range(19))
        segment_contents2 = bytes(range(23))

        # No need to test ihex and srec separately, since they share
        # the code in question anyway
        with self.tempsubdir():
            makeelf(
                "input.elf",
                False,
                True,
                [
                    SegmentDesc(1, 0x1000, segment_contents1, 43),
                    SegmentDesc(1, 0x1040, segment_contents2, 37),
                ],
            )

            self.elf2bin("--srec", "--zi", "-o", "output.hex", "input.elf")
            with open(f"output.hex") as fh:
                hexdata = fh.read()
            self.assertEqual(
                hexdata,
                """\
S31500001000000102030405060708090A0B0C0D0E0F62
S315000010101011120000000000000000000000000097
S3150000102000000000000000000000000000000000BA
S313000010300000000000000000000000000000AC
S31500001040000102030405060708090A0B0C0D0E0F22
S315000010501011121314151600000000000000000005
S31500001060000000000000000000000000000000007A
S311000010700000000000000000000000006E
S70500000000FA
""",
            )

    def testBinMulti(self):
        "Test --zi with multiple-file binary output."
        segment_contents1 = bytes(range(19))
        segment_contents2 = bytes(range(23))

        # No need to test bincombined and vhxcombined separately,
        # since they share the code in question anyway
        with self.tempsubdir():
            makeelf(
                "input.elf",
                False,
                True,
                [
                    SegmentDesc(1, 0x1000, segment_contents1, 43),
                    SegmentDesc(1, 0x1040, segment_contents2, 37),
                ],
            )

            self.elf2bin("--bin", "--zi", "-O", "output-%a.bin", "input.elf")
            with open(f"output-1000.bin", "rb") as fh:
                bindata = fh.read()
            self.assertEqual(bindata, segment_contents1 + b"\0" * 43)
            with open(f"output-1040.bin", "rb") as fh:
                bindata = fh.read()
            self.assertEqual(bindata, segment_contents2 + b"\0" * 37)

    def testBinCombined(self):
        "Test --zi with single-file binary output."
        segment_contents1 = bytes(range(19))
        segment_contents2 = bytes(range(23))

        # No need to test bincombined and vhxcombined separately,
        # since they share the code in question anyway
        with self.tempsubdir():
            makeelf(
                "input.elf",
                False,
                True,
                [
                    SegmentDesc(1, 0x1000, segment_contents1, 43),
                    SegmentDesc(1, 0x1040, segment_contents2, 37),
                ],
            )

            self.elf2bin(
                "--bincombined", "--zi", "-o", "output.bin", "input.elf"
            )
            with open(f"output.bin", "rb") as fh:
                bindata = fh.read()
            self.assertEqual(
                bindata,
                segment_contents1
                + b"\0" * (0x40 - len(segment_contents1))
                + segment_contents2
                + b"\0" * 37,
            )

    def testBinCombined32(self):
        "Test --zi in a 32-bit file."
        segment_contents1 = bytes(range(19))
        segment_contents2 = bytes(range(23))

        # This just checks that p_memsz is correctly extracted from
        # the other form of ELF program header table entry; there's no
        # need to re-test every mode that uses it
        with self.tempsubdir():
            makeelf(
                "input.elf",
                False,
                False,
                [
                    SegmentDesc(1, 0x1000, segment_contents1, 43),
                    SegmentDesc(1, 0x1040, segment_contents2, 37),
                ],
            )

            self.elf2bin(
                "--bincombined", "--zi", "-o", "output.bin", "input.elf"
            )
            with open(f"output.bin", "rb") as fh:
                bindata = fh.read()
            self.assertEqual(
                bindata,
                segment_contents1
                + b"\0" * (0x40 - len(segment_contents1))
                + segment_contents2
                + b"\0" * 37,
            )


class vaddr(Base):
    def testVirtual(self):
        "Test --virtual and --physical."
        for sixtyfour in False, True:
            with self.tempsubdir():
                # Input file whose single segment has different physical
                # and virtual addresses
                makeelf(
                    "input.elf",
                    False,
                    sixtyfour,
                    [
                        SegmentDesc(
                            segtype=1, paddr=0x1234, vaddr=0x5678, data=b"a"
                        )
                    ],
                )

                # Expected Intel Hex output if the segment's physical
                # address 0x1234 is used
                expected_physical = """\
:011234006158
:0400000500000000F7
:00000001FF
"""

                # Expected Intel Hex output if the segment's virtual
                # address 0x5678 is used
                expected_virtual = """\
:0156780061D0
:0400000500000000F7
:00000001FF
"""

                # Explicitly select --physical
                self.elf2bin(
                    "--physical", "--ihex", "-o", "output.hex", "input.elf"
                )
                with open("output.hex") as fh:
                    hexdata = fh.read()
                self.assertEqual(hexdata, expected_physical)

                # Explicitly select --virtual
                self.elf2bin(
                    "--virtual", "--ihex", "-o", "output.hex", "input.elf"
                )
                with open("output.hex") as fh:
                    hexdata = fh.read()
                self.assertEqual(hexdata, expected_virtual)

                # Use the default, which we expect to match -- physical
                self.elf2bin("--ihex", "-o", "output.hex", "input.elf")
                with open("output.hex") as fh:
                    hexdata = fh.read()
                self.assertEqual(hexdata, expected_physical)


def main():
    parser = argparse.ArgumentParser(description="Run tests.")
    parser.add_argument("elf2bin", help="Path to elf2bin binary under test")
    parser.add_argument(
        "-d",
        "--directory",
        help="Store test files here rather than in a temporary directory. "
        "This allows you to examine them afterwards and re-run the test "
        "commands outside the test harness.",
    )
    parser.add_argument(
        "testargs",
        nargs=argparse.REMAINDER,
        help="Arguments for unittest.main()",
    )
    args = parser.parse_args()

    global elf2bin_path
    elf2bin_path = os.path.abspath(args.elf2bin)

    main = lambda: unittest.main(argv=[sys.argv[0]] + args.testargs)

    global tempdir
    if args.directory:
        tempdir = args.directory
        try:
            os.mkdir(tempdir)
        except FileExistsError:
            pass
        main()
    else:
        with tempfile.TemporaryDirectory() as td:
            tempdir = td
            main()


if __name__ == "__main__":
    main()
