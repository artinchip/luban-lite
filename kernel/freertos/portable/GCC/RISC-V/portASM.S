/*
 * FreeRTOS Kernel V10.4.3 LTS Patch 3
 * Copyright (C) 2020 Amazon.com, Inc. or its affiliates.  All Rights Reserved.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy of
 * this software and associated documentation files (the "Software"), to deal in
 * the Software without restriction, including without limitation the rights to
 * use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of
 * the Software, and to permit persons to whom the Software is furnished to do so,
 * subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS
 * FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR
 * COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER
 * IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 * https://www.FreeRTOS.org
 * https://github.com/FreeRTOS
 *
 * 1 tab == 4 spaces!
 */

/*
 * The FreeRTOS kernel's RISC-V port is split between the the code that is
 * common across all currently supported RISC-V chips (implementations of the
 * RISC-V ISA), and code which tailors the port to a specific RISC-V chip:
 *
 * + The code that is common to all RISC-V chips is implemented in
 *   FreeRTOS\Source\portable\GCC\RISC-V-RV32\portASM.S.  There is only one
 *   portASM.S file because the same file is used no matter which RISC-V chip is
 *   in use.
 *
 * + The code that tailors the kernel's RISC-V port to a specific RISC-V
 *   chip is implemented in freertos_risc_v_chip_specific_extensions.h.  There
 *   is one freertos_risc_v_chip_specific_extensions.h that can be used with any
 *   RISC-V chip that both includes a standard CLINT and does not add to the
 *   base set of RISC-V registers.  There are additional
 *   freertos_risc_v_chip_specific_extensions.h files for RISC-V implementations
 *   that do not include a standard CLINT or do add to the base set of RISC-V
 *   registers.
 *
 * CARE MUST BE TAKEN TO INCLDUE THE CORRECT
 * freertos_risc_v_chip_specific_extensions.h HEADER FILE FOR THE CHIP
 * IN USE.  To include the correct freertos_risc_v_chip_specific_extensions.h
 * header file ensure the path to the correct header file is in the assembler's
 * include path.
 *
 * This freertos_risc_v_chip_specific_extensions.h is for use on RISC-V chips
 * that include a standard CLINT and do not add to the base set of RISC-V
 * registers.
 *
 */

#include "cpuport.h"

#if __riscv_xlen == 64
	#define portWORD_SIZE 8
	#define store_x sd
	#define load_x ld
#elif __riscv_xlen == 32
	#define store_x sw
	#define load_x lw
	#define portWORD_SIZE 4
#else
	#error Assembler did not define __riscv_xlen
#endif

#if defined(__riscv_d)
	#define portFWORD_SIZE 8
	#define store_f fsd
	#define load_f fld
#elif defined(__riscv_f)
	#define store_f fsw
	#define load_f flw
	#define portFWORD_SIZE 4
#endif

#include "freertos_risc_v_chip_specific_extensions.h"

/* Check the freertos_risc_v_chip_specific_extensions.h and/or command line
definitions. */
#if defined( portasmHAS_CLINT ) && defined( portasmHAS_MTIME )
	#error The portasmHAS_CLINT constant has been deprecated.  Please replace it with portasmHAS_MTIME.  portasmHAS_CLINT and portasmHAS_MTIME cannot both be defined at once.  See https://www.FreeRTOS.org/Using-FreeRTOS-on-RISC-V.html
#endif

#ifdef portasmHAS_CLINT
	#warning The portasmHAS_CLINT constant has been deprecated.  Please replace it with portasmHAS_MTIME and portasmHAS_SIFIVE_CLINT.  For now portasmHAS_MTIME and portasmHAS_SIFIVE_CLINT are derived from portasmHAS_CLINT.  See https://www.FreeRTOS.org/Using-FreeRTOS-on-RISC-V.html
	#define portasmHAS_MTIME portasmHAS_CLINT
	#define portasmHAS_SIFIVE_CLINT portasmHAS_CLINT
#endif

#ifndef portasmHAS_MTIME
	#error freertos_risc_v_chip_specific_extensions.h must define portasmHAS_MTIME to either 1 (MTIME clock present) or 0 (MTIME clock not present).  See https://www.FreeRTOS.org/Using-FreeRTOS-on-RISC-V.html
#endif

#ifndef portasmHAS_SIFIVE_CLINT
	#define portasmHAS_SIFIVE_CLINT 0
#endif

/* Only the standard core registers are stored by default.  Any additional
registers must be saved by the portasmSAVE_ADDITIONAL_REGISTERS and
portasmRESTORE_ADDITIONAL_REGISTERS macros - which can be defined in a chip
specific version of freertos_risc_v_chip_specific_extensions.h.  See the notes
at the top of this file. */
#if defined(__riscv_p)
#define portCONTEXT_SIZE ( 31 * portWORD_SIZE )
#else
#define portCONTEXT_SIZE ( 30 * portWORD_SIZE )
#endif
#if defined(__riscv_d) || defined(__riscv_f)
#define FLOAT_REG_NUM 32
#define portFCONTEXT_SIZE ( FLOAT_REG_NUM * portFWORD_SIZE )
#endif

.global xPortStartFirstTask
.global PendSV_Handler
.global freertos_risc_v_trap_handler
.global pxPortInitialiseStack
.extern pxCurrentTCB
.extern ulPortTrapHandler
.extern vTaskSwitchContext
.extern xTaskIncrementTick
.extern Timer_IRQHandler
.extern pullMachineTimerCompareRegister
.extern pullNextTime
.extern uxTimerIncrementsForOneTick /* size_t type so 32-bit on 32-bit core and 64-bits on 64-bit core. */
.extern xISRStackTop
.extern portasmHANDLE_INTERRUPT

/*-----------------------------------------------------------*/

/*
 * Functions: vPortYield
 */
.global vPortYield
.type   vPortYield, %function
vPortYield:
	li      t0, TSPEND_ADDR
	lb      t1, (t0)
	li      t2, 0x01
	or      t1, t1, t2
	sb      t1, (t0)

	/* make sure wite instruction is complete */
	fence
	lb      t1, (t0)
	fence
#ifdef __riscv_xthead
	sync.i
#else
	.long   0x01a0000b
#endif

	ret

.align 8
.func
PendSV_Handler:
freertos_risc_v_trap_handler:
#ifdef CONFIG_THEAD_EXT_SPSWAPEN
	csrrw   sp, mscratch, sp
#endif

	// portasmSAVE_ADDITIONAL_REGISTERS	/* Defined in freertos_risc_v_chip_specific_extensions.h to save any registers unique to the RISC-V implementation. */
#if defined(__riscv_d) || defined(__riscv_f)
	addi sp, sp, -portFCONTEXT_SIZE
	store_f f0, 0 * portFWORD_SIZE( sp )
	store_f f1, 1 * portFWORD_SIZE( sp )
	store_f f2, 2 * portFWORD_SIZE( sp )
	store_f f3, 3 * portFWORD_SIZE( sp )
	store_f f4, 4 * portFWORD_SIZE( sp )
	store_f f5, 5 * portFWORD_SIZE( sp )
	store_f f6, 6 * portFWORD_SIZE( sp )
	store_f f7, 7 * portFWORD_SIZE( sp )
	store_f f8, 8 * portFWORD_SIZE( sp )
	store_f f9, 9 * portFWORD_SIZE( sp )
	store_f f10, 10 * portFWORD_SIZE( sp )
	store_f f11, 11 * portFWORD_SIZE( sp )
	store_f f12, 12 * portFWORD_SIZE( sp )
	store_f f13, 13 * portFWORD_SIZE( sp )
	store_f f14, 14 * portFWORD_SIZE( sp )
	store_f f15, 15 * portFWORD_SIZE( sp )
	store_f f16, 16 * portFWORD_SIZE( sp )
	store_f f17, 17 * portFWORD_SIZE( sp )
	store_f f18, 18 * portFWORD_SIZE( sp )
	store_f f19, 19 * portFWORD_SIZE( sp )
	store_f f20, 20 * portFWORD_SIZE( sp )
	store_f f21, 21 * portFWORD_SIZE( sp )
	store_f f22, 22 * portFWORD_SIZE( sp )
	store_f f23, 23 * portFWORD_SIZE( sp )
	store_f f24, 24 * portFWORD_SIZE( sp )
	store_f f25, 25 * portFWORD_SIZE( sp )
	store_f f26, 26 * portFWORD_SIZE( sp )
	store_f f27, 27 * portFWORD_SIZE( sp )
	store_f f28, 28 * portFWORD_SIZE( sp )
	store_f f29, 29 * portFWORD_SIZE( sp )
	store_f f30, 30 * portFWORD_SIZE( sp )
	store_f f31, 31 * portFWORD_SIZE( sp )
#endif

	addi sp, sp, -portCONTEXT_SIZE
	store_x x1, 1 * portWORD_SIZE( sp )
	store_x x5, 2 * portWORD_SIZE( sp )
	store_x x6, 3 * portWORD_SIZE( sp )
	store_x x7, 4 * portWORD_SIZE( sp )
	store_x x8, 5 * portWORD_SIZE( sp )
	store_x x9, 6 * portWORD_SIZE( sp )
	store_x x10, 7 * portWORD_SIZE( sp )
	store_x x11, 8 * portWORD_SIZE( sp )
	store_x x12, 9 * portWORD_SIZE( sp )
	store_x x13, 10 * portWORD_SIZE( sp )
	store_x x14, 11 * portWORD_SIZE( sp )
	store_x x15, 12 * portWORD_SIZE( sp )
	store_x x16, 13 * portWORD_SIZE( sp )
	store_x x17, 14 * portWORD_SIZE( sp )
	store_x x18, 15 * portWORD_SIZE( sp )
	store_x x19, 16 * portWORD_SIZE( sp )
	store_x x20, 17 * portWORD_SIZE( sp )
	store_x x21, 18 * portWORD_SIZE( sp )
	store_x x22, 19 * portWORD_SIZE( sp )
	store_x x23, 20 * portWORD_SIZE( sp )
	store_x x24, 21 * portWORD_SIZE( sp )
	store_x x25, 22 * portWORD_SIZE( sp )
	store_x x26, 23 * portWORD_SIZE( sp )
	store_x x27, 24 * portWORD_SIZE( sp )
	store_x x28, 25 * portWORD_SIZE( sp )
	store_x x29, 26 * portWORD_SIZE( sp )
	store_x x30, 27 * portWORD_SIZE( sp )
	store_x x31, 28 * portWORD_SIZE( sp )

	csrr t0, mstatus					/* Required for MPIE bit. */
	store_x t0, 29 * portWORD_SIZE( sp )

#if defined(__riscv_p)
	csrr t0, vxsat
	store_x t0, 30 * portWORD_SIZE( sp )
#endif

	csrr a1, mepc
	store_x a1, 0( sp )					/* Save updated exception return address. */

	load_x  t0, pxCurrentTCB			/* Load pxCurrentTCB. */
	store_x  sp, 0( t0 )				/* Write sp to first TCB member. */

	load_x sp, xISRStackTop				/* Switch to ISR stack before function call. */
	jal vTaskSwitchContext

#ifdef TSPEND_NEED_CLEAR
	/* clear software interrupt */
	li      t0, TSPEND_ADDR
	li      t1, 0x0
	sw      t1, (t0)
	/* make sure wite instruction is complete */
	fence
	lb      t1, (t0)
#endif

	load_x  t1, pxCurrentTCB			/* Load pxCurrentTCB. */
	load_x  sp, 0( t1 )				 	/* Read sp from first TCB member. */

	/* Load mret with the address of the next instruction in the task to run next. */
	load_x t0, 0( sp )
	csrw mepc, t0

#if defined(__riscv_p)
	load_x t0, 30( sp )
	csrw vxsat, t0
#endif

	/* Load mstatus with the interrupt enable bits used by the task. */
	load_x  t0, 29 * portWORD_SIZE( sp )
	csrw mstatus, t0						/* Required for MPIE bit. */

	load_x  x1, 1 * portWORD_SIZE( sp )
	load_x  x5, 2 * portWORD_SIZE( sp )		/* t0 */
	load_x  x6, 3 * portWORD_SIZE( sp )		/* t1 */
	load_x  x7, 4 * portWORD_SIZE( sp )		/* t2 */
	load_x  x8, 5 * portWORD_SIZE( sp )		/* s0/fp */
	load_x  x9, 6 * portWORD_SIZE( sp )		/* s1 */
	load_x  x10, 7 * portWORD_SIZE( sp )	/* a0 */
	load_x  x11, 8 * portWORD_SIZE( sp )	/* a1 */
	load_x  x12, 9 * portWORD_SIZE( sp )	/* a2 */
	load_x  x13, 10 * portWORD_SIZE( sp )	/* a3 */
	load_x  x14, 11 * portWORD_SIZE( sp )	/* a4 */
	load_x  x15, 12 * portWORD_SIZE( sp )	/* a5 */
	load_x  x16, 13 * portWORD_SIZE( sp )	/* a6 */
	load_x  x17, 14 * portWORD_SIZE( sp )	/* a7 */
	load_x  x18, 15 * portWORD_SIZE( sp )	/* s2 */
	load_x  x19, 16 * portWORD_SIZE( sp )	/* s3 */
	load_x  x20, 17 * portWORD_SIZE( sp )	/* s4 */
	load_x  x21, 18 * portWORD_SIZE( sp )	/* s5 */
	load_x  x22, 19 * portWORD_SIZE( sp )	/* s6 */
	load_x  x23, 20 * portWORD_SIZE( sp )	/* s7 */
	load_x  x24, 21 * portWORD_SIZE( sp )	/* s8 */
	load_x  x25, 22 * portWORD_SIZE( sp )	/* s9 */
	load_x  x26, 23 * portWORD_SIZE( sp )	/* s10 */
	load_x  x27, 24 * portWORD_SIZE( sp )	/* s11 */
	load_x  x28, 25 * portWORD_SIZE( sp )	/* t3 */
	load_x  x29, 26 * portWORD_SIZE( sp )	/* t4 */
	load_x  x30, 27 * portWORD_SIZE( sp )	/* t5 */
	load_x  x31, 28 * portWORD_SIZE( sp )	/* t6 */
	addi sp, sp, portCONTEXT_SIZE

	//portasmRESTORE_ADDITIONAL_REGISTERS	/* Defined in freertos_risc_v_chip_specific_extensions.h to restore any registers unique to the RISC-V implementation. */
#if defined(__riscv_d) || defined(__riscv_f)
	load_f f0, 0 * portFWORD_SIZE( sp )
	load_f f1, 1 * portFWORD_SIZE( sp )
	load_f f2, 2 * portFWORD_SIZE( sp )
	load_f f3, 3 * portFWORD_SIZE( sp )
	load_f f4, 4 * portFWORD_SIZE( sp )
	load_f f5, 5 * portFWORD_SIZE( sp )
	load_f f6, 6 * portFWORD_SIZE( sp )
	load_f f7, 7 * portFWORD_SIZE( sp )
	load_f f8, 8 * portFWORD_SIZE( sp )
	load_f f9, 9 * portFWORD_SIZE( sp )
	load_f f10, 10 * portFWORD_SIZE( sp )
	load_f f11, 11 * portFWORD_SIZE( sp )
	load_f f12, 12 * portFWORD_SIZE( sp )
	load_f f13, 13 * portFWORD_SIZE( sp )
	load_f f14, 14 * portFWORD_SIZE( sp )
	load_f f15, 15 * portFWORD_SIZE( sp )
	load_f f16, 16 * portFWORD_SIZE( sp )
	load_f f17, 17 * portFWORD_SIZE( sp )
	load_f f18, 18 * portFWORD_SIZE( sp )
	load_f f19, 19 * portFWORD_SIZE( sp )
	load_f f20, 20 * portFWORD_SIZE( sp )
	load_f f21, 21 * portFWORD_SIZE( sp )
	load_f f22, 22 * portFWORD_SIZE( sp )
	load_f f23, 23 * portFWORD_SIZE( sp )
	load_f f24, 24 * portFWORD_SIZE( sp )
	load_f f25, 25 * portFWORD_SIZE( sp )
	load_f f26, 26 * portFWORD_SIZE( sp )
	load_f f27, 27 * portFWORD_SIZE( sp )
	load_f f28, 28 * portFWORD_SIZE( sp )
	load_f f29, 29 * portFWORD_SIZE( sp )
	load_f f30, 30 * portFWORD_SIZE( sp )
	load_f f31, 31 * portFWORD_SIZE( sp )
	addi sp, sp, portFCONTEXT_SIZE
#endif

#ifdef CONFIG_THEAD_EXT_SPSWAPEN
	csrrw   sp, mscratch, sp
#endif
	mret
	.endfunc
/*-----------------------------------------------------------*/

.align 8
.func
xPortStartFirstTask:

	load_x  sp, pxCurrentTCB			/* Load pxCurrentTCB. */
	load_x  sp, 0( sp )				 	/* Read sp from first TCB member. */

	load_x  x1, 0( sp )					/* Note for starting the scheduler the exception return address is used as the function return address. */

#if defined(__riscv_p)
	load_x t0, 30( sp )
	csrw vxsat, t0
#endif

	load_x  x6, 3 * portWORD_SIZE( sp )		/* t1 */
	load_x  x7, 4 * portWORD_SIZE( sp )		/* t2 */
	load_x  x8, 5 * portWORD_SIZE( sp )		/* s0/fp */
	load_x  x9, 6 * portWORD_SIZE( sp )		/* s1 */
	load_x  x10, 7 * portWORD_SIZE( sp )	/* a0 */
	load_x  x11, 8 * portWORD_SIZE( sp )	/* a1 */
	load_x  x12, 9 * portWORD_SIZE( sp )	/* a2 */
	load_x  x13, 10 * portWORD_SIZE( sp )	/* a3 */
	load_x  x14, 11 * portWORD_SIZE( sp )	/* a4 */
	load_x  x15, 12 * portWORD_SIZE( sp )	/* a5 */
	load_x  x16, 13 * portWORD_SIZE( sp )	/* a6 */
	load_x  x17, 14 * portWORD_SIZE( sp )	/* a7 */
	load_x  x18, 15 * portWORD_SIZE( sp )	/* s2 */
	load_x  x19, 16 * portWORD_SIZE( sp )	/* s3 */
	load_x  x20, 17 * portWORD_SIZE( sp )	/* s4 */
	load_x  x21, 18 * portWORD_SIZE( sp )	/* s5 */
	load_x  x22, 19 * portWORD_SIZE( sp )	/* s6 */
	load_x  x23, 20 * portWORD_SIZE( sp )	/* s7 */
	load_x  x24, 21 * portWORD_SIZE( sp )	/* s8 */
	load_x  x25, 22 * portWORD_SIZE( sp )	/* s9 */
	load_x  x26, 23 * portWORD_SIZE( sp )	/* s10 */
	load_x  x27, 24 * portWORD_SIZE( sp )	/* s11 */
	load_x  x28, 25 * portWORD_SIZE( sp )	/* t3 */
	load_x  x29, 26 * portWORD_SIZE( sp )	/* t4 */
	load_x  x30, 27 * portWORD_SIZE( sp )	/* t5 */
	load_x  x31, 28 * portWORD_SIZE( sp )	/* t6 */

	load_x  x5, 29 * portWORD_SIZE( sp )	/* Initial mstatus into x5 (t0) */
	addi x5, x5, 0x08						/* Set MIE bit so the first task starts with interrupts enabled - required as returns with ret not eret. */
	csrrw  x0, mstatus, x5					/* Interrupts enabled from here! */
	load_x  x5, 2 * portWORD_SIZE( sp )		/* Initial x5 (t0) value. */

	addi	sp, sp, portCONTEXT_SIZE

	//portasmRESTORE_ADDITIONAL_REGISTERS	/* Defined in freertos_risc_v_chip_specific_extensions.h to restore any registers unique to the RISC-V implementation. */
#if defined(__riscv_d) || defined(__riscv_f)
	load_f f0, 0 * portFWORD_SIZE( sp )
	load_f f1, 1 * portFWORD_SIZE( sp )
	load_f f2, 2 * portFWORD_SIZE( sp )
	load_f f3, 3 * portFWORD_SIZE( sp )
	load_f f4, 4 * portFWORD_SIZE( sp )
	load_f f5, 5 * portFWORD_SIZE( sp )
	load_f f6, 6 * portFWORD_SIZE( sp )
	load_f f7, 7 * portFWORD_SIZE( sp )
	load_f f8, 8 * portFWORD_SIZE( sp )
	load_f f9, 9 * portFWORD_SIZE( sp )
	load_f f10, 10 * portFWORD_SIZE( sp )
	load_f f11, 11 * portFWORD_SIZE( sp )
	load_f f12, 12 * portFWORD_SIZE( sp )
	load_f f13, 13 * portFWORD_SIZE( sp )
	load_f f14, 14 * portFWORD_SIZE( sp )
	load_f f15, 15 * portFWORD_SIZE( sp )
	load_f f16, 16 * portFWORD_SIZE( sp )
	load_f f17, 17 * portFWORD_SIZE( sp )
	load_f f18, 18 * portFWORD_SIZE( sp )
	load_f f19, 19 * portFWORD_SIZE( sp )
	load_f f20, 20 * portFWORD_SIZE( sp )
	load_f f21, 21 * portFWORD_SIZE( sp )
	load_f f22, 22 * portFWORD_SIZE( sp )
	load_f f23, 23 * portFWORD_SIZE( sp )
	load_f f24, 24 * portFWORD_SIZE( sp )
	load_f f25, 25 * portFWORD_SIZE( sp )
	load_f f26, 26 * portFWORD_SIZE( sp )
	load_f f27, 27 * portFWORD_SIZE( sp )
	load_f f28, 28 * portFWORD_SIZE( sp )
	load_f f29, 29 * portFWORD_SIZE( sp )
	load_f f30, 30 * portFWORD_SIZE( sp )
	load_f f31, 31 * portFWORD_SIZE( sp )
	addi sp, sp, portFCONTEXT_SIZE
#endif
	ret
	.endfunc
/*-----------------------------------------------------------*/

/*
 * Unlike other ports pxPortInitialiseStack() is written in assembly code as it
 * needs access to the portasmADDITIONAL_CONTEXT_SIZE constant.  The prototype
 * for the function is as per the other ports:
 * StackType_t *pxPortInitialiseStack( StackType_t *pxTopOfStack, TaskFunction_t pxCode, void *pvParameters );
 *
 * As per the standard RISC-V ABI pxTopcOfStack is passed in in a0, pxCode in
 * a1, and pvParameters in a2.  The new top of stack is passed out in a0.
 *
 * RISC-V maps registers to ABI names as follows (X1 to X31 integer registers
 * for the 'I' profile, X1 to X15 for the 'E' profile, currently I assumed).
 *
 * Register		ABI Name	Description						Saver
 * x0			zero		Hard-wired zero					-
 * x1			ra			Return address					Caller
 * x2			sp			Stack pointer					Callee
 * x3			gp			Global pointer					-
 * x4			tp			Thread pointer					-
 * x5-7			t0-2		Temporaries						Caller
 * x8			s0/fp		Saved register/Frame pointer	Callee
 * x9			s1			Saved register					Callee
 * x10-11		a0-1		Function Arguments/return values Caller
 * x12-17		a2-7		Function arguments				Caller
 * x18-27		s2-11		Saved registers					Callee
 * x28-31		t3-6		Temporaries						Caller
 *
 * The RISC-V context is saved t FreeRTOS tasks in the following stack frame,
 * where the global and thread pointers are currently assumed to be constant so
 * are not saved:
 *
 * mstatus
 * x31
 * x30
 * x29
 * x28
 * x27
 * x26
 * x25
 * x24
 * x23
 * x22
 * x21
 * x20
 * x19
 * x18
 * x17
 * x16
 * x15
 * x14
 * x13
 * x12
 * x11
 * pvParameters
 * x9
 * x8
 * x7
 * x6
 * x5
 * portTASK_RETURN_ADDRESS
 * [chip specific registers go here]
 * pxCode
 */
.align 8
.func
pxPortInitialiseStack:
#if defined(__riscv_d) || defined(__riscv_f)
	addi t0, x0, FLOAT_REG_NUM			/* The number of float registers. */
chip_float_stack_frame:					/* First add any float registers to the stack frame being created. */
	beq t0, x0, 1f						/* No more float registers to save. */
	addi a0, a0, -portFWORD_SIZE		/* Make space for float register. */
	store_f f0, 0(a0)					/* Give the float register an initial value of zero. */
	addi t0, t0, -1						/* Decrement the count of float registers remaining. */
	j chip_float_stack_frame			/* Until no more float registers. */
1:
#endif

#if defined(__riscv_p)
	csrr t0, vxsat
	addi a0, a0, -portWORD_SIZE
	store_x t0, 0(a0)					/* vxsat onto the stack. */
#endif

	csrr t0, mstatus					/* Obtain current mstatus value. */
	andi t0, t0, ~0x8					/* Ensure interrupts are disabled when the stack is restored within an ISR.  Required when a task is created after the schedulre has been started, otherwise interrupts would be disabled anyway. */
	addi t1, x0, 0x188					/* Generate the value 0x1880, which are the MPIE and MPP bits to set in mstatus. */
	slli t1, t1, 4
	or t0, t0, t1						/* Set MPIE and MPP bits in mstatus value. */

	addi a0, a0, -portWORD_SIZE
	store_x t0, 0(a0)					/* mstatus onto the stack. */
	addi a0, a0, -(22 * portWORD_SIZE)	/* Space for registers x11-x31. */
	store_x a2, 0(a0)					/* Task parameters (pvParameters parameter) goes into register X10/a0 on the stack. */
	addi a0, a0, -(6 * portWORD_SIZE)	/* Space for registers x5-x9. */
	store_x x0, 0(a0)					/* Return address onto the stack, could be portTASK_RETURN_ADDRESS */

	addi a0, a0, -portWORD_SIZE
	store_x a1, 0(a0)					/* mret value (pxCode parameter) onto the stack. */
	ret
	.endfunc
/*-----------------------------------------------------------*/
