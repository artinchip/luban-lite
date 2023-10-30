/*
 * Copyright (c) 2022, Artinchip Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

MEMORY
{
    DRAM   : ORIGIN = 0x00000000, LENGTH = 0x04000000   /* DRAM 64M */
}

PROVIDE (__dram_start             = 0x00000000);
PROVIDE (__dram_end               = 0x04000000);
PROVIDE (__dram_heap_end          = __dram_end);
PROVIDE (__heap_start             = __dram_heap_start);
PROVIDE (__heap_end               = __dram_heap_end);
PROVIDE (__min_heap_size           = 0x200);

REGION_ALIAS("REGION_TEXT",    DRAM);
REGION_ALIAS("REGION_RODATA",  DRAM);
REGION_ALIAS("REGION_DATA",    DRAM);
REGION_ALIAS("REGION_BSS",     DRAM);

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
  . = ALIGN (8) ;
  PROVIDE(__ctbp = .);
  *(.call_table_data)
  *(.call_table_text)

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

 } > REGION_DATA
 .eh_frame : ONLY_IF_RW {
  KEEP (*(.eh_frame))
 } > REGION_DATA 
 .gcc_except_table : ONLY_IF_RW {
  *(.gcc_except_table .gcc_except_table.*)
  __edata = .;
  __data_end__ = .;
 } > REGION_DATA

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

 ._dram_heap : {
  . = ALIGN(0x8) ;
  __dram_heap_start = .;
  . += __min_heap_size;
  . = ALIGN(0x8) ;
 } > REGION_BSS

  /* End of uninitalized data segement */
  PROVIDE(end = .);
}
