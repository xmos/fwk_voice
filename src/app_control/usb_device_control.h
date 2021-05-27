// Copyright (c) 2021 XMOS LIMITED. This Software is subject to the terms of the
// XMOS Public License: Version 1

#ifndef USB_DEVICE_CONTROL_H_
#define USB_DEVICE_CONTROL_H_

#include "device_control.h"

#define TUD_XMOS_DEVICE_CONTROL_DESC_LEN 9

#define TUD_XMOS_DEVICE_CONTROL_DESCRIPTOR(_itfnum, _stridx) \
  /* Interface */\
  9, TUSB_DESC_INTERFACE, _itfnum, 0, 0, TUSB_CLASS_VENDOR_SPECIFIC, 0x00, 0x00, _stridx

/*
 * Application callback that provides the USB device control driver
 * with the device control context to use.
 */
__attribute__ ((weak))
device_control_t *tud_device_control_get_ctrl_ctx_cb(void);

#endif /* USB_DEVICE_CONTROL_H_ */
