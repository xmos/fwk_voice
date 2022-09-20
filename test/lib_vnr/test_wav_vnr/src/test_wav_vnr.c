// Copyright 2020-2021 XMOS LIMITED.
// This Software is subject to the terms of the XMOS Public Licence: Version 1.
#include <stdio.h>
#include <string.h>
#include <stdint.h>
#if !X86_BUILD
    #include <xcore/hwtimer.h>
#else
    int get_reference_time(void) {return 0;}
#endif

#include "vnr_features_api.h"
#include "vnr_inference_api.h"
#include "fileio.h"
#include "wav_utils.h"

static int framenum=0;
static uint32_t max_feature_cycles=0, max_inference_cycles=0;

void test_wav_vnr(const char *in_filename)
{
    file_t input_file, new_slice_file, norm_patch_file, inference_output_file;

    int ret = file_open(&input_file, in_filename, "rb");
    assert((!ret) && "Failed to open file");

    ret = file_open(&new_slice_file, "new_slice.bin", "wb");
    assert((!ret) && "Failed to open file");

    ret = file_open(&norm_patch_file, "normalised_patch.bin", "wb");
    assert((!ret) && "Failed to open file");

    ret = file_open(&inference_output_file, "inference_output.bin", "wb");
    assert((!ret) && "Failed to open file");

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
    
    uint64_t start_init_cycles = (uint64_t)get_reference_time();
    vnr_input_state_t vnr_input_state;
    vnr_input_state_init(&vnr_input_state);

    vnr_feature_state_t vnr_feature_state;
    vnr_feature_state_init(&vnr_feature_state);

    int32_t err = vnr_inference_init();
    if(err) {
        printf("vnr_inference_init() returned error %ld. Exiting.\n", err);
        assert(0);
    }
    
    uint64_t end_init_cycles = (uint64_t)get_reference_time();
    uint32_t vnr_init_cycles = (uint32_t)(end_init_cycles - start_init_cycles);

    /*if(vnr_ie_state.input_size != (VNR_PATCH_WIDTH * VNR_MEL_FILTERS)) {
        printf("Error: Feature size mismatch\n");
        assert(0);
    }*/
    
    uint64_t start_feature_cycles, end_feature_cycles, start_inference_cycles, end_inference_cycles;
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

        
        // VNR feature extraction
        start_feature_cycles = (uint64_t)get_reference_time();
        complex_s32_t DWORD_ALIGNED input_frame[VNR_FD_FRAME_LENGTH];
        bfp_complex_s32_t X;
        vnr_form_input_frame(&vnr_input_state, &X, input_frame, new_frame);

        bfp_s32_t feature_patch;
        int32_t feature_patch_data[VNR_PATCH_WIDTH*VNR_MEL_FILTERS];
        vnr_extract_features(&vnr_feature_state, &feature_patch, feature_patch_data, &X);

        end_feature_cycles = (uint64_t)get_reference_time();

        file_write(&new_slice_file, (uint8_t*)(vnr_feature_state.feature_buffers[VNR_PATCH_WIDTH-1]), VNR_MEL_FILTERS*sizeof(int32_t));       
        file_write(&norm_patch_file, (uint8_t*)&feature_patch.exp, 1*sizeof(int32_t));
        file_write(&norm_patch_file, (uint8_t*)feature_patch.data, VNR_PATCH_WIDTH*VNR_MEL_FILTERS*sizeof(int32_t));
        
        // VNR inference
        start_inference_cycles = (uint64_t)get_reference_time();
        float_s32_t inference_output;
        vnr_inference(&inference_output, &feature_patch);
        end_inference_cycles = (uint64_t)get_reference_time();
        

        file_write(&inference_output_file, (uint8_t*)&inference_output, sizeof(float_s32_t));

        //profile
        uint32_t fe = (uint32_t)(end_feature_cycles - start_feature_cycles);
        uint32_t ie = (uint32_t)(end_inference_cycles - start_inference_cycles);
        if(max_feature_cycles < fe) {max_feature_cycles = fe;}
        if(max_inference_cycles < ie) {max_inference_cycles = ie;}

        framenum += 1;
        /*if(framenum == 1) {
            break;
        }*/
    }
    printf("Profile: max_feature_extraction_cycles = %ld, max_inference_cycles = %ld, vnr_init_cycles = %ld\n",max_feature_cycles, max_inference_cycles, vnr_init_cycles);
    printf("Profile: max_vnr_cycles (init + 2 feature+inference calls) = %ld\n", vnr_init_cycles + 2*(max_feature_cycles+max_inference_cycles));
    file_close(&input_file);
    file_close(&new_slice_file);
    file_close(&norm_patch_file);
    file_close(&inference_output_file);
    shutdown_session(); 
}

#if X86_BUILD
int main(int argc, char **argv) {
    if(argc != 2) {
        printf("Incorrect number of input arguments. Provide input file\n");
        assert(0);
    }
    test_wav_vnr(argv[1]);
}
#endif
