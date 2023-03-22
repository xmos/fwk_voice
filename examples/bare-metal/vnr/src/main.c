// Copyright 2022 XMOS LIMITED.
// This Software is subject to the terms of the XMOS Public Licence: Version 1.
#include <xcore/channel.h>
#include <xcore/chanend.h>
#include <xcore/select.h>
#include <xscope.h>
#include <stdlib.h>
#include <string.h>
#include "xmath/xmath.h"


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
