/******************************************************************************
 * @file     gcc_arm.ld
 * @brief    GNU Linker Script for Cortex-M based device
 * @version  V2.2.0
 * @date     20. January 2021
 ******************************************************************************/
/*
 * Copyright (c) 2009-2021 Arm Limited. All rights reserved.
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Licensed under the Apache License, Version 2.0 (the License); you may
 * not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an AS IS BASIS, WITHOUT
 * WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

/*
 *-------- <<< Use Configuration Wizard in Context Menu >>> -------------------
 */

/*---------------------- ITCM Configuration ----------------------------------
  <h> ITCM Configuration
    <o0> ITCM Base Address <0x0-0xFFFFFFFF:8>
    <o1> ITCM Size (in Bytes) <0x0-0xFFFFFFFF:8>
  </h>
  -----------------------------------------------------------------------------*/
__ITCM_BASE = 0x00000000;
__ITCM_SIZE = 0x00040000;

/*--------------------- Embedded DTCM Configuration ----------------------------
  <h> DTCM Configuration
    <o0> DTCM Base Address    <0x0-0xFFFFFFFF:8>
    <o1> DTCM Size (in Bytes) <0x0-0xFFFFFFFF:8>
  </h>
 -----------------------------------------------------------------------------*/
__DTCM_BASE = 0x20000000;
__DTCM_SIZE = 0x00010000;

/*--------------------- Embedded SRAM Configuration ----------------------------
  <h> SRAM0 Configuration
    <o0> SRAM0 Base Address    <0x0-0xFFFFFFFF:8>
    <o1> SRAM0 Size (in Bytes) <0x0-0xFFFFFFFF:8>
  </h>
 -----------------------------------------------------------------------------*/
__SRAM_BASE = 0x61000000;
__SRAM_SIZE = 0x00200000;

/*--------------------- Embedded ReRAM Configuration ----------------------------
  <h> ReRAM Configuration
    <o0> ReRAM Base Address    <0x0-0xFFFFFFFF:8>
    <o1> ReRAM Size (in Bytes) <0x0-0xFFFFFFFF:8>
  </h>
 -----------------------------------------------------------------------------*/
__ReRAM_BASE = 0x60020000;
__ReRAM_SIZE = 0x002E0000;

/*--------------------- IFRAM Configuration ----------------------------
  <h> IFRAM Configuration
    <o0> IFRAM Base Address    <0x0-0xFFFFFFFF:8>
    <o1> IFRAM Size (in Bytes) <0x0-0xFFFFFFFF:8>
  </h>
 -----------------------------------------------------------------------------*/
__IFRAM_BASE = 0x50000000;
__IFRAM_SIZE = 0x00040000;

/*--------------------- AORAM Configuration ----------------------------
  <h> AORAM Configuration
    <o0> AORAM Base Address    <0x0-0xFFFFFFFF:8>
    <o1> AORAM Size (in Bytes) <0x0-0xFFFFFFFF:8>
  </h>
 -----------------------------------------------------------------------------*/
__AORAM_BASE = 0x50300000;
__AORAM_SIZE = 0x00004000;

/*--------------------- Stack / Heap Configuration ----------------------------
  <h> Stack / Heap Configuration
    <o0> Stack Size (in Bytes) <0x0-0xFFFFFFFF:8>
    <o1> Heap Size (in Bytes) <0x0-0xFFFFFFFF:8>
  </h>
  -----------------------------------------------------------------------------*/
__STACK_SIZE      = 0x00002000;
__MIN_HEAP_SIZE   = 0x00008000;

/*
 *-------------------- <<< end of configuration section >>> -------------------
 */

/* ARMv8-M stack sealing:
   to use ARMv8-M stack sealing set __STACKSEAL_SIZE to 8 otherwise keep 0
 */
__STACKSEAL_SIZE = 0;


MEMORY
{
  RERAM (rx)  : ORIGIN = __ReRAM_BASE,    LENGTH = __ReRAM_SIZE
  SRAM  (rwx)  : ORIGIN = __SRAM_BASE,     LENGTH = __SRAM_SIZE
  ITCM  (rwx)  : ORIGIN = __ITCM_BASE,     LENGTH = __ITCM_SIZE
  DTCM  (rw)  : ORIGIN = __DTCM_BASE,     LENGTH = __DTCM_SIZE
  IFRAM (rw)  : ORIGIN = __IFRAM_BASE,    LENGTH = __IFRAM_SIZE
  AORAM (rw)  : ORIGIN = __AORAM_BASE,    LENGTH = __AORAM_SIZE
}

REGION_ALIAS("DEFAULT_ROM", SRAM);
REGION_ALIAS("DEFAULT_RAM", SRAM);

/* Linker script to place sections and symbol values. Should be used together
 * with other linker script that defines memory regions FLASH and RAM.
 * It references following symbols, which must be defined in code:
 *   Reset_Handler : Entry of reset handler
 *
 * It defines following symbols, which code can use without definition:
 *   __exidx_start
 *   __exidx_end
 *   __copy_table_start__
 *   __copy_table_end__
 *   __zero_table_start__
 *   __zero_table_end__
 *   __etext
 *   __data_start__
 *   __preinit_array_start
 *   __preinit_array_end
 *   __init_array_start
 *   __init_array_end
 *   __fini_array_start
 *   __fini_array_end
 *   __data_end__
 *   __bss_start__
 *   __bss_end__
 *   __end__
 *   end
 *   __HeapBase
 *   __HeapLimit
 *   __StackLimit
 *   __StackTop
 *   __stack
 */
ENTRY(Reset_Handler)

SECTIONS
{
  .text :
  {
    __text_start__ = .;
    KEEP(*(.vectors))
    *(.text*)
    *(.SRAMCODE)

    KEEP(*(.init))
    KEEP(*(.fini))

    /* .ctors */
    *crtbegin.o(.ctors)
    *crtbegin?.o(.ctors)
    *(EXCLUDE_FILE(*crtend?.o *crtend.o) .ctors)
    *(SORT(.ctors.*))
    *(.ctors)

    /* .dtors */
    *crtbegin.o(.dtors)
    *crtbegin?.o(.dtors)
    *(EXCLUDE_FILE(*crtend?.o *crtend.o) .dtors)
    *(SORT(.dtors.*))
    *(.dtors)

    *(.rodata*)

    KEEP(*(.eh_frame*))
  } > DEFAULT_ROM

  .ARM.extab (READONLY) :
  {
    *(.ARM.extab* .gnu.linkonce.armextab.*)
  } > DEFAULT_ROM

  __exidx_start = .;
  .ARM.exidx(READONLY) :
  {
    *(.ARM.exidx* .gnu.linkonce.armexidx.*)
  } > DEFAULT_ROM
  __exidx_end = .;

  .copy.table (READONLY) :
  {
    . = ALIGN(4);
    __copy_table_start__ = .;
    __copy_table_end__ = .;
  } > DEFAULT_ROM

  .zero.table (READONLY) :
  {
    . = ALIGN(4);
    __zero_table_start__ = .;

    LONG (__bss_start__)
    LONG ((__bss_end__ - __bss_start__) / 4)

    LONG (_ifram_start_)
    LONG ((_ifram_end_ - _ifram_start_) / 4)

    __zero_table_end__ = .;
  } > DEFAULT_ROM

  /**
   * Location counter can end up 2byte aligned with narrow Thumb code but
   * __etext is assumed by startup code to be the LMA of a section in RAM
   * which must be 4byte aligned
   */
  __etext = ALIGN (4);

  .data :
  {
    __data_start__ = .;
    *(vtable)
    *(.data)
    *(.data.*)

    . = ALIGN(4);
    /* preinit data */
    PROVIDE_HIDDEN (__preinit_array_start = .);
    KEEP(*(.preinit_array))
    PROVIDE_HIDDEN (__preinit_array_end = .);

    . = ALIGN(4);
    /* init data */
    PROVIDE_HIDDEN (__init_array_start = .);
    KEEP(*(SORT(.init_array.*)))
    KEEP(*(.init_array))
    PROVIDE_HIDDEN (__init_array_end = .);

    . = ALIGN(4);
    /* finit data */
    PROVIDE_HIDDEN (__fini_array_start = .);
    KEEP(*(SORT(.fini_array.*)))
    KEEP(*(.fini_array))
    PROVIDE_HIDDEN (__fini_array_end = .);

    KEEP(*(.jcr*))
    . = ALIGN(4);
    /* All data end */
    __data_end__ = .;
  } > DEFAULT_RAM AT > DEFAULT_RAM

  .stack (NOLOAD) :
  {
    __ram_start__ = .;
    . = ALIGN(8);
    __StackLimit = .;
    . = . + __STACK_SIZE;
    . = ALIGN(8);
    __StackTop = .;
  } > DEFAULT_RAM AT > DEFAULT_RAM
  PROVIDE(__stack = __StackTop);

  .bss (NOLOAD) :
  {
    . = ALIGN(4);
    __bss_start__ = .;
    *(.bss)
    *(.bss.*)
    *(COMMON)
    . = ALIGN(4);
    __bss_end__ = .;
    __ram_end__ = .;
  } > DEFAULT_RAM AT > DEFAULT_RAM

  /* Check if remaining space >= minimum required HEAP size */
  __HEAP_SIZE = LENGTH(DEFAULT_RAM) - (__bss_end__ - __text_start__);
  ASSERT(__HEAP_SIZE >= __MIN_HEAP_SIZE, "DEFAULT_RAM overflowed")

  .heap (NOLOAD) :
  {
    __HeapBase = .;
    . = . + __HEAP_SIZE;
    __HeapLimit = .;
  } > DEFAULT_RAM  AT > DEFAULT_RAM

  .ifram (NOLOAD) :
  {
    . = ALIGN(4);
    _ifram_start_ = .;
    *(ifram)
    *(ifram*)
    . = ALIGN(4);
    _ifram_end_ = .;
  } > IFRAM AT > IFRAM

  .itcmdata (NOLOAD) :
  {
    . = 4;
    _itcmdata_start_ = .;
    *(itcmdata)
    *(itcmdata*)
    . = ALIGN(4);
    _itcmdata_end_ = .;
  } > ITCM AT > ITCM

  .dtcmdata (NOLOAD) :
  {
    . = ALIGN(4);
    _dtcmdata_start_ = .;
    *(dtcmdata)
    *(dtcmdata*)
    . = ALIGN(4);
    _dtcmdata_end_ = .;
  } > DTCM AT > DTCM

  .aoram_boot_info (NOLOAD) :
  {
    . = ALIGN(4);
    _aoram_boot_info_ = .;
    . = . + 0x100;
  } > AORAM AT > AORAM

  .aoram_user (NOLOAD) :
  {
    . = ALIGN(4);
    _aoram_user_start_ = .;
    *(aoram_user)
    *(aoram_user*)
    . = ALIGN(4);
    _aoram_user_end_ = .;
  } > AORAM AT > AORAM
}
