// RUN: llvm-mc -triple=aarch64 -show-encoding -mattr=+pcdphint < %s \
// RUN:        | FileCheck %s --check-prefixes=CHECK-ENCODING,CHECK-INST
// RUN: not llvm-mc -triple=aarch64 -show-encoding < %s 2>&1 \
// RUN:        | FileCheck %s --check-prefixes=CHECK-ERROR
// RUN: llvm-mc -triple=aarch64 -filetype=obj -mattr=+pcdphint < %s \
// RUN:        | llvm-objdump -d --mattr=+pcdphint - | FileCheck %s --check-prefix=CHECK-INST
// RUN: llvm-mc -triple=aarch64 -filetype=obj -mattr=+pcdphint < %s \
// RUN:        | llvm-objdump -d --mattr=-pcdphint - | FileCheck %s --check-prefix=CHECK-UNKNOWN
// Disassemble encoding and check the re-encoding (-show-encoding) matches.
// RUN: llvm-mc -triple=aarch64 -show-encoding -mattr=+pcdphint < %s \
// RUN:        | sed '/.text/d' | sed 's/.*encoding: //g' \
// RUN:        | llvm-mc -triple=aarch64 -mattr=+pcdphint -disassemble -show-encoding \
// RUN:        | FileCheck %s --check-prefixes=CHECK-ENCODING,CHECK-INST

stshh keep
// CHECK-INST: stshh keep
// CHECK-ENCODING: encoding: [0x1f,0x26,0x03,0xd5]
// CHECK-ERROR: error: instruction requires: pcdphint
// CHECK-UNKNOWN:  d503261f      hint #0x30

stshh strm
// CHECK-INST: stshh strm
// CHECK-ENCODING: encoding: [0x3f,0x26,0x03,0xd5]
// CHECK-ERROR: error: instruction requires: pcdphint
// CHECK-UNKNOWN:  d503263f      hint #0x31
