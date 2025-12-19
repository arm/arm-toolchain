// Code to read ELF data structures for elf2bin
//
// Copyright (c) 2022-2025, Arm Limited and affiliates.
//
// Part of the Arm Toolchain project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//

#include "elf2bin.h"

using namespace llvm;
using namespace llvm::object;

template <typename ELFT>
static std::vector<Segment> get_segments(const InputObject &inobj,
                                         ELFObjectFile<ELFT> &elfobj,
                                         bool physical) {
  std::vector<Segment> segments;

  Expected<ArrayRef<typename ELFT::Phdr>> phdrs_or_err =
      elfobj.getELFFile().program_headers();
  if (!phdrs_or_err) {
    fatal(inobj, "unable to read program header table",
          phdrs_or_err.takeError());
    return segments;
  }

  auto &phdrs = *phdrs_or_err;
  if (phdrs.empty()) {
    fatal(inobj,
          "no program header table found (elf2bin only works on ELF "
          "executables or shared libraries, not relocatable object files)");
    return segments;
  }

  for (const typename ELFT::Phdr &phdr : phdrs) {
    if (phdr.p_type != llvm::ELF::PT_LOAD)
      continue;
    Segment seg;
    seg.fileoffset = phdr.p_offset;
    seg.baseaddr = physical ? phdr.p_paddr : phdr.p_vaddr;
    seg.filesize = phdr.p_filesz;
    seg.memsize = phdr.p_memsz;
    segments.push_back(seg);
  }

  return segments;
}

template <typename ELFT>
static uint64_t get_entry_point(const ELFObjectFile<ELFT> &obj) {
  return obj.getELFFile().getHeader().e_entry;
}

std::vector<Segment> InputObject::segments(bool physical) const {
  if (auto *specific = dyn_cast<ELF32LEObjectFile>(elf.get()))
    return get_segments(*this, *specific, physical);
  if (auto *specific = dyn_cast<ELF32BEObjectFile>(elf.get()))
    return get_segments(*this, *specific, physical);
  if (auto *specific = dyn_cast<ELF64LEObjectFile>(elf.get()))
    return get_segments(*this, *specific, physical);
  if (auto *specific = dyn_cast<ELF64BEObjectFile>(elf.get()))
    return get_segments(*this, *specific, physical);
  llvm_unreachable("unexpected subclass of ELFOBjectFileBase");
}

uint64_t InputObject::entry_point() const {
  if (auto *specific = dyn_cast<ELF32LEObjectFile>(elf.get()))
    return get_entry_point(*specific);
  if (auto *specific = dyn_cast<ELF32BEObjectFile>(elf.get()))
    return get_entry_point(*specific);
  if (auto *specific = dyn_cast<ELF64LEObjectFile>(elf.get()))
    return get_entry_point(*specific);
  if (auto *specific = dyn_cast<ELF64BEObjectFile>(elf.get()))
    return get_entry_point(*specific);
  llvm_unreachable("unexpected subclass of ELFOBjectFileBase");
}
