// Copyright (c) 2021 XMOS LIMITED. This Software is subject to the terms of the
// XMOS Public License: Version 1

#ifndef APP_CONTROL_H_
#define APP_CONTROL_H_

#include "device_control.h"

#include "app_conf.h"

#define APP_CONTROL_TRANSPORT_COUNT ((appconfI2C_CTRL_ENABLED ? 1 : 0) + (appconfUSB_ENABLED ? 1 : 0))

extern device_control_t *device_control_ctxs[APP_CONTROL_TRANSPORT_COUNT];
extern device_control_t *device_control_i2c_ctx;
extern device_control_t *device_control_usb_ctx;

int app_control_init(void);

#endif /* APP_CONTROL_H_ */
