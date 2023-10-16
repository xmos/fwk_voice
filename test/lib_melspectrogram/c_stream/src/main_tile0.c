// Copyright 2023 XMOS LIMITED.
// This Software is subject to the terms of the XMOS Public Licence: Version 1.
#include <platform.h>
#include <xs1.h>

#include <stdio.h>
#include <stdlib.h>
#include <stddef.h>
#include <string.h>
#include <stdint.h>
#include <stdbool.h>
#include <math.h>

#include <xscope.h>
#include <xcore/channel.h>
#include <xcore/chanend.h>
#include <xcore/parallel.h>
#include <xcore/select.h>
#include <xcore/hwtimer.h>

#include "melspectrogram_api.h"

#define SETSR(c) asm volatile("setsr %0" \
                              :          \
                              : "n"(c));

// Should be defined in the build process
#ifndef MEL_SMALL
#define MEL_SMALL 1
#endif
#ifndef TEST_BURN
#define TEST_BURN 0
#endif
#ifndef COMPILE_ALGORITHM
#define COMPILE_ALGORITHM 1
#endif
#ifndef USE_QUANT_EQ
#define USE_QUANT_EQ 1
#endif
#ifndef DB_SCALE
#define DB_SCALE 1
#endif
#ifndef SUBTRACT_MEAN
#define SUBTRACT_MEAN 1
#endif

#if MEL_SMALL
#define SAMPLE_BLOCK_LENGTH MEL_SPEC_SMALL_N_SAMPLES
#else
#define SAMPLE_BLOCK_LENGTH MEL_SPEC_LARGE_N_SAMPLES
#endif
#define SAMPLE_TYPE int16_t
#define SAMPLE_SIZE_BYTES sizeof(SAMPLE_TYPE)
#define SAMPLE_BLOCK_SIZE_BYTES SAMPLE_SIZE_BYTES *SAMPLE_BLOCK_LENGTH

#if COMPILE_ALGORITHM

#define N_MELS (MEL_SMALL ? MEL_SPEC_SMALL_N_MEL : MEL_SPEC_LARGE_N_MEL)
#define N_FRAMES (MEL_SMALL ? MEL_SPEC_SMALL_N_FRAMES : MEL_SPEC_LARGE_N_FRAMES)
#define FRAME_DIM (MEL_SMALL ? MEL_SPEC_SMALL_FRAME_DIM_SZ : MEL_SPEC_LARGE_FRAME_DIM_SZ)
#define TOP_TRIM (MEL_SMALL ? MEL_SPEC_SMALL_TRIM_START : MEL_SPEC_LARGE_TRIM_START)
#define END_TRIM (MEL_SMALL ? MEL_SPEC_SMALL_TRIM_END : MEL_SPEC_LARGE_TRIM_END)
#define TOP_DIM (MEL_SMALL ? MEL_SPEC_SMALL_TOP_DIM_SZ : MEL_SPEC_LARGE_TOP_DIM_SZ)
#define LOW_DIM (MEL_SMALL ? MEL_SPEC_SMALL_LOW_DIM_SZ : MEL_SPEC_LARGE_LOW_DIM_SZ)
static uint8_t sample_block[SAMPLE_BLOCK_SIZE_BYTES];
static int8_t output[TOP_DIM][N_MELS][FRAME_DIM][LOW_DIM];
#if (TOP_TRIM > 0)
static int8_t output_top_trim[TOP_DIM][N_MELS][TOP_TRIM][LOW_DIM];
#else
static int8_t * output_top_trim = NULL;
#endif
#if (END_TRIM > 0) 
static int8_t output_end_trim[TOP_DIM][N_MELS][END_TRIM][LOW_DIM];
#else
static int8_t * output_end_trim = NULL;
#endif 

static void process_data(int16_t *samples)
{
    uint32_t tic = get_reference_time();

    // Star of the show - call the mel_spec
    x_melspectrogram(&output[0][0][0][0],
                     (int8_t *)output_top_trim, 
                     (int8_t *)output_end_trim, 
                     samples,
                     MEL_SMALL ? MEL_SPEC_SMALL : MEL_SPEC_LARGE, 
                     USE_QUANT_EQ, 
                     DB_SCALE, 
                     SUBTRACT_MEAN);

    uint32_t toc = get_reference_time();
    uint32_t execution_time = toc - tic;

    for (int mel = 0; mel < N_MELS; mel++)
    {
        for (int frame = 0; frame < FRAME_DIM; frame++)
        {
            xscope_int(MEL_OUTPUT, output[0][mel][frame][0]);
        }
    }
    xscope_int(EXECUTION_TIME, execution_time);
}

static void tile0_data_rx(char *data, size_t size)
{
    static int input_bytes_received = 0;
    // The data input is null-terminated, so we ignore the final byte.
    // We expect to always get 128 bytes of real data, because the test harness
    // guarantees us this.
    memcpy(&sample_block[input_bytes_received], data, size - 1);
    input_bytes_received += size - 1;
    // SAMPLE_BLOCK_SIZE_BYTES should always be a multiple of 128
    if (input_bytes_received == SAMPLE_BLOCK_SIZE_BYTES)
    {
        process_data((int16_t *)sample_block);
        input_bytes_received = 0;
    }
}
#endif

DECLARE_JOB(process, (chanend_t));
void process(chanend_t c_xscope_data_in)
{
    int bytes_read = 0;
    char buffer[256];

    xscope_connect_data_from_host(c_xscope_data_in);
    xscope_mode_lossless();
    SELECT_RES(
        CASE_THEN(c_xscope_data_in, xscope_handler),
        DEFAULT_THEN(default_handler))
    {
    xscope_handler:
        xscope_data_from_host(c_xscope_data_in, &buffer[0], &bytes_read);
#if COMPILE_ALGORITHM
        tile0_data_rx(buffer, bytes_read);
#endif
        SELECT_CONTINUE_RESET;
    default_handler:
        SELECT_CONTINUE_RESET;
    }
}

DECLARE_JOB(burn, (void));
void burn(void)
{
    SETSR(XS1_SR_QUEUE_MASK | XS1_SR_FAST_MASK);
    for (;;)
        ;
}

void main_tile0(chanend_t c_xscope_data_in)
{
    PAR_JOBS(
        PJOB(process, (c_xscope_data_in)),
        PJOB(burn, ()),
        PJOB(burn, ()),
        PJOB(burn, ()),
        PJOB(burn, ())
#if TEST_BURN
            ,
        PJOB(burn, ()),
        PJOB(burn, ()),
        PJOB(burn, ())
#endif
    );
}