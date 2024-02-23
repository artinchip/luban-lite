/*
 * Copyright (c) 2022, Artinchip Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/*
 ******************************************************************************
                d12x Memory Layout
 ******************************************************************************

            SRAM 32K
 0x30040000+----------+
           |          |
           |   VE     |
           |          |
           |          |
           +----------+
           |   SW     |
           |          |
 0x30047FFF+----------+

           PSRAM 4M/8M
 0x40000000+----------+
           |          |
           | DE/GE/VE |
           |          |
           +----------+
           |   SW     |
 0x403FFFFF+----------+
/0x407FFFFF|          |
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

#ifdef CONFIG_ENABLE_ROM_API
INCLUDE rom_api.lds
#endif

MEMORY
{
    BROM      : ORIGIN = 0x30000000                                            , LENGTH = 32K
    SRAM_SW   : ORIGIN = 0x30040000                                            , LENGTH = AIC_SRAM_SW_SIZE
    SRAM_CMA  : ORIGIN = 0x30040000 + AIC_SRAM_SW_SIZE                         , LENGTH = AIC_SRAM_SIZE - AIC_SRAM_SW_SIZE
    PSRAM_SW  : ORIGIN = 0x40000000                                            , LENGTH = AIC_PSRAM_SW_SIZE
    PSRAM_CMA : ORIGIN = 0x40000000 + AIC_PSRAM_SW_SIZE                        , LENGTH = AIC_PSRAM_SIZE - AIC_PSRAM_SW_SIZE
#ifdef FPGA_BOARD_ARTINCHIP
    PSRAM_ext : ORIGIN = 0x40000000 + AIC_PSRAM_SIZE                           , LENGTH = 64M - AIC_PSRAM_SIZE
#endif
#ifdef AIC_XIP
    FLASH_XIP : ORIGIN = 0x60000000 + AIC_XIP_FW_OFFSET                        , LENGTH = 512M - AIC_XIP_FW_OFFSET
#endif
}

PROVIDE (__sram_sw_start           = 0x30040000);
PROVIDE (__sram_sw_end             = 0x30040000 + AIC_SRAM_SW_SIZE);
PROVIDE (__sram_cma_start          = __sram_sw_end);
PROVIDE (__sram_cma_end            = 0x30040000 + AIC_SRAM_SIZE);
PROVIDE (__psram_sw_start          = 0x40000000);
PROVIDE (__psram_sw_end            = 0x40000000 + AIC_PSRAM_SW_SIZE);
PROVIDE (__psram_cma_start         = __psram_sw_end);
PROVIDE (__psram_cma_end           = 0x40000000 + AIC_PSRAM_SIZE);
PROVIDE (__psram_start             = 0x40000000);
PROVIDE (__psram_end               = 0x40000000 + AIC_PSRAM_SIZE);

PROVIDE (__sram_sw_heap_end        = __sram_sw_end);
PROVIDE (__sram_cma_heap_end       = __sram_cma_end);
PROVIDE (__psram_sw_heap_end       = __psram_sw_end);
PROVIDE (__psram_cma_heap_end      = __psram_cma_end);

PROVIDE (__min_heap_size           = 0x200);
PROVIDE (__heap_start              = __psram_sw_heap_start);
PROVIDE (__heap_end                = __psram_sw_heap_end);
PROVIDE (__cma_heap_start          = __psram_cma_heap_start);
PROVIDE (__cma_heap_end            = __psram_cma_heap_end);

REGION_ALIAS("REGION_SRAM_CMA"     , SRAM_CMA);
REGION_ALIAS("REGION_SRAM_SW"      , SRAM_SW);
REGION_ALIAS("REGION_PSRAM_CMA"    , PSRAM_CMA);
REGION_ALIAS("REGION_PSRAM_SW"     , PSRAM_SW);

#ifdef AIC_XIP
REGION_ALIAS("REGION_TEXT"         , FLASH_XIP);
REGION_ALIAS("REGION_RODATA"       , FLASH_XIP);
#else
REGION_ALIAS("REGION_TEXT"         , PSRAM_SW);
REGION_ALIAS("REGION_RODATA"       , PSRAM_SW);
#endif
REGION_ALIAS("REGION_DATA"         , PSRAM_SW);
REGION_ALIAS("REGION_BSS"          , PSRAM_SW);

#ifdef FPGA_BOARD_ARTINCHIP
PROVIDE (__psram_ext_start         = __psram_end);
PROVIDE (__psram_ext_end           = 0x44000000);
PROVIDE (__temp_ext_heap_end       = __psram_ext_end);
REGION_ALIAS("REGION_FPGA_EXT"     , PSRAM_ext);
#endif

ENTRY(Reset_Handler)
SECTIONS
{
 .sram_sw : {
  . = ALIGN(0x4) ;
  __sram_sw_data_start = .;
  *(.sram_sw_data)
  . = ALIGN(0x4) ;
  __sram_sw_data_end = .;
  __sram_sw_heap_start = .;
 } > REGION_SRAM_SW AT > REGION_SRAM_SW

 .sram_cma : {
  . = ALIGN(0x4) ;
  __sram_cma_data_start = .;
  *(.sram_cma_data)
  . = ALIGN(0x4) ;
  __sram_cma_data_end = .;
  __sram_cma_heap_start = .;
 } > REGION_SRAM_CMA AT > REGION_SRAM_CMA

 .text : AT(ADDR(.text)){
  . = ALIGN(0x4) ;
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
  . = ALIGN (4) ;
  PROVIDE(__ctbp = .);
  *(.call_table_data)
  *(.call_table_text)

  /* section information for tiny console shell */
  . = ALIGN(4) ;
  __console_init_start = .;
  KEEP(*(.tinyspl.console.cmd))
  . = ALIGN(4) ;
  __console_init_end = .;

  /* section information for finsh shell */
  . = ALIGN(4);
  __fsymtab_start = .;
  KEEP(*(FSymTab))
  __fsymtab_end = .;
  . = ALIGN(4);
  __vsymtab_start = .;
  KEEP(*(VSymTab))
  __vsymtab_end = .;
  . = ALIGN(4);

  /* section information for initial. */
  . = ALIGN(4);
  __rt_init_start = .;
  KEEP(*(SORT(.rti_fn*)))
  __rt_init_end = .;
  . = ALIGN(4);
  . = ALIGN(0x10) ;

#ifdef RT_USING_MODULE
  /* section information for modules */
  . = ALIGN(4);
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
  . = ALIGN(0x4) ;
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

#ifdef AIC_XIP
  . = ALIGN(0x40) ;
#else
  . = ALIGN(0x4) ;
#endif
  __erodata = .;
  __rodata_end__ = .;
 } > REGION_RODATA

#ifdef AIC_XIP
 .ram.code : {
  . = ALIGN(0x8) ;
  __ram_code_start__ = .;
  __sdata = . ;
  __data_start__ = . ;
  *(.ram.code*)
  __ram_code_end__ = .;
  . = ALIGN(0x8) ;
 } > REGION_DATA AT > REGION_DATA

 .data : {
  . = ALIGN(0x4) ;
#else
 .data : {
  . = ALIGN(0x4) ;
  __data_start__ = . ;
  __sdata = . ;
#endif
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
  __start_init_call = .;
  *(.initcall.init)
  __stop_init_call = .;
  __start_cmd = .;
  *(.bootloaddata.cmd)
  . = ALIGN(4) ;
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
  . = ALIGN(0x4) ;

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
  . = ALIGN(0x4) ;
  __sbss = ALIGN(0x4) ;
  __bss_start__ = . ;
  *(.dynsbss)
  *(.sbss)
  *(.sbss.*)
  *(.scommon)
  *(.dynbss)
  *(.bss)
  *(.bss.*)
  *(COMMON)
  . = ALIGN(0x4) ;
  __ebss = . ;
  __end = . ;
  end = . ;
  __bss_end__ = .;
 } > REGION_BSS

 .psram_sw_heap : {
  . = ALIGN(0x4) ;
  __psram_sw_heap_start = .;
  . += __min_heap_size;
  . = ALIGN(0x4) ;
 } > REGION_PSRAM_SW AT > REGION_PSRAM_SW

 .psram_cma : {
  . = ALIGN(0x4) ;
  __psram_cma_data_start = .;
  *(.psram_cma_data)
  . = ALIGN(0x4) ;
  __psram_cma_data_end = .;
  __psram_cma_heap_start = .;
 } > REGION_PSRAM_CMA AT > REGION_PSRAM_CMA

 #ifdef FPGA_BOARD_ARTINCHIP
 .fpga_ext : {
  . = ALIGN(0x4) ;
  __fpga_ext_data_start = .;
  *(.fpga_ext_data)
  . = ALIGN(0x4) ;
  __fpga_ext_data_end = .;
  __fpga_ext_heap_start = .;
 } > REGION_FPGA_EXT AT > REGION_FPGA_EXT
 #endif
}
