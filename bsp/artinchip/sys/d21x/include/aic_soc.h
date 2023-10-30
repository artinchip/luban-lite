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

typedef enum {
    Supervisor_Software_IRQn        = 1U,
    Machine_Software_IRQn           = 3U,
    Supervisor_Timer_IRQn           = 5U,
    CORET_IRQn                      = 7U,
    Supervisor_External_IRQn        = 9U,
    Machine_External_IRQn           = 11U,

    DMA_IRQn                        = 32U,
    CE_IRQn                         = 33U,
    USB_DEV_IRQn                    = 34U,
    USB_HOST0_EHCI_IRQn             = 35U,
    USB_HOST0_OHCI_IRQn             = 36U,
    USB_HOST1_EHCI_IRQn             = 37U,
    USB_HOST1_OHCI_IRQn             = 38U,
    GMAC0_IRQn                      = 39U,
    GMAC1_IRQn                      = 40U,
    QSPI0_IRQn                      = 44U,
    QSPI1_IRQn                      = 45U,
    QSPI2_IRQn                      = 42U,
    QSPI3_IRQn                      = 43U,
    SDMC0_IRQn                      = 46U,
    SDMC1_IRQn                      = 47U,
    SDMC2_IRQn                      = 48U,
    SPI_ENC_IRQn                    = 41U,
    PWMCS_FAULT_IRQn                = 24U,
    PWMCS_PWM_IRQn                  = 25U,
    PWMCS_CAP_IRQn                  = 26U,
    PWMCS_QEP_IRQn                  = 27U,
    PSADC_IRQn                      = 28U,
    MTOP_IRQn                       = 51U,
    I2S0_IRQn                       = 52U,
    I2S1_IRQn                       = 53U,
    AUDIO_IRQn                      = 54U,
    GPIO_IRQn                       = 68U, /* 68~75 */
#ifdef QEMU_RUN
    UART0_IRQn                      = 32U,
#else
    UART0_IRQn                      = 76U,
#endif
    UART1_IRQn                      = 77U,
    UART2_IRQn                      = 78U,
    UART3_IRQn                      = 79U,
    UART4_IRQn                      = 80U,
    UART5_IRQn                      = 81U,
    UART6_IRQn                      = 82U,
    UART7_IRQn                      = 83U,
    LCD_IRQn                        = 55U,
    MIPI_DSI_IRQn                   = 56U,
    DVP_IRQn                        = 57U,
    MIPI_CSI_IRQn                   = 58U,
    DE_IRQn                         = 59U,
    GE_IRQn                         = 60U,
    VE_IRQn                         = 61U,
    WDT_IRQn                        = 64U,
    RTC_IRQn                        = 50U,
    I2C0_IRQn                       = 84U,
    I2C1_IRQn                       = 85U,
    I2C2_IRQn                       = 86U,
    I2C3_IRQn                       = 87U,
    CAN0_IRQn                       = 88U,
    CAN1_IRQn                       = 89U,
    PWM_IRQn                        = 90U,
    GPAI_IRQn                       = 92U,
    RTP_IRQn                        = 93U,
    TSEN_IRQn                       = 94U,
    CIR_IRQn                        = 95U,

    MAX_IRQn,
} IRQn_Type;

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
 qemu-system-riscv64 -cpu c906fd -machine smarth -nographic -s -S -monitor stdio
 (qemu) info mtree
 address-space: memory
   0000000000000000-ffffffffffffffff (prio 0, i/o): system
     0000000000000000-0000000007ffffff (prio 0, ram): smarth.sdram
     0000000010002000-0000000010002fff (prio 0, i/o): csky_exit
     0000000010011000-0000000010011fff (prio 0, i/o): csky_timer
     0000000010015000-0000000010015fff (prio 0, i/o): csky_uart
     0000004000000000-0000004003ffffff (prio 0, i/o): csky_plic
     0000004004000000-000000400400ffff (prio 0, i/o): csky_clint
 */

#define C906_PLIC_BASE              0x4000000000UL
#define C906_CINT_BASE              0x4004000000UL
#define DRAM_BASE                   0x00000000UL
#define UART0_BASE                  0x10015000UL
#define UART_BASE(id)               (UART0_BASE + (id) * 0x1000UL)

#define QEMU_IO_BASE                0x07000000UL
#define BROM_BASE                   QEMU_IO_BASE
#define SRAM_BASE                   QEMU_IO_BASE
#define DMA_BASE                    QEMU_IO_BASE
#define CE_BASE                     QEMU_IO_BASE
#define USB_DEV_BASE                QEMU_IO_BASE
#define USB_HOST0_BASE              QEMU_IO_BASE
#define USB_HOST1_BASE              QEMU_IO_BASE
#define GMAC0_BASE                  QEMU_IO_BASE
#define GMAC1_BASE                  QEMU_IO_BASE
#define QSPI0_BASE                  QEMU_IO_BASE
#define QSPI1_BASE                  QEMU_IO_BASE
#define QSPI2_BASE                  QEMU_IO_BASE
#define QSPI3_BASE                  QEMU_IO_BASE
#define SDMC0_BASE                  QEMU_IO_BASE
#define SDMC1_BASE                  QEMU_IO_BASE
#define SDMC2_BASE                  QEMU_IO_BASE
#define PBUS_BASE                   QEMU_IO_BASE
#define SYSCFG_BASE                 QEMU_IO_BASE
#define CMU_BASE                    QEMU_IO_BASE
#define CMT_BASE                    QEMU_IO_BASE
#define SPI_ENC_BASE                QEMU_IO_BASE
#define PWMCS_BASE                  QEMU_IO_BASE
#define PSADC_BASE                  QEMU_IO_BASE
#define DDR_CTL_BASE                QEMU_IO_BASE
#define DDR_MTOP_BASE               QEMU_IO_BASE
#define DDR_PHY_BASE                QEMU_IO_BASE
#define I2S0_BASE                   QEMU_IO_BASE
#define I2S1_BASE                   QEMU_IO_BASE
#define AUDIO_BASE                  QEMU_IO_BASE
#define GPIO_BASE                   QEMU_IO_BASE
#define PMT_BASE                    QEMU_IO_BASE
#define UART1_BASE                  QEMU_IO_BASE
#define UART2_BASE                  QEMU_IO_BASE
#define UART3_BASE                  QEMU_IO_BASE
#define UART4_BASE                  QEMU_IO_BASE
#define UART5_BASE                  QEMU_IO_BASE
#define UART6_BASE                  QEMU_IO_BASE
#define UART7_BASE                  QEMU_IO_BASE
#define LCD_BASE                    QEMU_IO_BASE
#define LVDS_BASE                   QEMU_IO_BASE
#define MIPI_DSI_BASE               QEMU_IO_BASE
#define DVP_BASE                    QEMU_IO_BASE
#define MIPI_CSI_BASE               QEMU_IO_BASE
#define DE_BASE                     QEMU_IO_BASE
#define GE_BASE                     QEMU_IO_BASE
#define VE_BASE                     QEMU_IO_BASE
#define WDT_BASE                    QEMU_IO_BASE
#define WRI_BASE                    QEMU_IO_BASE
#define SID_BASE                    QEMU_IO_BASE
#define RTC_BASE                    QEMU_IO_BASE
#define GTC_BASE                    QEMU_IO_BASE
#define I2C0_BASE                   QEMU_IO_BASE
#define I2C1_BASE                   QEMU_IO_BASE
#define I2C2_BASE                   QEMU_IO_BASE
#define I2C3_BASE                   QEMU_IO_BASE
#define CAN0_BASE                   QEMU_IO_BASE
#define CAN1_BASE                   QEMU_IO_BASE
#define PWM_BASE                    QEMU_IO_BASE
#define ADCIM_BASE                  QEMU_IO_BASE
#define GPAI_BASE                   QEMU_IO_BASE
#define RTP_BASE                    QEMU_IO_BASE
#define TSEN_BASE                   QEMU_IO_BASE
#define CIR_BASE                    QEMU_IO_BASE
#else
#define BROM_BASE                   0x00000000UL /* - 0x0000FFFF, 64KB	,1MB	*/
#define SRAM_BASE                   0x00100000UL /* - 0x00117FFF, 96KB	,1MB	*/
#define DMA_BASE                    0x10000000UL /* - 0x1000FFFF, 64KB	,64KB	*/
#define CE_BASE                     0x10020000UL /* - 0x1002FFFF, 64KB	,64KB	*/
#define USB_DEV_BASE                0x10200000UL /* - 0x1020FFFF, 64KB	,--	*/
#define USB_HOST0_BASE              0x10210000UL /* - 0x1021FFFF, 64KB	,--	*/
#define USB_HOST1_BASE              0x10220000UL /* - 0x1022FFFF, 64KB	,512KB	*/
#define GMAC0_BASE                  0x10280000UL /* - 0x1028FFFF, 64KB	,--	*/
#define GMAC1_BASE                  0x10290000UL /* - 0x1029FFFF, 64KB	,512KB	*/
#define QSPI0_BASE                  0x10400000UL /* - 0x1040FFFF, 64KB	,--	*/
#define QSPI1_BASE                  0x10410000UL /* - 0x1041FFFF, 64KB	,256KB	*/
#define QSPI2_BASE                  0x10420000UL /* - 0x1042FFFF, 64KB	,256KB	*/
#define QSPI3_BASE                  0x10430000UL /* - 0x1043FFFF, 64KB	,256KB	*/
#define SDMC0_BASE                  0x10440000UL /* - 0x1044FFFF, 64KB	,--	*/
#define SDMC1_BASE                  0x10450000UL /* - 0x1045FFFF, 64KB	,--	*/
#define SDMC2_BASE                  0x10460000UL /* - 0x1046FFFF, 64KB	,256KB	*/
#define PBUS_BASE                   0x107F0000UL /* - 0x1080FFFF, 128KB	,8MB	*/
#define SYSCFG_BASE                 0x18000000UL /* - 0x18000FFF, 4KB	,64KB	*/
#define CMU_BASE                    0x18020000UL /* - 0x18020FFF, 4KB	,32KB	*/
#define CMT_BASE                    0x18028000UL /* - 0x1802FFFF, 32KB	,--	*/
#define SPI_ENC_BASE                0x18100000UL /* - 0x18100FFF, 4KB	,--	*/
#define PWMCS_BASE                  0x18200000UL /* - 0x1820FFFF, 64KB	,--	*/
#define PSADC_BASE                  0x18210000UL /* - 0x18210FFF, 4KB	,--	*/
#define DDR_CTL_BASE                0x18400000UL /* - 0x1840FFFF, 64KB	,1MB	*/
#define MTOP_BASE                   0x184FF000UL /* - 0x184FFFFF, 4KB	,--	*/
#define DDR_PHY_BASE                0x18500000UL /* - 0x1850FFFF, 64KB	,1MB	*/
#define I2S0_BASE                   0x18600000UL /* - 0x18600FFF, 4KB	,--	*/
#define I2S1_BASE                   0x18601000UL /* - 0x18601FFF, 4KB	,64KB	*/
#define AUDIO_BASE                  0x18610000UL /* - 0x18610FFF, 4KB	,64KB	*/
#define GPIO_BASE                   0x18700000UL /* - 0x18700FFF, 4KB	,64KB	*/
#define PMT_BASE                    0x1870F000UL /* - 0x1870FFFF, 4KB	,--	*/
#define UART0_BASE                  0x18710000UL /* - 0x18710FFF, 4KB	,--	*/
#define UART1_BASE                  0x18711000UL /* - 0x18711FFF, 4KB	,--	*/
#define UART2_BASE                  0x18712000UL /* - 0x18712FFF, 4KB	,--	*/
#define UART3_BASE                  0x18713000UL /* - 0x18713FFF, 4KB	,--	*/
#define UART4_BASE                  0x18714000UL /* - 0x18714FFF, 4KB	,--	*/
#define UART5_BASE                  0x18715000UL /* - 0x18715FFF, 4KB	,--	*/
#define UART6_BASE                  0x18716000UL /* - 0x18716FFF, 4KB	,--	*/
#define UART7_BASE                  0x18717000UL /* - 0x18717FFF, 4KB	,64KB	*/
#define UART_BASE(id)               (UART0_BASE + (id) * 0x1000UL)
#define LCD_BASE                    0x18800000UL /* - 0x18800FFF, 4KB	,64KB	*/
#define LVDS_BASE                   0x18810000UL /* - 0x18810FFF, 4KB	,64KB	*/
#define MIPI_DSI_BASE               0x18820000UL /* - 0x18820FFF, 4KB	,64KB	*/
#define DVP_BASE                    0x18830000UL /* - 0x18830FFF, 4KB	,64KB	*/
#define MIPI_CSI_BASE               0x18840000UL /* - 0x18840FFF, 4KB	,64KB	*/
#define DE_BASE                     0x18A00000UL /* - 0x18AFFFFF, 1MB	,1MB	*/
#define GE_BASE                     0x18B00000UL /* - 0x18BFFFFF, 1MB	,1MB	*/
#define VE_BASE                     0x18C00000UL /* - 0x18CFFFFF, 1MB	,1MB	*/
#define WDT_BASE                    0x19000000UL /* - 0x19000FFF, 4KB	,64KB	*/
#define WRI_BASE                    0x1900F000UL /* - 0x1900FFFF, 4KB	,64KB	*/
#define SID_BASE                    0x19010000UL /* - 0x19010FFF, 4KB	,64KB	*/
#define RTC_BASE                    0x19030000UL /* - 0x19030FFF, 4KB	,64KB	*/
#define GTC_BASE                    0x19050000UL /* - 0x19051FFF, 8KB	,64KB	*/
#define I2C0_BASE                   0x19220000UL /* - 0x19220FFF, 4KB	,--	*/
#define I2C1_BASE                   0x19221000UL /* - 0x19221FFF, 4KB	,--	*/
#define I2C2_BASE                   0x19222000UL /* - 0x19222FFF, 4KB	,--	*/
#define I2C3_BASE                   0x19223000UL /* - 0x19223FFF, 4KB	,64KB	*/
#define CAN0_BASE                   0x19230000UL /* - 0x19230FFF, 4KB	,--	*/
#define CAN1_BASE                   0x19231000UL /* - 0x19231FFF, 4KB	,64KB	*/
#define PWM_BASE                    0x19240000UL /* - 0x19240FFF, 4KB	,64KB	*/
#define ADCIM_BASE                  0x19250000UL /* - 0x19250FFF, 4KB	,--	*/
#define GPAI_BASE                   0x19251000UL /* - 0x19251FFF, 4KB	,--	*/
#define RTP_BASE                    0x19252000UL /* - 0x19252FFF, 4KB	,--	*/
#define TSEN_BASE                   0x19253000UL /* - 0x19253FFF, 4KB	,--	*/
#define CIR_BASE                    0x19260000UL /* - 0x19260FFF, 4KB	,--	*/
#define C906_CORE_BASE              CPU_BASE
#define C906_PLIC_BASE              CPU_BASE
#define C906_CINT_BASE              (CPU_BASE + 0x4000000UL)
#define DRAM_BASE                   0x40000000UL /* - 0x7FFFFFFF, 1GB	,1GB	*/
#endif

/* ================================================================================ */
/* ================             Peripheral declaration             ================ */
/* ================================================================================ */


#ifdef __cplusplus
}
#endif

#endif  /* _AIC_SOC_H_ */
