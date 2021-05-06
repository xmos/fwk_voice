// Copyright (c) 2021 XMOS LIMITED. This Software is subject to the terms of the
// XMOS Public License: Version 1

#ifndef APP_CONF_H_
#define APP_CONF_H_

#define appconfGPIO_T0_RPC_PORT 5
#define appconfGPIO_T1_RPC_PORT 6

/* WW Config */
#define appconfWW_SAMPLES_PER_FRAME     (160)

/* Task Priorities */
#define appconfSTARTUP_TASK_PRIORITY            (configMAX_PRIORITIES-1)
#define appconfWW_TASK_PRIORITY                 (configMAX_PRIORITIES-2)
#define appconfUSB_MGR_TASK_PRIORITY            (configMAX_PRIORITIES-1)
#define appconfUSB_AUDIO_TASK_PRIORITY          (configMAX_PRIORITIES-1)
#define appconfGPIO_RPC_HOST_PRIORITY           (configMAX_PRIORITIES-1)
#define appconfGPIO_TASK_PRIORITY               (configMAX_PRIORITIES-1)
#define appconfQSPI_FLASH_TASK_PRIORITY         (configMAX_PRIORITIES-1)

#endif /* APP_CONF_H_ */
