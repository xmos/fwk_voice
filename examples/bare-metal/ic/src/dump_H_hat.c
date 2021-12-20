// Copyright 2021 XMOS LIMITED.
// This Software is subject to the terms of the XMOS Public Licence: Version 1.

#include "ic_api.h"

#include <stdio.h>
#include <math.h>
#include "fileio.h"

void ic_dump_H_hat(ic_state_t *state, file_t *file_handle){
    char strbuf[1024];
    sprintf(strbuf, "import numpy as np\n");
    file_write(file_handle, (uint8_t*)strbuf, strlen(strbuf));
    sprintf(strbuf, "frame_advance = %u\n", IC_FRAME_ADVANCE);
    file_write(file_handle, (uint8_t*)strbuf,  strlen(strbuf));
    sprintf(strbuf, "y_channel_count = %u\n", IC_Y_CHANNELS);
    file_write(file_handle, (uint8_t*)strbuf,  strlen(strbuf));
    sprintf(strbuf, "x_channel_count = %u\n", IC_X_CHANNELS);
    file_write(file_handle, (uint8_t*)strbuf,  strlen(strbuf));
    sprintf(strbuf, "max_phase_count = %u\n", IC_FILTER_PHASES);
    file_write(file_handle, (uint8_t*)strbuf,  strlen(strbuf));
    sprintf(strbuf, "f_bin_count = %u\n", state->H_hat_bfp[0][0].length);
    file_write(file_handle, (uint8_t*)strbuf,  strlen(strbuf));
    sprintf(strbuf, "H_hat = np.zeros((y_channel_count, x_channel_count, max_phase_count, f_bin_count), dtype=np.complex128)\n");
    file_write(file_handle, (uint8_t*)strbuf,  strlen(strbuf));

    for(int ych=0; ych<IC_Y_CHANNELS; ych++) {        
        for(int xch=0; xch<IC_X_CHANNELS; xch++) {
            for(int ph=0; ph<IC_FILTER_PHASES; ph++) {
                sprintf(strbuf, "H_hat[%u][%u][%u] = ", ych, xch, ph);
                file_write(file_handle, (uint8_t*)strbuf,  strlen(strbuf));
                sprintf(strbuf, "np.asarray([");
                file_write(file_handle, (uint8_t*)strbuf,  strlen(strbuf));
                for(int i=0; i<state->H_hat_bfp[ych][xch*IC_FILTER_PHASES + ph].length; i++) {
                    sprintf(strbuf, "%.12f + %.12fj, ", ldexp( state->H_hat_bfp[ych][xch*IC_FILTER_PHASES + ph].data[i].re, state->H_hat_bfp[ych][xch*IC_FILTER_PHASES + ph].exp),
                    ldexp( state->H_hat_bfp[ych][xch*IC_FILTER_PHASES + ph].data[i].im, state->H_hat_bfp[ych][xch*IC_FILTER_PHASES + ph].exp));
                    file_write(file_handle, (uint8_t*)strbuf,  strlen(strbuf));
                }
                sprintf(strbuf, "])\n");
                file_write(file_handle, (uint8_t*)strbuf,  strlen(strbuf));
            }
        }
    }
}

