/*
 * Copyright (c) 2022, ArtInChip Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

unsigned long g_aicos_irq_state = 0;
unsigned int g_aicos_irq_nested_cnt = 0;

void aicos_irq_enter(void)
{
    g_aicos_irq_nested_cnt++;
}

void aicos_irq_exit(void)
{
    g_aicos_irq_nested_cnt--;
}
