// Copyright 2022 XMOS LIMITED.
// This Software is subject to the terms of the XMOS Public Licence: Version 1.
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <limits.h>

#include "fileio.h"
#include "wav_utils.h"
#include "xmath/xmath.h"
#include "ic_defines.h"

extern void test_init(int32_t adapt_conf, int32_t * H_data);
extern void test(int32_t *output, int32_t * y_frame, int32_t * x_frame);
void test_bad_state(const char *conf_file_name, const char *input_file_name, const char *output_file_name)
{
    file_t conf_file, input_file, output_file;
    // Open conf_file containing the filter pre settings
    int ret = file_open(&conf_file, conf_file_name, "rb");
    assert((!ret) && "Failed to open file");
    // Open input wav file containing mic and ref channels of input data
    ret = file_open(&input_file, input_file_name, "rb");
    assert((!ret) && "Failed to open file");
    // Open output wav file that will contain the IC output
    ret = file_open(&output_file, output_file_name, "wb");
    assert((!ret) && "Failed to open file");
    
    // Read the data to initialise filter
    int num_words_H_py, adapt_mode;
    // Num words to accomodate H_hat data
    int num_words_H_c = IC_Y_CHANNELS * IC_FD_FRAME_LENGTH * IC_FILTER_PHASES * 2;
    file_read(&conf_file, &num_words_H_py, sizeof(int32_t));
    assert((num_words_H_py == num_words_H_c) && "num_words_h does not match with python");
    file_read(&conf_file, &adapt_mode, sizeof(int32_t));
    printf("num_words_H=%d, adapt_mode=%d\n", num_words_H_py, adapt_mode);
    
    int32_t H_hat_data[num_words_H_py];
    file_read(&conf_file, &H_hat_data[0], num_words_H_py * sizeof(int32_t));
    test_init(adapt_mode, H_hat_data);

    wav_header input_header, output_header;
    unsigned input_header_size;
    if(get_wav_header_details(&input_file, &input_header, &input_header_size) != 0){
        printf("error in get_wav_header_details()\n");
        _Exit(1);
    }
    file_seek(&input_file, input_header_size, SEEK_SET);
    if(input_header.bit_depth != 32)
    {
        printf("Error: unsupported wav bit depth (%d) for %s file. Only 32 supported\n", input_header.bit_depth, input_file_name);
        _Exit(1);
    }

    if(input_header.num_channels != 2){
        printf("Error: wav num channels(%d) does not match ic(2)\n", input_header.num_channels);
        _Exit(1);
    }

    int num_frames = wav_get_num_frames(&input_header);
    unsigned block_count = num_frames / 240;

    wav_form_header(&output_header,
            input_header.audio_format,
            1,
            input_header.sample_rate,
            input_header.bit_depth,
            block_count * 240);

    file_write(&output_file, (uint8_t*)(&output_header),  WAV_HEADER_BYTES);

    int32_t in_buff [240 * 2];
    int32_t DWORD_ALIGNED y_frame[240];
    int32_t DWORD_ALIGNED x_frame[240];
    int32_t DWORD_ALIGNED output_frame[240];

    unsigned bytes_per_frame = wav_get_num_bytes_per_frame(&input_header);

    for(unsigned b = 0; b < block_count; b++){
        //printf("frame %d of %d\n", b, block_count);
        long input_location =  wav_get_frame_start(&input_header, b * 240, input_header_size);
        file_seek (&input_file, input_location, SEEK_SET);
        file_read (&input_file, (uint8_t*)&in_buff[0], bytes_per_frame* 240);
        for(unsigned f = 0; f < 240; f++){
            unsigned i = (f * 2);
            y_frame[f] = in_buff[i];
            x_frame[f] = in_buff[i + 1];
        }

        test(output_frame, y_frame, x_frame);

        file_write(&output_file, (uint8_t*)(output_frame), output_header.bit_depth / 8 * 240);
    }

    file_close(&input_file);
    file_close(&output_file);
    shutdown_session();
}

#if X86_BUILD
int main(int argc, char **argv) {
    test_vnr_unit("input.bin", "output.bin");
    return 0;
}
#endif
