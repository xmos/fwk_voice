// Copyright 2022 XMOS LIMITED.
// This Software is subject to the terms of the XMOS Public Licence: Version 1.

#include "ic_api.h"

#include <stdio.h>
#include <math.h>
#include "fileio.h"


file_t *g_file_handle = NULL;
unsigned frame = 0;

//Define your state variable to print here
#define dut_var_3d state->X_fifo_bfp

void ic_dump_var_3d_start(ic_state_t *state, file_t *file_handle, unsigned n_frames){
    char strbuf[1024];
    sprintf(strbuf, "import numpy as np\n");
    file_write(file_handle, (uint8_t*)strbuf, strlen(strbuf));
    sprintf(strbuf, "frame_advance = %u\n", IC_FRAME_ADVANCE);
    file_write(file_handle, (uint8_t*)strbuf,  strlen(strbuf));
    sprintf(strbuf, "n_frames = %u\n", n_frames);
    file_write(file_handle, (uint8_t*)strbuf,  strlen(strbuf));
    sprintf(strbuf, "max_phase_count = %u\n", IC_FILTER_PHASES);
    file_write(file_handle, (uint8_t*)strbuf,  strlen(strbuf));
    sprintf(strbuf, "f_bin_count = %u\n", dut_var_3d[0][0].length);
    file_write(file_handle, (uint8_t*)strbuf,  strlen(strbuf));
    sprintf(strbuf, "dut_var = np.zeros((n_frames, max_phase_count, f_bin_count), dtype=np.complex128)\n\n");
    file_write(file_handle, (uint8_t*)strbuf,  strlen(strbuf));

    g_file_handle = file_handle;
}

void ic_dump_var_3d(ic_state_t *state){
    char strbuf[1024];
   
    for(int ph=0; ph<IC_FILTER_PHASES; ph++) {
        sprintf(strbuf, "dut_var[%u][%u] = ", frame, ph);
        file_write(g_file_handle, (uint8_t*)strbuf,  strlen(strbuf));
        sprintf(strbuf, "np.asarray([");
        file_write(g_file_handle, (uint8_t*)strbuf,  strlen(strbuf));
        for(int i=0; i<dut_var_3d[0][ph].length; i++) {
            sprintf(strbuf, "%.12f + %.12fj, ", ldexp( dut_var_3d[0][ph].data[i].re, dut_var_3d[0][ph].exp),
            ldexp( dut_var_3d[0][ph].data[i].im, dut_var_3d[0][ph].exp));
            file_write(g_file_handle, (uint8_t*)strbuf,  strlen(strbuf));
        }
        sprintf(strbuf, "])\n");
        file_write(g_file_handle, (uint8_t*)strbuf,  strlen(strbuf));
    }
    frame++;
}

// #define dut_var_2d state->sigma_XX_bfp
// #define dut_var_2d state->X_energy_bfp
// #define dut_var_2d state->inv_X_energy_bfp
#define dut_var_2d state->T_bfp
#define COMPLEX 1

void ic_dump_var_2d_start(ic_state_t *state, file_t *file_handle, unsigned n_frames){
    char strbuf[1024];
    sprintf(strbuf, "import numpy as np\n");
    file_write(file_handle, (uint8_t*)strbuf, strlen(strbuf));
    sprintf(strbuf, "frame_advance = %u\n", IC_FRAME_ADVANCE);
    file_write(file_handle, (uint8_t*)strbuf,  strlen(strbuf));
    sprintf(strbuf, "n_frames = %u\n", n_frames);
    file_write(file_handle, (uint8_t*)strbuf,  strlen(strbuf));
#if COMPLEX
    sprintf(strbuf, "f_bin_count = %u\n", dut_var_2d[0].length);
#else
    sprintf(strbuf, "frame_size = %u\n", dut_var_2d[0].length);
#endif
    file_write(file_handle, (uint8_t*)strbuf,  strlen(strbuf));
#if COMPLEX
    sprintf(strbuf, "dut_var = np.zeros((n_frames, f_bin_count), dtype=np.complex128)\n\n");
#else
    sprintf(strbuf, "dut_var = np.zeros((n_frames, frame_size), dtype=np.double)\n\n");
#endif
    file_write(file_handle, (uint8_t*)strbuf,  strlen(strbuf));
    g_file_handle = file_handle;
}

void ic_dump_var_2d(ic_state_t *state){
    char strbuf[1024];
   
    sprintf(strbuf, "dut_var[%u] = ", frame);
    file_write(g_file_handle, (uint8_t*)strbuf,  strlen(strbuf));
    sprintf(strbuf, "np.asarray([");
    file_write(g_file_handle, (uint8_t*)strbuf,  strlen(strbuf));
    for(int i=0; i<dut_var_2d[0].length; i++) {
#if COMPLEX
        sprintf(strbuf, "%.12f + %.12fj, ", ldexp( dut_var_2d[0].data[i].re, dut_var_2d[0].exp),
        ldexp( dut_var_2d[0].data[i].im, dut_var_2d[0].exp));
#else
        sprintf(strbuf, "%.12lf, ", ldexp( dut_var_2d[0].data[i], dut_var_2d[0].exp));
#endif
        file_write(g_file_handle, (uint8_t*)strbuf,  strlen(strbuf));
    }
    sprintf(strbuf, "])\n");
    file_write(g_file_handle, (uint8_t*)strbuf,  strlen(strbuf));
    frame++;
}