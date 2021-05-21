// Copyright (c) 2021 XMOS LIMITED. This Software is subject to the terms of the
// XMOS Public License: Version 1

#ifndef APP_CONF_H_
#define APP_CONF_H_

#define appconfUSB_AUDIO_PORT      0
#define appconfDEVICE_CONTROL_PORT 4
#define appconfGPIO_T0_RPC_PORT    5
#define appconfGPIO_T1_RPC_PORT    6

#ifndef appconfI2S_ENABLED
#define appconfI2S_ENABLED         1
#endif

#ifndef appconfUSB_ENABLED
#define appconfUSB_ENABLED         1
#endif

#define appconfI2S_MODE_MASTER     0
#define appconfI2S_MODE_SLAVE      1
#ifndef appconfI2S_MODE
#define appconfI2S_MODE            appconfI2S_MODE_MASTER
#endif

#define appconfAEC_REF_USB         0
#define appconfAEC_REF_I2S         1
#ifndef appconfAEC_REF_DEFAULT
#define appconfAEC_REF_DEFAULT     appconfAEC_REF_USB
#endif

#define appconfUSB_AUDIO_RELEASE   0
#define appconfUSB_AUDIO_TESTING   1
#ifndef appconfUSB_AUDIO_MODE
#define appconfUSB_AUDIO_MODE      appconfUSB_AUDIO_TESTING
#endif


/* WW Config */
#define appconfWW_FRAMES_PER_INFERENCE          (160)

/* I/O and interrupt cores for Tile 0 */
#define appconfXUD_IO_CORE                      1 /* Must be kept off core 0 with the RTOS tick ISR */
#define appconfI2C_IO_CORE                      2 /* Must be kept off core 0 with the RTOS tick ISR */
#define appconfUSB_INTERRUPT_CORE               3 /* Must be kept off I/O cores. Best kept off core 0 with the tick ISR. */

/* I/O and interrupt cores for Tile 1 */
#define appconfPDM_MIC_IO_CORE                  1 /* Must be kept off core 0 with the RTOS tick ISR */
#define appconfI2S_IO_CORE                      2 /* Must be kept off core 0 with the RTOS tick ISR */
#define appconfPDM_MIC_INTERRUPT_CORE           3 /* Must be kept off I/O cores. Best kept off core 0 with the tick ISR. */
#define appconfI2S_INTERRUPT_CORE               4 /* Must be kept off I/O cores. Best kept off core 0 with the tick ISR. */


/* Task Priorities */
#define appconfSTARTUP_TASK_PRIORITY            (configMAX_PRIORITIES-1)
#define appconfWW_TASK_PRIORITY                 (configMAX_PRIORITIES-2)
#define appconfUSB_MGR_TASK_PRIORITY            (configMAX_PRIORITIES-1)
#define appconfUSB_AUDIO_TASK_PRIORITY          (configMAX_PRIORITIES-1)
#define appconfGPIO_RPC_HOST_PRIORITY           (configMAX_PRIORITIES-1)
#define appconfGPIO_TASK_PRIORITY               (configMAX_PRIORITIES-1)
#define appconfQSPI_FLASH_TASK_PRIORITY         (configMAX_PRIORITIES-1)
#define appconfDEVICE_CONTROL_TASK_PRIORITY     (1)

#endif /* APP_CONF_H_ */
