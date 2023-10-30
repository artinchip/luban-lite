#ifndef _PM_CFG_H_
#define _PM_CFG_H_

/**
 * Modules used for
 * pm_module_request(PM_BOARD_ID, PM_SLEEP_MODE_IDLE)
 * pm_module_release(PM_BOARD_ID, PM_SLEEP_MODE_IDLE)
 * pm_module_release_all(PM_BOARD_ID, PM_SLEEP_MODE_IDLE)
 */
enum pm_module_id {
    PM_NONE_ID = 0,
    PM_POWER_ID,
    PM_BOARD_ID,
    PM_BSP_ID,
    PM_MAIN_ID,
    PM_PMS_ID,
    PM_PMC_ID,
    PM_TASK_ID,
    PM_SPI_ID,
    PM_I2C_ID,
    PM_UART_ID,
    PM_CAN_ID,
    PM_ETH_ID,
    PM_SENSOR_ID,
    PM_LCD_ID,
    PM_KEY_ID,
    PM_TP_ID,
    PM_PWM_ID,
    PM_AUDIO_ID,
    PM_I2S_ID,
    PM_DE_ID,
    PM_GE_ID,
    PM_VE_ID,
    PM_MODULE_MAX_ID, /* enum must! */
};

extern rt_timer_t touch_timer;
extern volatile uint8_t wakeup_triggered;

#endif