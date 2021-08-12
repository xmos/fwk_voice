/*
 * test.c
 *
 *  Created on: Apr 7, 2021
 *      Author: mbruno
 */

#include <stdio.h>
#include "device_control_host.h"

#define VERSION_CMD 0x80

int main(int argc, char **argv)
{
    control_ret_t ret = CONTROL_ERROR;

#if USE_USB
    ret = control_init_usb(0x20B1, 0x3652, 0);
#elif USE_I2C
    ret = control_init_i2c(0x42);
#endif

    control_version_t version;
    control_status_t status;

    ret = control_read_command(CONTROL_SPECIAL_RESID,
                               CONTROL_GET_VERSION,
                               &version,
                               sizeof(version));

    if (ret == CONTROL_SUCCESS) {
        printf("This is device control version %d\n", version);
    }

    ret = control_read_command(CONTROL_SPECIAL_RESID,
                               CONTROL_GET_LAST_COMMAND_STATUS,
                               &status,
                               sizeof(status));

    if (ret == CONTROL_SUCCESS) {
        printf("This last command status is %d\n", status);
    }

    uint8_t buf[16];
    ret = control_read_command('A',
                               0x85,
                               buf,
                               sizeof(buf));
    if (ret == CONTROL_SUCCESS) {
        for (int i = 0; i < sizeof(buf); i++) {
            printf("%02x ", buf[i]);
        }
        printf("\n");
    }

    ret = control_read_command(CONTROL_SPECIAL_RESID,
                               CONTROL_GET_LAST_COMMAND_STATUS,
                               &status,
                               sizeof(status));

    if (ret == CONTROL_SUCCESS) {
        printf("This last command status is %d\n", status);
    }

    for (int i = 0; i < sizeof(buf); i++) {
        buf[i] = 0x90 + i;
    }
    ret = control_write_command('E',
                               0x04,
                               buf,
                               sizeof(buf));
    if (ret == CONTROL_SUCCESS) {
        printf("Write command complete\n");
    } else {
        printf("Write command failed\n");
    }


    ret = control_read_command('C',
                               0x86,
                               buf,
                               sizeof(buf));
    if (ret == CONTROL_SUCCESS) {
        for (int i = 0; i < sizeof(buf); i++) {
            printf("%02x ", buf[i]);
        }
        printf("\n");
    }

    ret = control_read_command('A',
                               0x85,
                               buf,
                               sizeof(buf));
    if (ret == CONTROL_SUCCESS) {
        for (int i = 0; i < sizeof(buf); i++) {
            printf("%02x ", buf[i]);
        }
        printf("\n");
    }


    return 0;
//    ret = control_write_command(SPECIAL_RESID,
//                                    DEV_CTRL_CMD_SET_READ_PARAMS,
//                                    (uint8_t *) params,
//                                    param_count * sizeof(uint32_t));
}
