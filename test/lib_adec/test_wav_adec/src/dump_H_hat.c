// Copyright 2017-2021 XMOS LIMITED.
// This Software is subject to the terms of the XMOS Public Licence: Version 1.

#include "aec_state.h"

#include <stdio.h>
#include <math.h>
#include "fileio.h"

void aec_dump_H_hat(aec_state_t *state, file_t *file_handle){
    char strbuf[1024];
    sprintf(strbuf, "import numpy as np\n");
    file_write(file_handle, (uint8_t*)strbuf, strlen(strbuf));
    sprintf(strbuf, "frame_advance = %u\n", AEC_FRAME_ADVANCE);
    file_write(file_handle, (uint8_t*)strbuf,  strlen(strbuf));
    sprintf(strbuf, "y_channel_count = %u\n", state->shared_state->num_y_channels);
    file_write(file_handle, (uint8_t*)strbuf,  strlen(strbuf));
    sprintf(strbuf, "x_channel_count = %u\n", state->shared_state->num_x_channels);
    file_write(file_handle, (uint8_t*)strbuf,  strlen(strbuf));
    sprintf(strbuf, "max_phase_count = %u\n", state->num_phases);
    file_write(file_handle, (uint8_t*)strbuf,  strlen(strbuf));
    sprintf(strbuf, "f_bin_count = %u\n", state->H_hat[0][0].length);
    file_write(file_handle, (uint8_t*)strbuf,  strlen(strbuf));
    sprintf(strbuf, "H_hat = np.zeros((y_channel_count, x_channel_count, max_phase_count, f_bin_count), dtype=np.complex128)\n");
    file_write(file_handle, (uint8_t*)strbuf,  strlen(strbuf));

    for(int ych=0; ych<state->shared_state->num_y_channels; ych++) {        
        for(int xch=0; xch<state->shared_state->num_x_channels; xch++) {
            for(int ph=0; ph<state->num_phases; ph++) {
                sprintf(strbuf, "H_hat[%u][%u][%u] = ", ych, xch, ph);
                file_write(file_handle, (uint8_t*)strbuf,  strlen(strbuf));
                sprintf(strbuf, "np.asarray([");
                file_write(file_handle, (uint8_t*)strbuf,  strlen(strbuf));
                for(int i=0; i<state->H_hat[ych][xch*state->num_phases + ph].length; i++) {
                    sprintf(strbuf, "%.12f + %.12fj, ", ldexp( state->H_hat[ych][xch*state->num_phases + ph].data[i].re, state->H_hat[ych][xch*state->num_phases + ph].exp),
                    ldexp( state->H_hat[ych][xch*state->num_phases + ph].data[i].im, state->H_hat[ych][xch*state->num_phases + ph].exp));
                    file_write(file_handle, (uint8_t*)strbuf,  strlen(strbuf));
                }
                sprintf(strbuf, "])\n");
                file_write(file_handle, (uint8_t*)strbuf,  strlen(strbuf));
            }
        }
    }
}

