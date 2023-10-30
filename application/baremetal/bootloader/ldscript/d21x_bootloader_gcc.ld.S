/*
 * Copyright (c) 2022, Artinchip Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/*
 ******************************************************************************
                d21x Memory Layout
 ******************************************************************************

0x40000000+----------+
          |          |
          | Software |
          |          |
          |          |
          |          |
          |          |
          |          |
          |          |
          |          |
          |          |
          |          |
          |          |
          +----------+
          |   CMA    |
          |          |
          | ge/ve/de |
          |          |
0x43FFFFFF+----------+
 */

#include "rtconfig.h"

MEMORY
{
    DRAM_SW     : ORIGIN = 0x42000100, LENGTH = AIC_DRAM_TOTAL_SIZE - 0x2000000
}

PROVIDE (__dram_start             = 0x40000000);
PROVIDE (__dram_end               = 0x40000000 + AIC_DRAM_TOTAL_SIZE);
PROVIDE (__dram_sw_heap_end       = __dram_end);

PROVIDE (__min_heap_size           = 0x200);
PROVIDE (__heap_start              = __dram_sw_heap_start);
PROVIDE (__heap_end                = __dram_sw_heap_end);

REGION_ALIAS("REGION_TEXT"          , DRAM_SW);
REGION_ALIAS("REGION_RODATA"        , DRAM_SW);
REGION_ALIAS("REGION_DATA"          , DRAM_SW);
REGION_ALIAS("REGION_BSS"           , DRAM_SW);
REGION_ALIAS("REGION_DRAM_SW"       , DRAM_SW);

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

 ._dram_sw_sw_heap : {
  . = ALIGN(0x8) ;
  __dram_sw_heap_start = .;
  . += __min_heap_size;
  . = ALIGN(0x8) ;
 } > REGION_DRAM_SW AT > REGION_DRAM_SW

}
