
// Copyright 2022 XMOS LIMITED.
// This Software is subject to the terms of the XMOS Public Licence: Version 1.
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <limits.h>

#include "fileio.h"
#include "wav_utils.h"

extern void test_init();
extern void test(int32_t *output, int32_t *input);
void test_vnr_unit(const char *input_file_name, const char *output_file_name)
{
    file_t input_file, output_file;
    // Open input wav file containing mic and ref channels of input data
    int ret = file_open(&input_file, input_file_name, "rb");
    assert((!ret) && "Failed to open file");
    // Open output wav file that will contain the AEC output
    ret = file_open(&output_file, output_file_name, "wb");
    assert((!ret) && "Failed to open file");
    
    int filesize = get_file_size(&input_file);
    printf("filesize = %d\n",filesize);
    
    int input_framesize, output_framesize;
    file_read(&input_file, &input_framesize, sizeof(int32_t)); //No. of int32 values per frame
    file_read(&input_file, &output_framesize, sizeof(int32_t)); //No. of int32 values per frame
    printf("input_framesize=%d, output_framesize=%d\n",input_framesize, output_framesize);
    
    int filesize_words = filesize/4; //No. of int32_t words in file
    int num_frames = (filesize_words - 2)/input_framesize;
    printf("num_frames = %d\n",num_frames);
    
    int32_t biggest_possible_input_frame[2048];
    int32_t biggest_possible_output_frame[512];

    test_init();
    for(int fr=0; fr<num_frames; fr++)
    {
        file_read(&input_file, biggest_possible_input_frame, input_framesize*sizeof(int32_t)); //No. of int32 values per frame
        test(biggest_possible_output_frame, biggest_possible_input_frame);
        file_write(&output_file, biggest_possible_output_frame, output_framesize*sizeof(int32_t)); //No. of int32 values per frame
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
