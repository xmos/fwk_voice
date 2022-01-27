// Copyright 2017-2021 XMOS LIMITED.
// This Software is subject to the terms of the XMOS Public Licence: Version 1.

#include <fcntl.h>
#include <xclib.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <unistd.h>

#include "vad.h"
#include "voice_toolbox.h"
#include "audio_test_tools.h"


//These don't make a lot of sense to define anywhere
#define VAD_INPUT_CHANNELS 1
#define VAD_OUTPUT_CHANNES 1
//#define VAD_PROC_FRAME_LENGTH_LOG2 9
#define VAD_FRAME_ADVANCE 240



//#define VAD_PROC_FRAME_LENGTH (1<<VAD_PROC_FRAME_LENGTH_LOG2)
#define VAD_INPUT_CHANNEL_PAIRS ((VAD_INPUT_CHANNELS+1)/2)
#define VAD_OUTPUT_CHANNEL_PAIRS ((VAD_OUTPUT_CHANNES+1)/2)

void vad_test_task(chanend app_to_vad, chanend vad_to_app, chanend ?c_control){


    vad_state_t [[aligned(8)]] state[VAD_OUTPUT_CHANNES];

    for(unsigned ch=0;ch<VAD_OUTPUT_CHANNES;ch++)
        vad_init_state(state[ch]);


#define APP_TO_VAD_STATE VTB_RX_STATE_UINT64_SIZE(VAD_INPUT_CHANNELS*2, VAD_PROC_FRAME_LENGTH, VAD_FRAME_ADVANCE, 0)
    uint64_t rx_state[APP_TO_VAD_STATE];
    vtb_md_t md;
    vtb_md_init(md);

    vtb_rx_state_init(rx_state, VAD_INPUT_CHANNEL_PAIRS*2, VAD_PROC_FRAME_LENGTH, VAD_FRAME_ADVANCE,
            null, APP_TO_VAD_STATE);

    vtb_ch_pair_t [[aligned(8)]] in_frame[VAD_INPUT_CHANNEL_PAIRS][VAD_PROC_FRAME_LENGTH];
    vtb_ch_pair_t [[aligned(8)]] out_frame[VAD_OUTPUT_CHANNEL_PAIRS][VAD_FRAME_ADVANCE];


    while(1){

        vtb_rx_notification_and_data(app_to_vad, rx_state, (in_frame, vtb_ch_pair_t[]), md);

        for(unsigned ch = 0;ch < VAD_OUTPUT_CHANNES;ch++){
            int32_t [[aligned(8)]] single_ch_data[VAD_PROC_FRAME_LENGTH];

            for(unsigned s=0;s<VAD_PROC_FRAME_LENGTH;s++){
                single_ch_data[s] = (in_frame[ch/2][s], int32_t[])[ch&1];
            }
            int32_t vad_percentage = vad_percentage_voice(single_ch_data, state[ch]);
            for(unsigned s=0;s<VAD_FRAME_ADVANCE;s++){
                (out_frame[ch/2][s], int32_t[])[ch&1] = vad_percentage << 24;
            }
        }

        vtb_tx_notification_and_data(vad_to_app, (out_frame, vtb_ch_pair_t[]), VAD_OUTPUT_CHANNEL_PAIRS*2, VAD_FRAME_ADVANCE, md);
    }
}
