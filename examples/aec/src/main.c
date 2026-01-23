// Copyright 2022 XMOS LIMITED.
// This Software is subject to the terms of the XMOS Public Licence: Version 1.
#include <stdio.h>
#include <assert.h>
#include "aec.h"

static inline void producer(int32_t frame_y[AEC_MAX_Y_CHANNELS][AEC_FRAME_ADVANCE],
                            int32_t frame_x[AEC_MAX_X_CHANNELS][AEC_FRAME_ADVANCE]) {
    for(unsigned ch = 0; ch < AEC_MAX_Y_CHANNELS; ch++) {
        for(unsigned samp = 0; samp < AEC_FRAME_ADVANCE; samp++){
            frame_y[ch][samp] = ch * samp;
        }
    }
    for(unsigned ch = 0; ch < AEC_MAX_X_CHANNELS; ch++) {
        for(unsigned samp = 0; samp < AEC_FRAME_ADVANCE; samp++){
            frame_x[ch][samp] = ch * samp;
        }
    }
}

static inline void consumer(int32_t frame_y[AEC_MAX_Y_CHANNELS][AEC_FRAME_ADVANCE]) {
    (void)frame_y;
    printf("frame done\n");
}

int main() {

    #if AEC_THREADS == 1
    aec_task_distribution_t tdist = aec_tdist_chans2_threads1;
    #elif AEC_THREADS == 2
    aec_task_distribution_t tdist = aec_tdist_chans2_threads2;
    #else
    #error "Unsupported number of AEC threads"
    #endif

    int32_t DWORD_ALIGNED frame_y[AEC_MAX_Y_CHANNELS][AEC_FRAME_ADVANCE];
    int32_t DWORD_ALIGNED frame_x[AEC_MAX_X_CHANNELS][AEC_FRAME_ADVANCE];

    // Initialise AEC
    aec_state_t DWORD_ALIGNED aec_state;
    aec_init(&aec_state,
            AEC_MAX_Y_CHANNELS, AEC_MAX_X_CHANNELS,
            AEC_MAIN_FILTER_PHASES, AEC_SHADOW_FILTER_PHASES, &tdist);

    for(unsigned b = 0; b < 5; b++){
        producer(frame_y, frame_x);
        // Reuse mic data memory for the main filter output
        // Reuse ref data memory for the shadow filter output
        aec_process_frame(&aec_state, frame_y, frame_x, frame_y, frame_x);
        consumer(frame_y);
    }
    return 0;
}
