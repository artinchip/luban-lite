/*
 * Copyright (c) 2022-2023, ArtInChip Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Authors: dwj <weijie.ding@artinchip.com>
 */

#ifndef __HAL_AUDIOCODEC_REG_H__
#define __HAL_AUDIOCODEC_REG_H__

/* codec register definition */
#define RX_DMIC_IF_CTRL_REG					(0x00)
#define RX_DMIC_IF_ADOUT_SHIFT_EN				BIT(15)
#define RX_DMIC_IF_ADOUT_SHIFT_MASK				GENMASK(14, 12)
#define RX_DMIC_IF_DLT_LENGTH					(10)
#define RX_DMIC_IF_DMIC_RX_DLT_EN				(9)
#define RX_DMIC_IF_DEC2_FLT					BIT(7)
#define RX_DMIC_IF_DEC1_FLT					BIT(6)
#define RX_DMIC_IF_DEC_EN_MASK					GENMASK(7, 6)
#define RX_DMIC_IF_DEC_EN_ALL					(3 << 6)
#define RX_DMIC_IF_DATA_SWAP					(5)
#define RX_DMIC_IF_EN						BIT(4)
#define RX_DMIC_IF_FS_IN_MASK					GENMASK(3, 1)
#define RX_DMIC_IF_FS_IN(fs)					((fs) << 1)
#define RX_DMIC_IF_RX_CLK_MASK					BIT(0)
#define RX_DMIC_IF_RX_CLK_24576KHZ				(0)
#define RX_DMIC_IF_RX_CLK_22579KHZ				(1)

#define RX_HPF_1_2_CTRL_REG					(0x04)
#define RX_HPF2_EN						BIT(1)
#define RX_HPF1_EN						BIT(0)
#define RX_HPF_MASK                                             (3)

#define RX_HPF1_COEFF_REG					(0x08)
#define RX_HPF2_COEFF_REG					(0x0C)
#define RX_HPF1_GAIN_REG					(0x10)
#define RX_HPF2_GAIN_REG					(0x14)

#define RX_DVC_1_2_CTRL_REG					(0x18)
#define RX_DVC2_GAIN						(24)
#define RX_DVC1_GAIN						(16)
#define RX_DVC2_EN						BIT(1)
#define RX_DVC1_EN						BIT(0)
#define RX_DVC_MASK						(3)

#define TX_MIXER_CTRL_REG					(0x1C)
#define TX_MIXER0_EN						BIT(31)
#define TX_MIXER1_EN						BIT(30)
#define TX_MIXER1_ADCOUT_GAIN					BIT(28)
#define TX_MIXER1_DMICOUTR_GAIN					BIT(27)
#define TX_MIXER1_DMICOUTL_GAIN					BIT(26)
#define TX_MIXER1_AUDOUTR_GAIN					BIT(25)
#define TX_MIXER1_AUDOUTL_GAIN					BIT(24)
#define TX_MIXER0_ADCOUT_GAIN					BIT(20)
#define TX_MIXER0_DMICOUTR_GAIN					BIT(19)
#define TX_MIXER0_DMICOUTL_GAIN					BIT(18)
#define TX_MIXER0_AUDOUTR_GAIN					BIT(17)
#define TX_MIXER0_AUDOUTL_GAIN					BIT(16)
#define TX_MIXER1_ADCOUT_SEL					BIT(12)
#define TX_MIXER1_DMICOUTR_SEL					BIT(11)
#define TX_MIXER1_DMICOUTL_SEL					BIT(10)
#define TX_MIXER1_AUDOUTR_SEL					BIT(9)
#define TX_MIXER1_AUDOUTL_SEL					BIT(8)
#define TX_MIXER1_PATH_MASK					GENMASK(12, 8)
#define TX_MIXER0_ADCOUT_SEL					BIT(4)
#define TX_MIXER0_DMICOUTR_SEL					BIT(3)
#define TX_MIXER0_DMICOUTL_SEL					BIT(2)
#define TX_MIXER0_AUDOUTR_SEL					BIT(1)
#define TX_MIXER0_AUDOUTL_SEL					BIT(0)
#define TX_MIXER0_PATH_MASK					GENMASK(4, 0)

#define TX_DVC_3_4_CTRL_REG					(0x20)
#define TX_DVC4_GAIN						(24)
#define TX_DVC3_GAIN						(16)
#define TX_DVC4_EN						BIT(1)
#define TX_DVC3_EN						BIT(0)
#define TX_DVC4_MASK						TX_DVC4_EN
#define TX_DVC3_MASK						TX_DVC3_EN

#define TX_PLAYBACK_CTRL_REG					(0x24)
#define TX_DLT							(12)
#define TX_IF_CH1_EN						BIT(6)
#define TX_IF_CH0_EN						BIT(5)
#define TX_PLAYBACK_IF_EN					BIT(4)
#define TX_IF_CH1_MASK						TX_IF_CH1_EN
#define TX_IF_CH0_MASK						TX_IF_CH0_EN
#define TX_FS_OUT_MASK						GENMASK(3, 1)
#define TX_FS_OUT(fs)						((fs) << 1)
#define TX_CLK_MASK						BIT(0)
#define TX_CLK_24576KHZ						(0)
#define TX_CLK_22579KHZ						(1)

#define TX_SDM_CTRL_REG						(0x28)
#define TX_SDM_CH1_EN						BIT(1)
#define TX_SDM_CH0_EN						BIT(0)
#define TX_SDM_CH1_MASK						TX_SDM_CH1_EN
#define TX_SDM_CH0_MASK						TX_SDM_CH0_EN

#define TX_PWM_CTRL_REG                                         (0x2C)
#define TX_PWM1_DIFEN                                           BIT(5)
#define TX_PWM1_EN                                              BIT(4)
#define TX_PWM1_MASK                                            (3 << 4)
#define TX_PWM0_DIFEN                                           BIT(1)
#define TX_PWM0_EN                                              BIT(0)
#define TX_PWM0_MASK                                            (3)

#define DMIC_RXFIFO_CTRL_REG                                    (0x30)
#define DMIC_RXFIFO_FLUSH                                       BIT(31)
#define DMIC_RXFIFO_CH1_EN                                      BIT(1)
#define DMIC_RXFIFO_CH0_EN                                      BIT(0)
#define DMIC_RXFIFO_CH_MASK                                     (3)

#define TXFIFO_CTRL_REG                                         (0x34)
#define TXFIFO_FLUSH                                            BIT(31)
#define TXFIFO_CH1_EN                                           BIT(1)
#define TXFIFO_CH0_EN                                           BIT(0)
#define TXFIFO_CH_MASK                                          (3)

#define FIFO_INT_EN_REG                                         (0x38)
#define FIFO_AUDOUT_DRQ_EN                                      BIT(7)
#define FIFO_DMICIN_DRQ_EN                                      BIT(3)

#define FIFO_STA_REG                                            (0x3C)
#define TXFIFO_SPACE_SHIFT                                      (16)
#define TXFIFO_SPACE_MAX                                        (0x80)

#define DMIC_RXFIFO_DATA_REG                                    (0x40)
#define DMIC_RX_CNT_REG                                         (0x44)
#define TXFIFO_DATA_REG                                         (0x48)
#define TX_CNT_REG                                              (0x4C)

#define FADE_CTRL0_REG                                          (0x58)
#define FADE_CTRL0_CH1_EN                                       (2)
#define FADE_CTRL0_CH0_EN                                       (1)
#define FADE_CTRL0_EN                                           (0)
#define FADE_CTRL0_MASK                                         GENMASK(2, 0)
#define FADE_CTRL0_DISABLE                                      (0)

#define FADE_CTRL1_REG                                          (0x5C)
#define FADE_CTRL1_TARGET_VOL_MASK                              GENMASK(14, 0)
#define FADE_CTRL1_TARGET_VOL(vol)                              ((vol) << 0)

#define GLOBE_CTL_REG                                           (0x60)
#define GLOBE_GLB_RST                                           BIT(2)
#define GLOBE_TX_GLBEN                                          BIT(1)
#define GLOBE_RX_GLBEN                                          BIT(0)

#define ADC_IF_CTRL_REG						(0x70)
#define ADC_IF_CTRL_FILT_SEL					(16)
#define ADC_IF_CTRL_ADOUT_SHIFT_EN				BIT(15)
#define ADC_IF_CTRL_ADOUT_SHIFT					(12)
#define ADC_IF_CTRL_ADC_RX_DLT					(10)
#define ADC_IF_CTRL_ADC_RX_DLT_EN				(9)
#define ADC_IF_CTRL_EN_DEC0_MASK				BIT(6)
#define ADC_IF_CTRL_FS_ADC_IN_MASK				GENMASK(3, 1)
#define ADC_IF_CTRL_FS_ADC_IN(fs)				((fs) << 1)
#define ADC_IF_CTRL_RX_CLK_FRE_MASK				BIT(0)
#define ADC_IF_CTRL_RX_CLK_24576KHZ				(0)
#define ADC_IF_CTRL_RX_CLK_22579KHZ				(1)

#define ADC_HPF0_CTRL_REG					(0x74)
#define ADC_HPF0_CTRL_HPF0_EN					BIT(0)

#define ADC_DVC0_CTRL_REG					(0x80)
#define ADC_DVC0_CTRL_DVC0_GAIN					(16)
#define ADC_DVC0_CTRL_DVC0_EN					BIT(0)

#define ADC_RXFIFO_CTRL_REG					(0x84)
#define ADC_RXFIFO_FLUSH					BIT(31)
#define ADC_RXFIFO_RXTH						(8)
#define ADC_RXFIFO_EN						BIT(0)

#define ADC_RXFIFO_INT_EN_REG					(0x88)
#define ADC_RXFIFO_ADCIN_DRQ_EN					BIT(3)

#define ADC_RXFIFO_STA_REG					(0x8C)
#define ADC_RXFIFO_SPACE_CNT					(0)

#define ADC_RXFIFO_DATA_REG					(0x90)
#define ADC_RXFIFO_DATA_CNT_REG					(0x94)

#define ADC_CTL1_REG						(0xA0)
#define ADC_CTL1_MBIAS_EN					BIT(2)
#define ADC_CTL1_PGA_EN						BIT(1)
#define ADC_CTL1_ADC_EN						BIT(0)

#define ADC_CTL2_REG						(0xA4)
#define ADC_CTL2_MBIAS_CTL					(8)
#define ADC_CTL2_PGA_GAIN_SEL					(0)
#define ADC_CTL2_PGA_GAIN_MASK					GENMASK(3, 0)

#define DEFAULT_AUDIO_FREQ					(24576000)

#endif
