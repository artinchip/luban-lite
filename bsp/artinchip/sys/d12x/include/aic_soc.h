/*
 * Copyright (c) 2022, Artinchip Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef _AIC_SOC_H_
#define _AIC_SOC_H_

#ifdef __cplusplus
extern "C" {
#endif

#ifndef IHS_VALUE
#define  IHS_VALUE                  (20000000)
#endif

#ifndef EHS_VALUE
#define  EHS_VALUE                  (20000000)
#endif

/* frequence */

#define CLOCK_120M                  120000000
#define CLOCK_100M                  100000000
#define CLOCK_72M                   72000000
#define CLOCK_60M                   60000000
#define CLOCK_50M                   50000000
#define CLOCK_36M                   36000000
#define CLOCK_30M                   30000000
#define CLOCK_AUDIO                 24576000
#define CLOCK_24M                   24000000
#define CLOCK_12M                   12000000
#define CLOCK_4M                    4000000
#define CLOCK_1M                    1000000
#define CLOCK_32K                   32768

#ifndef __ASSEMBLY__
/* -------------------------  Interrupt Number Definition  ------------------------ */
typedef enum IRQn {
    NMI_EXPn                        = -2,  /* NMI Exception */
    Supervisor_Software_IRQn        = 1U,
    Machine_Software_IRQn           = 3U,  /* Machine software interrupt */
    User_Timer_IRQn                 = 4,   /* User timer interrupt */
    Supervisor_Timer_IRQn           = 5U,  /* Supervisor timer interrupt */
    CORET_IRQn                      = 7U,  /* core Timer Interrupt */
    Supervisor_External_IRQn        = 9U,
    Machine_External_IRQn           = 11U, /* Machine external interrupt */

    DCE_IRQn                        = 31U,
    DMA_IRQn                        = 32U,
    QSPI0_IRQn                      = 44U,
    QSPI1_IRQn                      = 45U,
    SDMC0_IRQn                      = 46U,
    SDMC1_IRQn                      = 47U,
    XSPI_IRQn                       = 49U,
    SPI_ENC_IRQn                    = 41U,
    MTOP_IRQn                       = 51U,
    AUDIO_IRQn                      = 54U,
    GPIO_IRQn                       = 68U, /* 68~75 */
#ifdef QEMU_RUN
    UART0_IRQn                      = 16U,
#else
    UART0_IRQn                      = 76U,
#endif
    UART1_IRQn                      = 77U,
    UART2_IRQn                      = 78U,
    UART3_IRQn                      = 79U,
    LCD_IRQn                        = 55U,
    MDI_IRQn                        = 58U,
    DE_IRQn                         = 59U,
    GE_IRQn                         = 60U,
    VE_IRQn                         = 61U,
    WDT_IRQn                        = 64U,
    I2C0_IRQn                       = 84U,
    I2C1_IRQn                       = 85U,
    CAN0_IRQn                       = 88U,
    CAN1_IRQn                       = 89U,
    PWM_IRQn                        = 90U,
    GPAI_IRQn                       = 92U,
    RTP_IRQn                        = 93U,
    TSEN_IRQn                       = 94U,
    CIR_IRQn                        = 95U,

    MAX_IRQn,
}
IRQn_Type;

#define UART_IRQn(id)    (UART0_IRQn + id)
#endif

/* ================================================================================ */
/* ================       Device Specific Peripheral Section       ================ */
/* ================================================================================ */


/* ================================================================================ */
/* ================              Peripheral memory map             ================ */
/* ================================================================================ */
#ifdef QEMU_RUN
/*
 qemu-system-riscv32 -cpu e906 -machine smartl -kernel *.elf -nographic -s -S -monitor stdio
 QEMU 4.1.0 monitor - type 'help' for more information
 (qemu) info mtree
 address-space: memory
   0000000000000000-ffffffffffffffff (prio 0, i/o): system
     0000000000000000-0000000000ffffff (prio 0, ram): smartl.sdram0
     0000000010002000-0000000010002fff (prio 0, i/o): csky_exit
     0000000020000000-0000000020ffffff (prio 0, ram): smartl.sdram1
     0000000040011000-0000000040011fff (prio 0, i/o): csky_timer
     0000000040015000-0000000040015fff (prio 0, i/o): csky_uart
     0000000050000000-0000000050ffffff (prio 0, ram): smartl.sdram2
     0000000060000000-0000000060ffffff (prio 0, ram): smartl.sdram3
     00000000e0000000-00000000e0ffffff (prio 0, i/o): csky_clic
     00000000effff000-00000000effff03f (prio 0, ram): smartl.systemmap
 */

#define E907_CORET_BASE             0xE0004000UL
#define E907_CLIC_BASE              0xE0800000UL
#define UART0_BASE                  0x40015000UL
#define UART_BASE(id)               (UART0_BASE + (id) * 0x1000UL)
#define DRAM_BASE                   0x00000000UL

#define QEMU_IO_BASE                0x60000000UL
#define BROM_BASE                   QEMU_IO_BASE
#define SRAM_BASE                   QEMU_IO_BASE
#define DMA_BASE                    QEMU_IO_BASE
#define DCE_BASE                    QEMU_IO_BASE
#define XSPI_BASE                   QEMU_IO_BASE
#define QSPI0_BASE                  QEMU_IO_BASE
#define QSPI1_BASE                  QEMU_IO_BASE
#define QSPI2_BASE                  QEMU_IO_BASE
#define QSPI3_BASE                  QEMU_IO_BASE
#define SDMC0_BASE                  QEMU_IO_BASE
#define SDMC1_BASE                  QEMU_IO_BASE
#define AHBCFG_BASE                 QEMU_IO_BASE
#define SYSCFG_BASE                 QEMU_IO_BASE
#define CMU_BASE                    QEMU_IO_BASE
#define SPI_ENC_BASE                QEMU_IO_BASE
#define AXICFG_BASE                 QEMU_IO_BASE
#define MTOP_BASE                   QEMU_IO_BASE
#define AUDIO_BASE                  QEMU_IO_BASE
#define GPIO_BASE                   QEMU_IO_BASE
#define UART1_BASE                  QEMU_IO_BASE
#define UART2_BASE                  QEMU_IO_BASE
#define UART3_BASE                  QEMU_IO_BASE
#define LCD_BASE                    QEMU_IO_BASE
#define MDI_BASE                    QEMU_IO_BASE
#define DE_BASE                     QEMU_IO_BASE
#define GE_BASE                     QEMU_IO_BASE
#define VE_BASE                     QEMU_IO_BASE
#define WDT_BASE                    QEMU_IO_BASE
#define WRI_BASE                    QEMU_IO_BASE
#define SID_BASE                    QEMU_IO_BASE
#define GTC_BASE                    QEMU_IO_BASE
#define I2C0_BASE                   QEMU_IO_BASE
#define I2C1_BASE                   QEMU_IO_BASE
#define CAN0_BASE                   QEMU_IO_BASE
#define CAN1_BASE                   QEMU_IO_BASE
#define PWM_BASE                    QEMU_IO_BASE
#define ADCIM_BASE                  QEMU_IO_BASE
#define GPAI_BASE                   QEMU_IO_BASE
#define RTP_BASE                    QEMU_IO_BASE
#define TSEN_BASE                   QEMU_IO_BASE
#define CIR_BASE                    QEMU_IO_BASE
#define FLASH_XIP_BASE              QEMU_IO_BASE
#else
#define BROM_BASE                   0x30000000UL /* - 0x30007FFF, 512KB	*/
#define SRAM_BASE                   0x30040000UL /* - 0x3004FFFF, 1MB	*/
#define DMA_BASE                    0x10000000UL /* - 0x1000FFFF, 64KB	,64KB	*/
#define DCE_BASE                    0x10010000UL /* - 0x10010FFF,  4KB	,4KB	*/
#define XSPI_BASE                   0x10300000UL /* - 0x10030FFF,  4KB	,4KB	*/
#define QSPI0_BASE                  0x10400000UL /* - 0x1040FFFF, 64KB	,--	*/
#define QSPI1_BASE                  0x10410000UL /* - 0x1041FFFF, 64KB	,256KB	*/
#define QSPI2_BASE                  0x10420000UL /* - 0x1042FFFF, 64KB	,256KB	*/
#define QSPI3_BASE                  0x10430000UL /* - 0x1043FFFF, 64KB	,256KB	*/
#define SDMC0_BASE                  0x10440000UL /* - 0x1044FFFF, 64KB	,--	*/
#define SDMC1_BASE                  0x10450000UL /* - 0x1045FFFF, 64KB	,--	*/
#define AHBCFG_BASE                 0x104FE000UL /* - 0x104FEFFF,  4KB	,4KB	*/
#define SYSCFG_BASE                 0x18000000UL /* - 0x18000FFF, 4KB	,64KB	*/
#define CMU_BASE                    0x18020000UL /* - 0x18020FFF, 4KB	,32KB	*/
#define SPI_ENC_BASE                0x18100000UL /* - 0x18100FFF, 4KB	,--	*/
#define AXICFG_BASE                 0x184FE000UL /* - 0x184FEFFF, 4KB	,--	*/
#define MTOP_BASE                   0x184FF000UL /* - 0x184FFFFF, 4KB	,--	*/
#define AUDIO_BASE                  0x18610000UL /* - 0x18610FFF, 4KB	,64KB	*/
#define GPIO_BASE                   0x18700000UL /* - 0x18700FFF, 4KB	,64KB	*/
#define UART0_BASE                  0x18710000UL /* - 0x18710FFF, 4KB	,--	*/
#define UART1_BASE                  0x18711000UL /* - 0x18711FFF, 4KB	,--	*/
#define UART2_BASE                  0x18712000UL /* - 0x18712FFF, 4KB	,--	*/
#define UART3_BASE                  0x18713000UL /* - 0x18713FFF, 4KB	,--	*/
#define UART_BASE(id)               (UART0_BASE + (id) * 0x1000UL)
#define LCD_BASE                    0x18800000UL /* - 0x18800FFF, 4KB	,64KB	*/
#define MDI_BASE                    0x18840000UL /* - 0x18840FFF, 4KB	,64KB	*/
#define DE_BASE                     0x18A00000UL /* - 0x18AFFFFF, 1MB	,1MB	*/
#define GE_BASE                     0x18B00000UL /* - 0x18BFFFFF, 1MB	,1MB	*/
#define VE_BASE                     0x18C00000UL /* - 0x18CFFFFF, 1MB	,1MB	*/
#define WDT_BASE                    0x19000000UL /* - 0x19000FFF, 4KB	,64KB	*/
#define WRI_BASE                    0x1900F000UL /* - 0x1900FFFF, 4KB	,64KB	*/
#define SID_BASE                    0x19010000UL /* - 0x19010FFF, 4KB	,64KB	*/
#define GTC_BASE                    0x19050000UL /* - 0x19051FFF, 8KB	,64KB	*/
#define I2C0_BASE                   0x19220000UL /* - 0x19220FFF, 4KB	,--	*/
#define I2C1_BASE                   0x19221000UL /* - 0x19221FFF, 4KB	,--	*/
#define CAN0_BASE                   0x19230000UL /* - 0x19230FFF, 4KB	,--	*/
#define CAN1_BASE                   0x19231000UL /* - 0x19231FFF, 4KB	,64KB	*/
#define PWM_BASE                    0x19240000UL /* - 0x19240FFF, 4KB	,64KB	*/
#define ADCIM_BASE                  0x19250000UL /* - 0x19250FFF, 4KB	,--	*/
#define GPAI_BASE                   0x19251000UL /* - 0x19251FFF, 4KB	,--	*/
#define RTP_BASE                    0x19252000UL /* - 0x19252FFF, 4KB	,--	*/
#define TSEN_BASE                   0x19253000UL /* - 0x19253FFF, 4KB	,--	*/
#define CIR_BASE                    0x19260000UL /* - 0x19260FFF, 4KB	,--	*/

#define FLASH_XIP_BASE              0x60000000UL
#endif

/* ================================================================================ */
/* ================             Peripheral declaration             ================ */
/* ================================================================================ */


#ifdef __cplusplus
}
#endif

#endif  /* _AIC_SOC_H_ */
