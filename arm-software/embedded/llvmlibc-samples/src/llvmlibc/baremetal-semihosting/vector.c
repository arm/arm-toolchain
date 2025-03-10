// Copyright (c) 2024-2025, Arm Limited and affiliates.
//
// Part of the Arm Toolchain project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception

#include <stdlib.h>
#include <stdint.h>

/* Adapted from https://developer.arm.com/documentation/107565/0101/Use-case-examples/Generic-Information/What-is-inside-a-program-image-/Vector-table */

extern void __llvm_libc_exit();

extern uint8_t __stack[];

extern void _start(void);


void NMI_Handler() {}
void HardFault_Handler() { __llvm_libc_exit(); }
void MemManage_Handler() { __llvm_libc_exit(); }
void BusFault_Handler() { __llvm_libc_exit(); }
void UsageFault_Handler() { __llvm_libc_exit(); }
void SVC_Handler() {}
void DebugMon_Handler() {}
void PendSV_Handler() {}
void SysTick_Handler() {}
typedef void(*VECTOR_TABLE_Type)(void);
const VECTOR_TABLE_Type __VECTOR_TABLE[496] __attribute__((section(".vectors"))) __attribute__((aligned(128))) = {
  (VECTOR_TABLE_Type)__stack,               /*     Initial Stack Pointer */
  _start,                                   /*     Reset Handler */
  NMI_Handler,                              /*     NMI Handler */
  HardFault_Handler,                        /*     Hard Fault Handler */
  MemManage_Handler,                        /*     MPU Fault Handler */
  BusFault_Handler,                         /*     Bus Fault Handler */
  UsageFault_Handler,                       /*     Usage Fault Handler */
  0,                                        /*     Reserved */
  0,                                        /*     Reserved */
  0,                                        /*     Reserved */
  0,                                        /*     Reserved */
  SVC_Handler,                              /*     SVC Handler */
  DebugMon_Handler,                         /*     Debug Monitor Handler */
  0,                                        /*     Reserved */
  PendSV_Handler,                           /*     PendSV Handler */
  SysTick_Handler,                          /*     SysTick Handler */
  /* Unused */
};
