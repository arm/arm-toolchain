/* Copyright (c) 2023-2026, Arm Limited and affiliates. 
 * 
 * Part of the Arm Toolchain project, under the Apache License v2.0 with LLVM Exceptions.
 * See https://llvm.org/LICENSE.txt for license information.
 * SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception */

/* deflate-fixed.c - decode Deflate (RFC 1951), without support for
 * dynamic Huffman trees
 */

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

/*
 * This source file can be compiled two ways.
 *
 * Compiled without any special #defines, it generates an object
 * module intended to be linked into an embedded image, containing a
 * decompressor in the form of a single function taking two pointer
 * arguments for the source and destination data, which decompresses
 * from memory to memory.
 *
 * Compiled with -DSTANDALONE, it implements a full self-contained
 * command-line program containing the same decompression logic, but
 * reading from a file and writing to another file. This is useful for
 * debugging the decompressor itself. Also, you can use it as a build
 * step, to verify during linking that this decompression code really
 * does convert your compressed data to exactly the output you expect.
 * (That way, even if this code or compress.py does have a bug in it,
 * you have reasonable confidence that it's not a bug affecting *your*
 * input file.)
 *
 * Switching between these modes is done by defining 'struct input'
 * and 'struct output' to contain all necessary state for
 * reading/writing bytes of data.
 */
#ifndef STANDALONE

/*
 * In embedded mode, 'struct input' and 'struct output' are just
 * wrappers on single pointer variables, and the reading and writing
 * functions wrap trivial expressions of the form '*p++'. These should
 * be inlined at compile time.
 */
struct input {
  const unsigned char *p;
};
struct output {
  unsigned char *p;
};
static inline unsigned char input_char(struct input *in) { return *in->p++; }
static inline void output_char(struct output *out, unsigned char c) {
  *out->p++ = c;
}

#else

/*
 * In standalone mode, 'struct input' contains a stdio stream and
 * retrieves bytes via fgetc. 'struct output' writes to an in-memory
 * buffer rather than directly to the output file, so that past output
 * data is still available to use as the source of LZ77 copy
 * operations. But unlike the embedded version, this 'struct output'
 * reallocates the buffer on demand, instead of expecting an output
 * area of the correct size to already be available.
 */
struct input {
  FILE *fp;
};
struct output {
  unsigned char *buffer, *p;
  size_t size;
};
static inline unsigned char input_char(struct input *in) {
  int c = fgetc(in->fp);
  if (c == EOF) {
    fprintf(stderr, "unexpected end of file\n");
    exit(1);
  }
  return c;
}
static inline void output_char(struct output *out, unsigned char c) {
  size_t pos = out->p - out->buffer;
  if (pos >= out->size) {
    out->size = pos + pos / 4 + 1024;
    out->buffer = realloc(out->buffer, out->size);
    out->p = out->buffer + pos;
  }
  *out->p++ = c;
}

#endif /* STANDALONE */

/*
 * Most of the Deflate stream is bit- rather than byte-oriented, with
 * the bits in each byte considered to be ordered 'little-endian',
 * with the low-order bit 0 logically first and bit 7 last.
 *
 * The BitBuffer layer takes care of reading input a byte at a time
 * and permitting an arbitrary number of bits to be consumed.
 */
struct BitBuffer {
  /* 'nbits' indicates how many bits of the input are currently
   * held in this buffer. Must be at most 32, to fit in 'value'. */
  unsigned nbits;

  /* 'value' stores the actual input bits in question. The one that
   * comes logically first in the file is bit 0, and the one that
   * comes last is bit (nbits-1). */
  uint32_t value;
};

/*
 * bit_read and bit_read_reverse return the next 'nwanted' bits of the
 * input, reading extra bytes from 'in' if necessary.
 *
 * bit_read returns the bits in the same format as in BitBuffer, i.e.
 * with the logically first bit of the value as  bit 0. That's the
 * correct order for fixed-length multi-bit values in Deflate, such as
 * the block type code, uncompressed block lengths, or the extra bits
 * following a copy-length or copy-distance Huffman symbol.
 *
 * bit_read_reverse returns the bits in the opposite order, with bit
 * (nwanted-1) being the logically first bit, and bit 0 being the last
 * bit. That's the correct bit order for arithmetically processing
 * Huffman code values.
 */
static inline uint32_t bit_read_common(struct BitBuffer *bb, struct input *in,
                                unsigned nwanted) {
  /* Read bytes from the input until we have enough bits in the buffer */
  while (bb->nbits < nwanted) {
    bb->value |= input_char(in) << bb->nbits;
    bb->nbits += 8;
  }

  /* Return the full bb->value for our caller to process into the output */
  uint32_t ret = bb->value & ((1 << nwanted) - 1);

  /* Consume them by shifting 'value' downward */
  bb->value >>= nwanted;
  bb->nbits -= nwanted;

  return ret;
}
static inline uint32_t bit_read(struct BitBuffer *bb, struct input *in,
                                unsigned nwanted) {
  /* Make the return value by masking off all but the low-order bits */
  uint32_t value = bit_read_common(bb, in, nwanted);
  return value & ((1 << nwanted) - 1);
}

static inline uint32_t bitreverse32(uint32_t x) {
#ifdef __clang__
  return __builtin_bitreverse32(x);
#else
  x = ((x & 0x55555555) << 1) | ((x & 0xaaaaaaaa) >> 1);
  x = ((x & 0x33333333) << 2) | ((x & 0xcccccccc) >> 2);
  x = ((x & 0x0f0f0f0f) << 4) | ((x & 0xf0f0f0f0) >> 4);
  x = ((x & 0x00ff00ff) << 8) | ((x & 0xff00ff00) >> 8);
  return ((x & 0x0000ffff) << 16) | ((x & 0xffff0000) >> 16);
#endif
}

static inline uint32_t bit_read_reverse(struct BitBuffer *bb, struct input *in,
                                unsigned nwanted) {
  /* Make the return value by bit-reversing and shifting downward */
  uint32_t value = bit_read_common(bb, in, nwanted);
  return bitreverse32(value) >> (32 - nwanted);
}

/*
 * In the Deflate format, LZ77 length and distance values are stored
 * in the form of a Huffman code symbol giving the magnitude and a
 * high-order bit or two, followed by some additional low-order bits.
 *
 * RFC 1951 gives two lookup tables that map each symbol number to a
 * pair of values indicating how many extra bits should be read from
 * the stream, and what offset should be added to them to deliver the
 * final length or distance value.
 *
 * Here, we compute the same values arithmetically, since the table
 * format is mostly regular.
 *
 * The 'struct extra' returned from these functions has the semantics:
 * read 'bits' bits from the stream (in unreversed endianness), and
 * add 'offset' to them to get the final output value.
 */
struct extra {
  uint16_t bits, offset;
};

/* Process a copy-length symbol decoded from the initial 'literal or
 * length' Huffman tree. Input should be at least 257, which is the id
 * of the first copy-length symbol. (0-255 mean literals, and 256
 * means end of block). */
static struct extra len_extra(unsigned i) {
  struct extra ex;

  i -= 257; /* now the first length symbol has value 0 */

  if (i < 8) {
    ex.bits = 0;
    ex.offset = 3 + i;
    return ex;
  }
  if (i >= 28) {
    ex.bits = 0;
    ex.offset = 258;
    return ex;
  }
  ex.bits = (i >> 2) - 1;
  ex.offset = 3 + ((4 + (i & 3)) << ex.bits);
  return ex;
}

/* Process a copy-distance symbol decoded from the followup Huffman
 * tree used only for distances. */
static struct extra dist_extra(unsigned i) {
  struct extra ex;

  if (i < 4) {
    ex.bits = 0;
    ex.offset = 1 + i;
    return ex;
  }
  ex.bits = (i >> 1) - 1;
  ex.offset = 1 + ((2 + (i & 1)) << ex.bits);
  return ex;
}

/*
 * Main decompression function.
 *
 * In embedded mode, this takes a bare output and input pointer as
 * arguments, and initialises a 'struct input' and 'struct output'
 * internally to work with the abstraction above.
 *
 * In standalone mode, this takes a 'struct input' and 'struct output'
 * from its caller, and main() will set them up appropriately.
 */
#ifdef STANDALONE
void deflate_fixed(struct output *out, struct input *in)
#else
void deflate_fixed(unsigned char *outptr, const unsigned char *inptr)
#endif
{
#ifndef STANDALONE
  struct input in[1] = {{.p = inptr}};
  struct output out[1] = {{.p = outptr}};
#endif
  struct BitBuffer bb[1] = {{.value = 0, .nbits = 0}};

  /*
   * Each iteration of this while loop processes a single top-level
   * Deflate block.
   */
  while (1) {
    /*
     * Block header: one bit saying whether it's the last block in the
     * stream, and two bits saying what its type is.
     */
    unsigned bfinal = bit_read(bb, in, 1);
    unsigned btype = bit_read(bb, in, 2);

    switch (btype) {
    case 0: {
      /*
       * Block type 0 is a completely uncompressed block. Following
       * the block header, starting at the next byte boundary, we
       * expect to see a 16-bit length field, then another 16-bit
       * field containing its bitwise complement (presumably for error
       * checking), and then exactly that many bytes of literal data.
       */
      bit_read(bb, in, bb->nbits & 7); /* align to byte boundary */
      uint32_t blocklen = bit_read(bb, in, 16);
      uint32_t complement = bit_read(bb, in, 16);
#ifdef STANDALONE
      /* In standalone mode, check the two length values match. */
      if (blocklen != (0xFFFF ^ complement)) {
        fprintf(stderr, "uncomp block len mismatch!\n");
        exit(1);
      }
#else
      /* In embedded mode, save time and space by not bothering. */
      (void)complement; /* placate -Wunused-variable */
#endif
      for (uint32_t i = 0; i < blocklen; i++)
        output_char(out, bit_read(bb, in, 8));

      break;
    }
    case 1:
      /*
       * Block type 1 is a compressed block using the fixed
       * Huffman trees defined in RFC 1951.
       *
       * This inner while loop reads individual operations from
       * the block, each of which is either a literal, or a
       * copy, or the 'end of block' symbol.
       */
      while (1) {
        /*
         * Read and decode a value using the literal/length Huffman
         * tree, which has three lengths of code.
         *
         * All code lengths are at least 7 bits, so start by reading
         * the first 7 bits to decide what to do.
         */
        uint32_t symbol = bit_read_reverse(bb, in, 7);

        /*
         * Decode the symbol fully, based on those first 7 bits.
         *
         * In general you would append N extra bits by shifting
         * 'symbol' left by N and retrieving the new low bits via
         * bit_read_reverse. Here, we only do that in the case
         * where the number of extra bits is more than 1, because
         * bit-reversing a 1-bit value is a waste of effort.
         */
        if (symbol < 0x18) {
          /* Symbols 256, ..., 279 have 7-bit codes 0000000 to 0010111 */
          symbol += 256;
        } else if (symbol < 0x60) {
          /* Symbols 0, ..., 143   have 8-bit codes 00110000 to 10111111 */
          symbol = (symbol << 1) + bit_read(bb, in, 1) - 0x30;
        } else if (symbol < 0x64) {
          /* Symbols 280, ..., 287 have 8-bit codes 11000000 to 11000111 */
          symbol = (symbol << 1) + bit_read(bb, in, 1) - 0xC0 + 280;
        } else {
          /* Symbols 144, ..., 255 have 9-bit codes 110010000 to 111111111 */
          symbol = (symbol << 2) + bit_read_reverse(bb, in, 2) - 0x190 + 144;
        }

        /*
         * The symbol value we now have will take values 0-255 meaning
         * a literal character, or 256 meaning end of block, or 257 or
         * greater for an LZ77 copy.
         */
        if (symbol < 0x100) {
          output_char(out, symbol);
        } else if (symbol == 0x100) {
          break; /* end of block */
        } else {
          /*
           * Finish decoding the length, by reading extra bits from
           * the stream if necessary.
           */
          struct extra lenex = len_extra(symbol);
          uint32_t copylen = lenex.offset + bit_read(bb, in, lenex.bits);

          /*
           * Now read and decode a value using the secondary
           * copy-distance Huffman tree. The fixed one of these is
           * trivial: it simply assigns consecutive 5-bit codes to all
           * 32 of the legal symbol values.
           */
          uint32_t distsym = bit_read_reverse(bb, in, 5);
          struct extra distex = dist_extra(distsym);
          uint32_t copydist = distex.offset + bit_read(bb, in, distex.bits);

          /*
           * Now we know how many bytes to copy from where, so do it.
           */
          for (uint32_t i = 0; i < copylen; i++)
            output_char(out, *(out->p - copydist));
        }
      }
      break;
#ifdef STANDALONE
    default:
      /*
       * Block type 2 indicates a dynamic Huffman tree, which we
       * expect our compressor never to write. Type 3 is not
       * even defined by the Deflate format.
       */
      fprintf(stderr, "unsupported btype %u\n", btype);
      exit(1);
#endif
    }

    /*
     * If this block was the last in the stream, terminate the outer
     * loop.
     */
    if (bfinal)
      break;
  }
}

#ifdef STANDALONE

/*
 * Simple main program, for standalone mode.
 */

int main(int argc, char **argv) {
  if (argc != 3) {
    fprintf(stderr, "usage: deflate-fixed INFILE OUTFILE\n");
    exit(1);
  }

  struct input in[1];
  in->fp = fopen(argv[1], "rb");
  if (!in->fp) {
    fprintf(stderr, "deflate-fixed: unable to open '%s'\n", argv[1]);
    exit(1);
  }

  struct output out[1];
  out->size = 1024;
  out->buffer = out->p = malloc(out->size);

  deflate_fixed(out, in);

  FILE *fp = fopen(argv[2], "wb");
  if (!fp) {
    fprintf(stderr, "deflate-fixed: unable to open '%s'\n", argv[2]);
    exit(1);
  }
  fwrite(out->buffer, out->p - out->buffer, 1, fp);
  fclose(fp);
}

#endif
