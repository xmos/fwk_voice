// Copyright 2022 XMOS LIMITED.
// This Software is subject to the terms of the XMOS Public Licence: Version 1.
#include <stdio.h>
#include <assert.h>

#include <agc_api.h>

#include <fileio.h>
#include <wav_utils.h>


void agc_task(const char *input_file_name, const char *output_file_name) {
    //open files
    file_t input_file, output_file;
    int ret = file_open(&input_file, input_file_name, "rb");
    assert((!ret) && "Failed to open input file");
    ret = file_open(&output_file, output_file_name, "wb");
    assert((!ret) && "Failed to open output file");

    wav_header input_header_struct, output_header_struct;
    unsigned input_header_size;

    if (get_wav_header_details(&input_file, &input_header_struct, &input_header_size) != 0) {
        printf("error in get_wav_header_details()\n");
        _Exit(1);
    }

    file_seek(&input_file, input_header_size, SEEK_SET);

    if (input_header_struct.bit_depth != 32) {
         printf("Error: unsupported wav bit depth (%d) for %s file. Only 32 supported\n", input_header_struct.bit_depth, input_file_name);
         _Exit(1);
    }

    if (input_header_struct.num_channels != 1) {
        printf("Error: wav num channels (%d) does not match expected (1)\n", input_header_struct.num_channels);
        _Exit(1);
    }

    unsigned frame_count = wav_get_num_frames(&input_header_struct);

    unsigned block_count = frame_count / AGC_FRAME_ADVANCE;
    wav_form_header(&output_header_struct,
                    input_header_struct.audio_format,
                    1,   // number of channels
                    input_header_struct.sample_rate,
                    input_header_struct.bit_depth,
                    block_count * AGC_FRAME_ADVANCE);

    file_write(&output_file, (uint8_t*)(&output_header_struct), WAV_HEADER_BYTES);

    int32_t input_read_buffer[AGC_FRAME_ADVANCE];
    int32_t output_write_buffer[AGC_FRAME_ADVANCE];
    unsigned bytes_per_frame = wav_get_num_bytes_per_frame(&input_header_struct);


    // Initialise the AGC configuration using one of the pre-defined profiles in api/agc_profiles.h, and then
    // make any alterations to the defaults. In this application, there is no VNR, so adapt_on_vnr must be
    // disabled.
    agc_config_t conf = AGC_PROFILE_ASR;
    conf.adapt_on_vnr = 0;

    agc_state_t agc;
    agc_init(&agc, &conf);

    // Initialise the meta-data. Since this application has neither VNR nor AEC, the meta-data will be
    // constant and can use these pre-defined values to make clear the absence of VNR and AEC.
    agc_meta_data_t md = {AGC_META_DATA_NO_VNR, AGC_META_DATA_NO_AEC, AGC_META_DATA_NO_AEC};

    for (unsigned bl = 0; bl < block_count; ++bl) {
        long input_location =  wav_get_frame_start(&input_header_struct, bl * AGC_FRAME_ADVANCE, input_header_size);
        file_seek (&input_file, input_location, SEEK_SET);
        file_read (&input_file, (uint8_t *)input_read_buffer, bytes_per_frame * AGC_FRAME_ADVANCE);

        // Call the AGC function to process the input frame, writing the output into the output buffer.
        agc_process_frame(&agc, output_write_buffer, input_read_buffer, &md);

        file_write(&output_file, (uint8_t *)output_write_buffer, bytes_per_frame * AGC_FRAME_ADVANCE);
    }

    file_close(&input_file);
    file_close(&output_file);
    shutdown_session();
}


#if X86_BUILD
int main(int argc, char **argv) {
    if (argc < 3) {
        printf("Arguments missing. Expected: <input file name> <output file name>\n");
        assert(0);
    }
    agc_task(argv[1], argv[2]);
    return 0;
}
#endif
