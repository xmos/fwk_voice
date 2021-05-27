// Copyright (c) 2021 XMOS LIMITED. This Software is subject to the terms of the
// XMOS Public License: Version 1

#include <platform.h>

/* Library headers */
#include "rtos_printf.h"
#include "app_control/usb_device_control.h"
#include "app_control/i2c_device_control.h"

/* App headers */
#include "app_conf.h"
#include "app_control.h"
#include "platform/driver_instances.h"

#if ON_TILE(USB_TILE_NO)
static device_control_t device_control_usb_ctx_s;
#else
static device_control_client_t device_control_usb_ctx_s;
#endif
device_control_t *device_control_usb_ctx = (device_control_t *) &device_control_usb_ctx_s;

#if ON_TILE(I2C_CTRL_TILE_NO)
static device_control_t device_control_i2c_ctx_s;
#else
static device_control_client_t device_control_i2c_ctx_s;
#endif
device_control_t *device_control_i2c_ctx = (device_control_t *) &device_control_i2c_ctx_s;

device_control_t *device_control_ctxs[APP_CONTROL_TRANSPORT_COUNT] = {
#if appconfUSB_ENABLED
        (device_control_t *) &device_control_usb_ctx_s,
#endif
#if appconfI2C_CTRL_ENABLED
        (device_control_t *) &device_control_i2c_ctx_s,
#endif
};

device_control_t *tud_device_control_get_ctrl_ctx_cb(void)
{
    return device_control_usb_ctx;
}

int app_control_init(void)
{
    control_ret_t ret = CONTROL_SUCCESS;

#if appconfI2C_CTRL_ENABLED
    if (ret == CONTROL_SUCCESS) {
        ret = device_control_init(device_control_i2c_ctx,
                                  THIS_XCORE_TILE == I2C_CTRL_TILE_NO ? DEVICE_CONTROL_HOST_MODE : DEVICE_CONTROL_CLIENT_MODE,
                                  appconf_CONTROL_SERVICER_COUNT,
                                  &intertile_ctx, 1);
    }
    if (ret == CONTROL_SUCCESS) {
        ret = device_control_start(device_control_i2c_ctx,
                                   appconfDEVICE_CONTROL_I2C_PORT,
                                   appconfDEVICE_CONTROL_I2C_CLIENT_PRIORITY);
    }
#endif

#if appconfUSB_ENABLED
    if (ret == CONTROL_SUCCESS) {
        ret = device_control_init(device_control_usb_ctx,
                                  THIS_XCORE_TILE == USB_TILE_NO ? DEVICE_CONTROL_HOST_MODE : DEVICE_CONTROL_CLIENT_MODE,
                                  appconf_CONTROL_SERVICER_COUNT,
                                  &intertile_ctx, 1);
    }
    if (ret == CONTROL_SUCCESS) {
        ret = device_control_start(device_control_usb_ctx,
                                   appconfDEVICE_CONTROL_USB_PORT,
                                   appconfDEVICE_CONTROL_USB_CLIENT_PRIORITY);
    }
#endif

    return ret;
}
