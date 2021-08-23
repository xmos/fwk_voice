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
#if 1
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
#endif

    uint8_t buf[16];

    for (int i = 1; i <= 4; i++) {
#if 1
        for (int i = 0; i < sizeof(buf); i++) {
            buf[i] = 0x90 + i;
        }

        ret = control_write_command(i,
                                   0x04,
                                   buf,
                                   sizeof(buf));
        if (ret == CONTROL_SUCCESS) {
            printf("Write command complete\n");
        } else {
            printf("Write command failed\n");
        }
#endif
#if 1
        ret = control_read_command(i,
                                   0x85,
                                   buf,
                                   sizeof(buf));
        if (ret == CONTROL_SUCCESS) {
            printf("Read command complete\n\t");
            for (int i = 0; i < sizeof(buf); i++) {
                printf("%02x ", buf[i]);
            }
            printf("\n");
        } else {
            printf("Read command failed\n");
        }
#endif
    }

    return 0;
}
