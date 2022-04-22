// Copyright 2020-2021 XMOS LIMITED.
// This Software is subject to the terms of the XMOS Public Licence: Version 1.
#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <xcore/hwtimer.h>

#include "vnr_features_api.h"
#include "fileio.h"
#include "wav_utils.h"

#define AUDIO_FEATURES_NUM_MELS (24)
static int framenum=0;
static uint32_t max_fft_cycles=0, max_mel_cycles=0, max_log2_cycles=0;

void dut_mel(vnr_input_state_t *input_state, const int32_t *new_x_frame, file_t *mel_file, file_t *mel_exp_file, file_t *fft_file, file_t *fft_exp_file, file_t *log2_file, file_t *quant_patch_file) {
    int32_t DWORD_ALIGNED x_data[VNR_PROC_FRAME_LENGTH + VNR_FFT_PADDING];
    vnr_form_input_frame(input_state, x_data, new_x_frame);
    
    uint64_t start_fft, end_fft, start_mel, end_mel, start_log2, end_log2;
    start_fft = (uint64_t)get_reference_time();
    bfp_complex_s32_t X;
    vnr_forward_fft(&X, x_data);
    end_fft = (uint64_t)get_reference_time();

    file_write(fft_file, (uint8_t*)(X.data), X.length*sizeof(complex_s32_t));
    file_write(fft_exp_file, (uint8_t*)(&X.exp), 1*sizeof(exponent_t));
    
    // MEL
    start_mel = (uint64_t)get_reference_time();
    float_s32_t mel_output[AUDIO_FEATURES_NUM_MELS];
    vnr_mel_compute(mel_output, &X);
    end_mel = (uint64_t)get_reference_time();

    //log2
    start_log2 = (uint64_t)get_reference_time();
    fixed_s32_t mel_log2[AUDIO_FEATURES_NUM_MELS];
    vnr_log2(mel_log2, mel_output, AUDIO_FEATURES_NUM_MELS);
    end_log2 = (uint64_t)get_reference_time();

    vnr_add_new_slice(input_state->feature_buffers, mel_log2);
#if (VNR_FD_FRAME_LENGTH < (VNR_MEL_FILTERS*VNR_PATCH_WIDTH))
    #error ERROR squared_mag_data memory not enough for reuse as normalised_patch
#endif

    bfp_s32_t normalised_patch;
    // Reuse x_data memory for normalised_patch
    bfp_s32_init(&normalised_patch, x_data, 0, VNR_MEL_FILTERS*VNR_PATCH_WIDTH, 1);
    vnr_normalise_patch(&normalised_patch, input_state->feature_buffers);

    int8_t quantised_patch[VNR_MEL_FILTERS*VNR_PATCH_WIDTH];
    vnr_quantise_patch(quantised_patch, &normalised_patch, &input_state->vnr_features_config); 
    
    //profile
    uint32_t fft_cycles = (uint32_t)(end_fft-start_fft);
    uint32_t mel_cycles = (uint32_t)(end_mel-start_mel);
    uint32_t log2_cycles = (uint32_t)(end_log2 - start_log2);
    if(max_fft_cycles < fft_cycles) {max_fft_cycles = fft_cycles;}
    if(max_mel_cycles < mel_cycles) {max_mel_cycles = mel_cycles;}
    if(max_log2_cycles < log2_cycles) {max_log2_cycles = log2_cycles;} 

    //printf("FFT cycles %ld, MEL cycles %ld\n", (uint32_t)(end_fft-start_fft), (uint32_t)(end_mel-start_mel));
    for(int i=0; i<AUDIO_FEATURES_NUM_MELS; i++) {
        file_write(mel_file, (uint8_t*)(&mel_output[i].mant), sizeof(int32_t));
        file_write(mel_exp_file, (uint8_t*)(&mel_output[i].exp), sizeof(exponent_t));
    }
    
    file_write(log2_file, (uint8_t*)(mel_log2), AUDIO_FEATURES_NUM_MELS*sizeof(int32_t));

    file_write(quant_patch_file, (uint8_t*)quantised_patch, VNR_PATCH_WIDTH*VNR_MEL_FILTERS*sizeof(int8_t));
}

void test_mel(const char *in_filename, const char *mel_filename, const char *mel_exp_filename, const char *fft_filename, const char *fft_exp_filename)
{
    file_t input_file, mel_file, mel_exp_file, fft_file, fft_exp_file, mel_log2_file, quant_patch_file;

    int ret = file_open(&input_file, in_filename, "rb");
    assert((!ret) && "Failed to open file");

    ret = file_open(&mel_file, mel_filename, "wb");
    assert((!ret) && "Failed to open file");
    ret = file_open(&mel_exp_file, mel_exp_filename, "wb");
    assert((!ret) && "Failed to open file");
    
    ret = file_open(&fft_file, fft_filename, "wb");
    assert((!ret) && "Failed to open file");
    ret = file_open(&fft_exp_file, fft_exp_filename, "wb");
    assert((!ret) && "Failed to open file");

    ret = file_open(&mel_log2_file, "mel_log2.bin", "wb");
    assert((!ret) && "Failed to open file");

    ret = file_open(&quant_patch_file, "quant_patch.bin", "wb");
    assert((!ret) && "Failed to open file");

    wav_header input_header_struct;
    unsigned input_header_size;
    if(get_wav_header_details(&input_file, &input_header_struct, &input_header_size) != 0){
        printf("error in get_wav_header_details()\n");
        _Exit(1);
    }
    file_seek(&input_file, input_header_size, SEEK_SET);
    // Ensure 16bit wav file
    if(input_header_struct.bit_depth != 16){
        printf("Test works on only 16 bit wav files. %d bit wav input provided\n",input_header_struct.bit_depth);
        assert(0);
    }

    // Ensure input wav file contains correct number of channels 
    if(input_header_struct.num_channels != 1){
        printf("Error: wav num channels(%d) does not match expected 1 channel\n", input_header_struct.num_channels);
        _Exit(1);
    }

    unsigned frame_count = wav_get_num_frames(&input_header_struct);
    // Calculate number of frames in the wav file
    unsigned block_count = frame_count / VNR_FRAME_ADVANCE;
    printf("%d Frames in this test input wav\n",block_count);

    vnr_input_state_t DWORD_ALIGNED vnr_input_state;
    vnr_input_state_init(&vnr_input_state);

    unsigned bytes_per_frame = wav_get_num_bytes_per_frame(&input_header_struct);
    int16_t input_read_buffer[VNR_FRAME_ADVANCE] = {0}; // Array for storing interleaved input read from wav file
    int32_t new_frame[VNR_FRAME_ADVANCE] = {0};
    
    for(unsigned b=0; b<block_count; b++) {
        long input_location =  wav_get_frame_start(&input_header_struct, b * VNR_FRAME_ADVANCE, input_header_size);
        file_seek (&input_file, input_location, SEEK_SET);
        file_read (&input_file, (uint8_t*)&input_read_buffer[0], bytes_per_frame* VNR_FRAME_ADVANCE);
        // Convert to 32 bit
        for(int i=0; i<VNR_FRAME_ADVANCE; i++) {
            new_frame[i] = (int32_t)input_read_buffer[i] << 16; //1.31
        }
        dut_mel(&vnr_input_state, new_frame, &mel_file, &mel_exp_file, &fft_file, &fft_exp_file, &mel_log2_file, &quant_patch_file);
        framenum += 1;
        /*if(framenum == 1) {
            break;
        }*/
    }
    printf("Profile: max_fft_cycles = %ld, max_mel_cycles = %ld, max_log2_cycles = %ld\n",max_fft_cycles, max_mel_cycles, max_log2_cycles);
    shutdown_session(); 
}
