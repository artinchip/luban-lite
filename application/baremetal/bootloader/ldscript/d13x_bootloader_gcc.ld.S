/*
 * Copyright (c) 2022, Artinchip Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/*
 ******************************************************************************
                d13x Memory Layout
 ******************************************************************************

           配置(1)        配置(2)       配置(3)
          SRAM_S1 OFF   SRAM_S1 OFF    SRAM_S1 ON
              TCM ON        TCM OFF
0x30040000+----------+  +----------+ +----------+
          |   ITCM   |  |          | |          |
          |   128K   |  |          | |   TCM    |
0x30060000+----------+  |          | |    +     |
          |   DTCM   |  |          | | SRAM_S0  |
          |   128k   |  |  SRAM_S0 | |          |
0x30080000+----------+  |    1M    | |1M-sram_s1|
          |          |  |          | |   size   |
          |          |  |          | |          |
          |          |  |          | |          |
          |  SRAM_S0 |  |          | +----------+
          |   768K   |  |          |
          |          |  |          |
          |          |  |          |
          |          |  |          |
          |          |  |          |
0x3013FFFF+----------+  +----------+

0x40000000 ------------------------> +----------+
- sram_s1 size                       |  SRAM_S1 |
                                     |          |
                                     |128K/256K/|
                                     |384K/512K/|
                                     |640K/768K |
0x3FFFFFFF ------------------------> +----------+

0x40000000+----------+
          |          |
          |  PSRAM   |
          |  4M/8M   |
          |          |
          |          |
          +----------+
          |          |
          | FPGA Ext |
          |  PSRAM   |
          | 64M-psram|
          |     size |
          |          |
          |          |
          |          |
          |          |
          |          |
          |          |
0x43FFFFFF+----------+
 */

#include "rtconfig.h"

MEMORY
{
    /* The last 256KB for bootloader */
    SRAM_SW     : ORIGIN = 0x30100100, LENGTH = 0x40000
    PSRAM       : ORIGIN = 0x40000000, LENGTH = AIC_PSRAM_SIZE
}

PROVIDE (__sram_start             = 0x30040000);
PROVIDE (__sram_end               = 0x30040000 + AIC_SRAM_TOTAL_SIZE);

PROVIDE (__min_heap_size           = 0x200);
#ifdef AIC_BOOTLOADER_PSRAM_EN
PROVIDE (__heap_start              = 0x40000000);
/* Limit bootloader's heap to 8MB */
PROVIDE (__heap_end                = 0x40000000 + 0x800000);
#else
PROVIDE (__heap_start              = 0x300E0000);
PROVIDE (__heap_end                = 0x30100000);
#endif
REGION_ALIAS("REGION_TEXT"          , SRAM_SW);
REGION_ALIAS("REGION_RODATA"        , SRAM_SW);
REGION_ALIAS("REGION_DATA"          , SRAM_SW);
REGION_ALIAS("REGION_BSS"           , SRAM_SW);
REGION_ALIAS("REGION_SRAM_SW"       , SRAM_SW);

ENTRY(Reset_Handler)
SECTIONS
{
 .text : AT(ADDR(.text)){
  . = ALIGN(0x8) ;
  __stext = . ;
  KEEP(*startup_gcc.o(*.text*))
  *(.text)
  *(.text*)
  *(.text.*)
  *(.gnu.warning)
  *(.stub)
  *(.gnu.linkonce.t*)
  *(.glue_7t)
  *(.glue_7)
  *(.jcr)
  *(.init)
  *(.fini)

  . = ALIGN(0x10) ;
  __etext = . ;
 } > REGION_TEXT
 .eh_frame_hdr : {
  *(.eh_frame_hdr)
 } > REGION_TEXT
 .eh_frame : ONLY_IF_RO {
  KEEP (*(.eh_frame))
 } > REGION_TEXT
 .gcc_except_table : ONLY_IF_RO {
  *(.gcc_except_table .gcc_except_table.*)
 } > REGION_TEXT

 .rodata :{
  . = ALIGN(0x8) ;
  __srodata = .;
  *(.rdata)
  *(.rdata*)
  *(.rdata1)
  *(.rdata.*)
  *(.rodata)
  *(.rodata1)
  *(.rodata*)
  *(.rodata.*)
  *(.srodata*)
  *(.rodata.str1.4)
  . = ALIGN(0x8) ;
  __ctor_start__ = .;
  KEEP (*(SORT(.ctors.*)))
  KEEP (*(.ctors))
  __ctor_end__ = .;
  KEEP (*(SORT(.dtors.*)))
  KEEP (*(.dtors))
  __dtor_end__ = .;

  . = ALIGN(0x8) ;
  __erodata = .;
  __rodata_end__ = .;
 } > REGION_RODATA

 .data : {
  . = ALIGN(0x8) ;
  __data_start__ = . ;
  __sdata = . ;
  data_start = . ;
  KEEP(*startup_gcc.o(*.vectors*))
  *(.got.plt)
  *(.got)
  *(.gnu.linkonce.r*)
  *(.data)
  *(.data*)
  *(.data1)
  *(.data.*)
  *(.gnu.linkonce.d*)
  *(.data1)
  *(.gcc_except_table)
  *(.gcc_except_table*)
  . = ALIGN(8) ;
  __console_init_start = .;
  KEEP(*(.tinyspl.console.cmd))
  . = ALIGN(8) ;
  __console_init_end = .;
  __global_pointer$ = .;
  *(.sdata)
  *(.sdata.*)
  *(.gnu.linkonce.s.*)
  *(.note.ABI-tag)
  __edata = .;
  __data_end__ = .;
  . = ALIGN(0x8) ;

 } > REGION_DATA AT > REGION_RODATA
 .eh_frame : ONLY_IF_RW {
  KEEP (*(.eh_frame))
 } > REGION_DATA AT > REGION_RODATA
 .gcc_except_table : ONLY_IF_RW {
  *(.gcc_except_table .gcc_except_table.*)
  __edata = .;
  __data_end__ = .;
 } > REGION_DATA AT > REGION_RODATA

 .bss : {
  . = ALIGN(0x8) ;
  __sbss = ALIGN(0x8) ;
  __bss_start__ = . ;
  *(.dynsbss)
  *(.sbss)
  *(.sbss.*)
  *(.scommon)
  *(.dynbss)
  *(.bss)
  *(.bss.*)
  *(COMMON)
  . = ALIGN(0x8) ;
  __ebss = . ;
  __end = . ;
  end = . ;
  __bss_end__ = .;
 } > REGION_BSS
}
