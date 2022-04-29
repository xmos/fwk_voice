// Copyright 2022 XMOS LIMITED.
// This Software is subject to the terms of the XMOS Public Licence: Version 1.
#include <xcore/channel.h>
#include <xcore/chanend.h>
#include <xcore/select.h>
#include <xscope.h>
#include <stdlib.h>
#include <string.h>
#include <xs3_math.h>
#include "vnr_features_api.h"
#include "vnr_inference_api.h"

extern void vnr_task_init();
extern float_s32_t vnr_process_frame(int32_t *new_frame);

void get_frame(chanend_t xscope_chan, chanend_t c_frame_out){
  char buffer[256];
  int32_t data[240];
  int bytes_read = 0;
  int input_bytes = 0;
  xscope_mode_lossless();
  xscope_connect_data_from_host(xscope_chan);
  while(1){
    SELECT_RES(CASE_THEN(xscope_chan, read_host_data)){
      read_host_data:{
        xscope_data_from_host(xscope_chan, &buffer[0], &bytes_read);
        memcpy(&data[0] + input_bytes / 4, &buffer[0], bytes_read);
        input_bytes += bytes_read;
        if(input_bytes / 4 == 240){
          chan_out_buf_word(c_frame_out, (uint32_t *)data, input_bytes / 4);
          input_bytes = 0;
        }
      }
    }
  }
}

void vnr(chanend_t c_frame_in, chanend_t c_frame_out)
{
    vnr_input_state_t vnr_input_state;
    vnr_feature_state_t vnr_feature_state;
    vnr_ie_state_t vnr_ie_state;

    vnr_input_state_init(&vnr_input_state);
    vnr_feature_state_init(&vnr_feature_state);
    int32_t err = vnr_inference_init(&vnr_ie_state);
    while(1)
    {
        int32_t new_frame[240];
        chan_in_buf_word(c_frame_in, (uint32_t*)&new_frame[0], 240);

        int32_t DWORD_ALIGNED input_frame[VNR_PROC_FRAME_LENGTH + VNR_FFT_PADDING];
        bfp_complex_s32_t X;
        vnr_form_input_frame(&vnr_input_state, &X, input_frame, new_frame);

        bfp_s32_t feature_patch;
        int32_t feature_patch_data[VNR_PATCH_WIDTH*VNR_MEL_FILTERS];
        vnr_extract_features(&vnr_feature_state, &feature_patch, feature_patch_data, &X);

        float_s32_t inference_output;
        //printf("inference_output: 0x%x, %d\n", inference_output.mant, inference_output.exp);
        vnr_inference(&vnr_ie_state, &inference_output, &feature_patch);
        chan_out_buf_byte(c_frame_out, (uint8_t*)&inference_output, sizeof(float_s32_t));
    }
}

void send_frame(chanend_t c_frame_in){
  float_s32_t vnr_output;
  int32_t *buffer = (int32_t*)&vnr_output;
  while(1){
    chan_in_buf_byte(c_frame_in, (uint8_t *)buffer, sizeof(float_s32_t));
    //printf("inference_output send_frame: 0x%x, %d\n", vnr_output.mant, vnr_output.exp);
    for(int v = 0; v < sizeof(float_s32_t)/4; v++){
      xscope_int(0, buffer[v]);
    }
  }
}
