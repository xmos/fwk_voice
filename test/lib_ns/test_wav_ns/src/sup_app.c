// Copyright 2021 XMOS LIMITED.
// This Software is subject to the terms of the XMOS Public Licence: Version 1.
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <limits.h>

#include <ns_api.h>

#include <fileio.h>
#include <wav_utils.h>

#define MAX_CHANNELS 1

#if PROFILE_PROCESSING
#include "profile.h"
#include "ns_test.h"
#include "ns_priv.h"
#include "bfp_math.h"
#endif

extern void ns_process_frame(ns_state_t * state,
                        int32_t output [NS_FRAME_ADVANCE],
                        const int32_t input[NS_FRAME_ADVANCE]);

void ns_task(const char *input_file_name, const char *output_file_name){
    file_t input_file, output_file;

    // Open input wav file containing mic and ref channels of input data
    int ret = file_open(&input_file, input_file_name, "rb");
    assert((!ret) && "Failed to open file");
    // Open output wav file that will contain the Noise Suppressor output
    ret = file_open(&output_file, output_file_name, "wb");
    assert((!ret) && "Failed to open file");

    wav_header input_header_struct, output_header_struct;
    unsigned input_header_size;
    if(get_wav_header_details(&input_file, &input_header_struct, &input_header_size) != 0){
        printf("error in get_wav_header_details()\n");
        _Exit(1);
    }
    file_seek(&input_file, input_header_size, SEEK_SET);
    // Ensure 32bit wav file
    if(input_header_struct.bit_depth != 32)
    {
        printf("Error: unsupported wav bit depth (%d) for %s file. Only 32 supported\n", input_header_struct.bit_depth, input_file_name);
        _Exit(1);
    }
    // Ensure input wav file contains correct number of channels 
    if(input_header_struct.num_channels != MAX_CHANNELS){
        printf("Error: wav num channels(%d) does not match ns(%u)\n", input_header_struct.num_channels, MAX_CHANNELS);
        _Exit(1);
    }

    unsigned frame_count = wav_get_num_frames(&input_header_struct);
    // Calculate number of frames in the wav file
    unsigned block_count = frame_count / NS_FRAME_ADVANCE;
    wav_form_header(&output_header_struct,
            input_header_struct.audio_format,
            MAX_CHANNELS,
            input_header_struct.sample_rate,
            input_header_struct.bit_depth,
            block_count * NS_FRAME_ADVANCE);
    
    file_write(&output_file, (uint8_t*)(&output_header_struct),  WAV_HEADER_BYTES);

    int32_t input_read_buffer[NS_FRAME_ADVANCE * MAX_CHANNELS] = {0}; // Array for storing interleaved input read from wav file
    int32_t output_write_buffer[NS_FRAME_ADVANCE * MAX_CHANNELS];

    int32_t DWORD_ALIGNED frame[MAX_CHANNELS][NS_FRAME_ADVANCE];
    
    unsigned bytes_per_frame = wav_get_num_bytes_per_frame(&input_header_struct);

    //Initialise noise suppressor
#if PROFILE_PROCESSING
    prof(0, "start_ns_init");
#endif
    ns_state_t DWORD_ALIGNED ns;

    ns_init(&ns);

#if PROFILE_PROCESSING
    prof(1, "end_ns_init");
#endif

    for(int b = 0; b < block_count; b++){
        long input_location =  wav_get_frame_start(&input_header_struct, b * NS_FRAME_ADVANCE, input_header_size);
        file_seek (&input_file, input_location, SEEK_SET);
        file_read (&input_file, (uint8_t*)&input_read_buffer[0], bytes_per_frame * NS_FRAME_ADVANCE);
        // Deinterleave and copy samples to their respective buffers
        for(unsigned f = 0; f < NS_FRAME_ADVANCE; f++){
            for(unsigned ch = 0; ch < MAX_CHANNELS; ch++){
                unsigned i =(f * MAX_CHANNELS) + ch;
                frame[ch][f] = input_read_buffer[i];
            }
        }
        // Call Noise Suppression functions to process NS_FRAME_ADVANCE new samples of data
        // Reuse mic data memory for main filter output
#if PROFILE_PROCESSING
        prof(2, "start_ns_process_frame");
#endif

#if PROFILE_PROCESSING
        bfp_s32_t curr_frame, abs_Y_suppressed, abs_Y_original;
        int32_t DWORD_ALIGNED curr_frame_data[NS_PROC_FRAME_LENGTH + 2];
        int32_t scratch1[NS_PROC_FRAME_BINS];
        int32_t scratch2[NS_PROC_FRAME_BINS];
        bfp_s32_init(&curr_frame, curr_frame_data, NS_INT_EXP, NS_PROC_FRAME_LENGTH, 0);
        bfp_s32_init(&abs_Y_suppressed, scratch1, NS_INT_EXP, NS_PROC_FRAME_BINS, 0);
        bfp_s32_init(&abs_Y_original, scratch2, NS_INT_EXP, NS_PROC_FRAME_BINS, 0);

        prof(4, "start_frame_packing");
        ns_pack_input(&curr_frame, frame[0], &ns.prev_frame);
        prof(5, "end_frame_packing");

        prof(6, "start_windowing");
        ns_apply_window(&curr_frame, &ns.wind, &ns.rev_wind, NS_PROC_FRAME_LENGTH, NS_WINDOW_LENGTH);
        prof(7, "end_windowing");

        prof(8, "start_fft");
        bfp_complex_s32_t *curr_fft = bfp_fft_forward_mono(&curr_frame);
        bfp_fft_unpack_mono(curr_fft);
        prof(9, "end_fft");

        prof(10, "start_mag");
        bfp_complex_s32_mag(&abs_Y_suppressed, curr_fft);

        memcpy(abs_Y_original.data, abs_Y_suppressed.data, sizeof(int32_t) * NS_PROC_FRAME_BINS);
        abs_Y_original.exp = abs_Y_suppressed.exp;
        abs_Y_original.hr = abs_Y_suppressed.hr;
        prof(11, "end_mag");

        prof(20, "start_proc_frame");
        ns_priv_process_frame(&abs_Y_suppressed, &ns);
        prof(21, "end_proc_frame");

        prof(12, "start_rescaling");
        ns_rescale_vector(curr_fft, &abs_Y_suppressed, &abs_Y_original);
        prof(13, "end_rescaling");

        prof(14, "start_ifft");
        bfp_fft_pack_mono(curr_fft);
        bfp_fft_inverse_mono(curr_fft);
        prof(15, "end_ifft");

        prof(16, "start_windowing2");
        ns_apply_window(&curr_frame, &ns.wind, &ns.rev_wind, NS_PROC_FRAME_LENGTH, NS_WINDOW_LENGTH);
        prof(17, "end_windowing2");

        prof(18, "start_forming_output");
        ns_form_output(frame[0], &curr_frame, &ns.overlap);
        prof(19, "end_forming_output");
#else
        ns_process_frame(&ns, frame[0], frame[0]);
#endif

#if PROFILE_PROCESSING
        prof(3, "end_ns_process_frame");
#endif

        // Create interleaved output that can be written to wav file
        for (unsigned ch = 0; ch < MAX_CHANNELS; ch++){
            for(unsigned i = 0; i < NS_FRAME_ADVANCE; i++){
                output_write_buffer[i*(MAX_CHANNELS) + ch] = frame[ch][i];
            }
        }

        file_write(&output_file, (uint8_t*)(output_write_buffer), output_header_struct.bit_depth/8 * NS_FRAME_ADVANCE * MAX_CHANNELS);
#if PROFILE_PROCESSING
        print_prof(0, 22, b+1);
#endif
    }
    file_close(&input_file);
    file_close(&output_file);
    shutdown_session();
}

#if X86_BUILD
int main(int argc, char **argv) {
    if(argc < 3) {
        printf("Arguments missing. Expected: <input file name> <output file name>\n");
        assert(0);
    }
    ns_task(argv[1], argv[2]);
    return 0;
}
#endif