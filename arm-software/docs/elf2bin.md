# `elf2bin`: convert ELF images to binary or hex via their segment view

`elf2bin` is a tool for converting ELF images to binary or hex
formats.

It does a similar job to `llvm-objcopy` with binary or hex output, but
unlike `llvm-objcopy`, it exclusively uses the ELF program header
table, ignoring the sections. So it can cope with ELF images which
have no section header table at all, or images whose sections don't
exactly match up to the segments, e.g. have a gap in a segment which
no section covers. Also, it supports a wider range of binary and hex
output options.

## Using `elf2bin`

The general format of an `elf2bin` command involves:

* An output mode option, telling `elf2bin` what kind of binary or hex
  output it's generating.

* One or more input file names, which must be ELF images or dynamic
  libraries.

* Either a single output file name, or a pattern that tells `elf2bin`
  how to name multiple output files.

* Optionally, other options to adjust behavior.

### Output modes

The available output-mode options are:

* `--ihex`: Intel hex format (the one where lines begin with '`:`').
  `elf2bin`'s take on this format is similar to `fromelf`'s: it always
  uses the record types that denote 32-bit linear addresses (`04` and
  `05`), and never the 8086-style segment:offset ones.

* `--srec`: Motorola hex format (the one where lines begin with
  '`S`'). Similarly, `elf2bin` keeps this as simple and consistent as
  possible: it always uses the 32-bit record types (`S3` and `S7`),
  and never the 16- or 24-bit ones.

* `--bin`: write out a binary file for each loadable segment in the
  image.

* `--bincombined`: write out a _single_ binary file, which contains
  all the loadable segments in the image, with padding between them to
  put them at the correct relative offsets from each other. The
  resulting file is suitable for loading at the base address of the
  first segment in memory (but you can adjust the base further
  downwards with `--base`).

* `--vhx` and `--vhxcombined`: Verilog hex format. The output of these
  modes is identical to `--bin` and `--bincombined`, except that the
  binary output file is post-converted into a text format with one
  2-digit hex number per line, representing the same sequence of bytes
  that would have appeared in the binary version.

### Output file naming

If `elf2bin` is writing a single output file, you can use the `-o` (or
`--output-file`) option to tell it the name of the file, e.g.

```
elf2bin --srec -o output.hex input.elf
```

But in many situations `elf2bin` will produce multiple output files:

* because you gave it multiple input files
* because you used `--bin` or `--vhx` on a file with multiple segments
* because you used `--banks` to split binary output into interleaved bank files
* more than one of the above

In that case, using `-o` will produce an error, because `elf2bin` will
notice that you've asked it to write more than one output file to the
same location. Instead, you must use the `-O` or `--output-pattern`
option to provide a _pattern_ for constructing each output file name.

Patterns look a bit like `printf` format strings: they consist of
literal characters interleaved with formatting directives introduced
by `%`. The available formatting directives are:

* `%f` expands to the base name of the input file, with directory path
  and file extension removed. For example, if an input file is called
  `foo/bar/baz.elf`, then `%f` will expand to just `baz` when
  generating output from that file.

* `%F` expands to the _full_ name of the input file, with the
  directory path still removed, but the extension left on. For
  example, `foo/bar/baz.elf` will turn into `baz.elf`.

* `%a` and `%A` expand to the base address of a particular ELF
  segment. These are for the `--bin` or `--vhx` modes, where each
  segment is output to a separate file. ELF contains no way to assign
  segments a human-readable name, so the base address is the simplest
  way to distinguish them. The address is generated in hex, with no
  leading zeroes (unless it's actually `0`). `%a` generates hex digits
  `a`-`f` in lower case, and `%A` generates them in upper case.

* `%b` expands to the bank number, if you're using `--banks` to split
  binary (or VHX) output into more than one bank. Banks are numbered
  consecutively upwards from 0, and are written in decimal.

* `%%` expands to a literal `%`, if you need one in the output file
  name.

Some examples:

```
elf2bin --ihex -O %f.hex one.elf two.elf                # generates one.hex and two.hex
elf2bin --ihex -O %F.hex one.elf two.elf                # generates one.elf.hex and two.elf.hex
elf2bin --bin -O out-%a.bin input.elf                   # might generate, say, out-0.bin and out-f000.bin
elf2bin --bin -O out-%A.bin input.elf                   # same but you'd get out-F000.bin
elf2bin --bincombined --banks 1x2 out-%b.bin input.elf  # out-0.bin and out-1.bin
elf2bin --srec -O out-%%.hex input.elf                  # just gives out-%.hex
```

In all cases, `elf2bin` will check its set of output files to ensure
you haven't tried to direct two output files to the same name.

In a complex case, you may need to use more than one of these
directives. For example, if you're using `--bin` with multiple ELF
files at once, some of which have multiple segments, _and_ you're
using bank interleaving, then you'll need to use all of `%f`, `%a` (or
`%A`) and `%b` to generate a distinct name for each output file:

```
elf2bin --bin --banks 2x4 -O %f-%a-%b.bin one.elf two.elf
```

### Other options

If you're using the `--bincombined` or `--vhxcombined` output modes,
you can use the `--base` option to specify the address you want the
output file to begin at. If this is lower than the start address of
any segment, `elf2bin` will prepend padding to the file. For example,
if `input.elf` has its lowest segment starting at 0x8000, then you'll
normally get an output file beginning with the data of that segment.
But adding `--base 0x6000` will give an output file beginning with
0x2000 zero bytes, so that you could load the whole file beginning at
address 0x6000 and all the segments would end up in the right places.

In binary and VHX formats, you can use `--banks` to request the output
split up into interleaved banks, for example so that you can direct a
CPU's 32-bit data bus to four ROMs each with an 8-bit data bus. You
specify the 'width' of each bank (number of consecutive bytes of data
that go into each bank file before moving on to the next), and the
number of banks. For example, `--banks 2x4` generates four banks, each
of which receives 2 consecutive bytes of the data in turn. That is,
the output file for bank 0 would get all the bytes intended to end up
in memory at addresses 0,1 (mod 8), bank 1 would get addresses 2,3
(mod 8), bank 2 would get 4,5 and bank 3 would get 6,7.

In the record-based hex formats `--ihex` and `--srec`, you can use
`--datareclen` to control the number of bytes of the ELF file that
appear in each data record. By default this is 16. The upper limit is
different for the two formats.

You can use `--segments` to restrict `elf2bin` to writing only a
subset of the loadable segments in the ELF file. The argument is a
comma-separated list of base addresses. For example, if you had an
input file containing segments at addresses 0x8000, 0x20000 and
0x10000000, then `--segments 0x8000,0x10000000` would skip the middle
one. This option applies to all output modes.

Normally, `elf2bin` treats each segment as containing only the bytes
actually stored in the ELF file. That is, the segment is treated as
having length corresponding to its `p_filesz` field, not its
`p_memsz`. You can use `--zi`, in any mode, to tell `elf2bin` to
include zero padding after each segment to bring it up to its
`p_memsz` length.
