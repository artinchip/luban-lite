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

#ifdef CONFIG_ENABLE_ROM_API
INCLUDE rom_api.lds
#endif

MEMORY
{
    BROM        : ORIGIN = 0x00000000                                             , LENGTH = 64K
    DRAM_SW     : ORIGIN = 0x40000000                                             , LENGTH = AIC_DRAM_TOTAL_SIZE - AIC_DRAM_CMA_SIZE - 0x100
    DRAM_CMA    : ORIGIN = 0x40000000 + AIC_DRAM_TOTAL_SIZE - AIC_DRAM_CMA_SIZE   , LENGTH = AIC_DRAM_CMA_SIZE
}

PROVIDE (__dram_sw_start          = 0x40000000);
PROVIDE (__dram_sw_end            = 0x40000000 + AIC_DRAM_TOTAL_SIZE - AIC_DRAM_CMA_SIZE);
PROVIDE (__dram_cma_start         = __dram_sw_end);
PROVIDE (__dram_cma_end           = 0x40000000 + AIC_DRAM_TOTAL_SIZE);
PROVIDE (__dram_start             = 0x40000000);
PROVIDE (__dram_end               = 0x40000000 + AIC_DRAM_TOTAL_SIZE);

PROVIDE (__dram_cma_heap_end      = __dram_cma_end);
PROVIDE (__dram_sw_heap_end       = __dram_sw_end);

PROVIDE (__min_heap_size           = 0x200);
PROVIDE (__heap_start              = __dram_sw_heap_start);
PROVIDE (__heap_end                = __dram_sw_heap_end);
PROVIDE (__cma_heap_start          = __dram_cma_heap_start);
PROVIDE (__cma_heap_end            = __dram_cma_heap_end);

REGION_ALIAS("REGION_TEXT"          , DRAM_SW);
REGION_ALIAS("REGION_RODATA"        , DRAM_SW);
REGION_ALIAS("REGION_DATA"          , DRAM_SW);
REGION_ALIAS("REGION_BSS"           , DRAM_SW);
REGION_ALIAS("REGION_DRAM_SW"       , DRAM_SW);
REGION_ALIAS("REGION_DRAM_CMA"      , DRAM_CMA);

ENTRY(Reset_Handler)
SECTIONS
{
 .text : AT(ADDR(.text)){
  . = ALIGN(0x8) ;
  __stext = . ;
  KEEP(*startup_gcc.o(*.text*))
  KEEP(*startup_gcc.o(*.vectors*))
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
  . = ALIGN (8) ;
  PROVIDE(__ctbp = .);
  *(.call_table_data)
  *(.call_table_text)

  /* section information for tiny console shell */
  . = ALIGN(8) ;
  __console_init_start = .;
  KEEP(*(.tinyspl.console.cmd))
  . = ALIGN(8) ;
  __console_init_end = .;

  /* section information for finsh shell */
  . = ALIGN(8);
  __fsymtab_start = .;
  KEEP(*(FSymTab))
  __fsymtab_end = .;
  . = ALIGN(8);
  __vsymtab_start = .;
  KEEP(*(VSymTab))
  __vsymtab_end = .;
  . = ALIGN(8);
  
  /* section information for initial. */
  . = ALIGN(8);
  __rt_init_start = .;
  KEEP(*(SORT(.rti_fn*)))
  __rt_init_end = .;
  . = ALIGN(8);
  . = ALIGN(0x10) ;

#ifdef RT_USING_MODULE
  /* section information for modules */
  . = ALIGN(8);
  __rtmsymtab_start = .;
  KEEP(*(RTMSymTab))
  __rtmsymtab_end = .;
#endif

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
  . = ALIGN(0x4) ;
    PROVIDE(__ctors_start__ = .);
  KEEP (*(SORT(.init_array.*)))
  KEEP (*(.init_array))
  PROVIDE(__ctors_end__ = .);
  PROVIDE(__dtors_start__ = .);
  KEEP(*(SORT(.dtors.*)))
  KEEP(*(.dtors))
  PROVIDE(__dtors_end__ = .);

  /* usb host class */
  . = ALIGN(0x8) ;
  __usbh_class_info_start__ = .;
  KEEP(*(.usbh_class_info))
  __usbh_class_info_end__ = .;

  . = ALIGN(0x8) ;
  __erodata = .;
  __rodata_end__ = .;
 } > REGION_RODATA

 .data : {
  . = ALIGN(0x8) ;
  __data_start__ = . ;
  __sdata = . ;
  data_start = . ;
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
  __start_init_call = .;
  *(.initcall.init)
  __stop_init_call = .;
  __start_cmd = .;
  *(.bootloaddata.cmd)
  . = ALIGN(8) ;
  __stop_cmd = .;
  __global_pointer$ = .;
  *(.sdata)
  *(.sdata.*)
  *(.gnu.linkonce.s.*)
  *(__libc_atexit)
  *(__libc_subinit)
  *(__libc_subfreeres)
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

 .dram_cma : {
  . = ALIGN(0x8) ;
  __dram_cma_data_start = .;
  *(.dram_cma_data)
  . = ALIGN(0x8) ;
  __dram_cma_data_end = .;
  __dram_cma_heap_start = .;
 } > REGION_DRAM_CMA AT > REGION_DRAM_CMA
}
