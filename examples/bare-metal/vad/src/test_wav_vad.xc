// Copyright 2017-2021 XMOS LIMITED.
// This Software is subject to the terms of the XMOS Public Licence: Version 1.
#include <platform.h>

#include "audio_test_tools.h"
#include "vad.h"

void app_control(chanend c_control_to_wav, chanend c_control_to_dsp){

    //Play the file
    att_pw_play(c_control_to_wav);
}

int main(){
    chan app_to_dsp;
    chan dsp_to_app;
    chan c_control_to_dsp;
    chan c_control_to_wav;

    par {
        on tile[0]:{
            app_control(c_control_to_wav, c_control_to_dsp);
        }
        on tile[0]:{
            att_process_wav(app_to_dsp, dsp_to_app, c_control_to_wav);
            _Exit(0);
        }
        on tile[1]: vad_test_task (app_to_dsp, dsp_to_app, c_control_to_dsp);
    }
    return 0;
}

