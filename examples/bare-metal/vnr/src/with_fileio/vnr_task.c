
// Copyright 2022 XMOS LIMITED.
// This Software is subject to the terms of the XMOS Public Licence: Version 1.
#include "vnr_features_api.h"
#include "vnr_inference_api.h"
#if PROFILE_PROCESSING
#include "profile.h"
#else
static void prof(int n, const char* str) {}
static void print_prof(int a, int b, int framenum){}
#endif
#include "fileio.h"
#include "wav_utils.h"

void vnr_task(const char* input_filename, const char *output_filename){
    vnr_input_state_t vnr_input_state;
    vnr_feature_state_t vnr_feature_state;
    
    prof(0, "start_vnr_init");
    vnr_input_state_init(&vnr_input_state);
    vnr_feature_state_init(&vnr_feature_state);
    int32_t err = vnr_inference_init();
    prof(1, "end_vnr_init");
    if(err) {
        printf("ERROR: vnr_inference_init() returned error %ld\n",err);
        assert(0);
    }

    file_t input_file;
    int ret = file_open(&input_file, input_filename, "rb");
    assert((!ret) && "Failed to open file");

    file_t output_file;
    ret = file_open(&output_file, output_filename, "wb");


    int framenum = 0;
    wav_header input_header_struct;
    unsigned input_header_size;
    if(get_wav_header_details(&input_file, &input_header_struct, &input_header_size) != 0){
        printf("error in get_wav_header_details()\n");
        _Exit(1);
    }
    file_seek(&input_file, input_header_size, SEEK_SET);
    // Ensure 16bit wav file
    if((input_header_struct.bit_depth != 32) && (input_header_struct.bit_depth != 16)){
        printf("Test works on only 16 bit or 32 bit wav files. %d bit wav input provided\n",input_header_struct.bit_depth);
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
    unsigned bytes_per_frame = wav_get_num_bytes_per_frame(&input_header_struct);

    int16_t input_read_buffer[VNR_FRAME_ADVANCE] = {0}; // Array for storing interleaved input read from wav file
    int32_t new_frame[VNR_FRAME_ADVANCE] = {0};

    for(unsigned b=0; b<block_count; b++) {
        long input_location =  wav_get_frame_start(&input_header_struct, b * VNR_FRAME_ADVANCE, input_header_size);
        file_seek (&input_file, input_location, SEEK_SET);
        
        if(input_header_struct.bit_depth == 16){
            file_read (&input_file, (uint8_t*)&input_read_buffer[0], bytes_per_frame* VNR_FRAME_ADVANCE);
            // Convert to 32 bit
            for(int i=0; i<VNR_FRAME_ADVANCE; i++) {
                new_frame[i] = (int32_t)input_read_buffer[i] << 16; //1.31
            }
        }
        else {
            file_read (&input_file, (uint8_t*)&new_frame[0], bytes_per_frame* VNR_FRAME_ADVANCE);
        }
        prof(2, "start_vnr_form_input_frame");
        complex_s32_t DWORD_ALIGNED input_frame[VNR_FD_FRAME_LENGTH];
        bfp_complex_s32_t X;
        vnr_form_input_frame(&vnr_input_state, &X, input_frame, new_frame);
        prof(3, "end_vnr_form_input_frame");

        prof(4, "start_vnr_extract_features");
        bfp_s32_t feature_patch;
        int32_t feature_patch_data[VNR_PATCH_WIDTH*VNR_MEL_FILTERS];
        vnr_extract_features(&vnr_feature_state, &feature_patch, feature_patch_data, &X);
        prof(5, "end_vnr_extract_features");

        prof(6, "start_vnr_inference");
        float_s32_t inference_output;
        //printf("inference_output: 0x%x, %d\n", inference_output.mant, inference_output.exp);
        vnr_inference(&inference_output, &feature_patch);
        prof(7, "end_vnr_inference");
        file_write(&output_file, (uint8_t*)&inference_output, sizeof(float_s32_t));

        framenum++;
        print_prof(0, 8, framenum);        
    }
    file_close(&input_file);
    file_close(&output_file);
    shutdown_session();
}

#if X86_BUILD
int main(int argc, char** argv) {
    if(argc < 3) {
        printf("Incorrect no. of arguments. Expected input and output file names\n");
        assert(0);
    }
    vnr_task(argv[1], argv[2]);
}
#endif
