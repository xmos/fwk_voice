// Copyright 2023 XMOS LIMITED.
// This Software is subject to the terms of the XMOS Public Licence: Version 1.

#ifndef _MEL_SPEC_SETTINGS_H
#define _MEL_SPEC_SETTINGS_H

/* Values here may be changed, with caution:

   - The SCALE and ZERO_POINT values control the quantisation process and 
   may be freely changed.
   - The MIN_LOG10_IN and DB_DYNAMIC_RANGE values both pertain to the dB
   conversion process; values below MIN_LOG10_IN will be set to MIN_LOG10_IN
   before log10 is taken. Output values less than max(output) - DB_DYNAMIC_RANGE
   will be set to max(output) - DB_DYNAMIC_RANGE. These options, and their
   values, are set to mimic the default behaviour of librosa.power_to_db in
   Python.
   - PRE_DB_OFFSET sets a controllable scalar offset applied to the entire 
   matrix prior to passing through dB conversion. Note that this is applied 
   before the use of MIN_LOG10_IN.
   - TOP_DIM_SZ, LOW_DIM_SZ, and FRAME_DIM_SZ control the size of the expected 
   output array. The x_melspectrogram function expects an output vector of 
   int8_t[TOP_DIM_SZ][N_MEL][FRAME_DIM_SZ][LOW_DIM_SZ]. This will be 
   memset to 0 at the start of execution. The 0th elements of the top and low
   dims will be populated, such that out[0][:][:][0] will always contain data.
   All other values out[1:][:][:][1:] will be 0.
   - TRIM_START and TRIM_END control "trimming" the output frame count to fit
   into the main output buffer. Trimmed frames output to the out_x_trim buffers.
   NB: It is essential that TRIM_START + TRIM_END + FRAME_DIM_SZ == N_FRAMES.
   - HOP controls the hop size used. This parameter may be freely changed. 
   It is usual that this value be <= N_FFT, otherwise there will be samples
   in the input left unprocessed.
   - N_FRAMES sets a fixed number of output frames. This number may be
   freely changed. It is used in controlling the length of the overall 
   spectrogram, i.e. how many hops are taken. 
   - N_SAMPLES controls the expected size of the input vector, and may be freely
   changed. 
   - N_MEL controls the number of Mel bands the output will be formed of. 
   If this value is changed, a new set of Mel filterbanks will need to be
   generated using the provided gen_mel_filters.py and fed into call to 
   x_mel_spec. This value is also used in the expected size of the output.
   - N_FFT controls the width of the FFT window used. If this value is
   changed, new Mel filterbanks will need to be generated. New Hanning window
   coefficients will also need to be generated using the provided 
   gen_hann_windows.py and fed into the call to x_mel_spec. This value should
   always be a power of 2. 
   - CENTRE and PAD_MODE control the location of frames in the input stream. If
   CENTRE is false, the first sample of frame t will be at y[t * hop]. In this 
   case, if N_SAMPLES is less than (N_FRAMES * HOP) + N_FFT, the input will be
   0-padded to reach this length. This means that the final frames of the
   output may be half-padded or may be purely 0 data. If CENTRE
   is true, then the centre sample of frame t will be at y[t * hop]. In this 
   case, additional samples are added at the start and end of the input stream
   to "centre" it. How these samples are added may be configured using members
   of the enum mel_spec_pad_mode_t: MEL_SPEC_PAD_MODE_CONSTANT zero pads the 
   input stream, while _REFLECT and _SYMMETRIC reflect the array around its 
   first and last elements, as below:
   CONSTANT:  [1, 2, 3, 4, 5] -> [0, 0, 0, 0, 0, |1, 2, 3, 4, 5|, 0, 0, 0, 0, 0]
   REFLECT:   [1, 2, 3, 4, 5] -> [4, 5, 4, 3, 2, |1, 2, 3, 4, 5|, 4, 3, 2, 1, 2]
   SYMMETRIC: [1, 2, 3, 4, 5] -> [5, 4, 3, 2, 1, |1, 2, 3, 4, 5|, 5, 4, 3, 2, 1]

*/

#define MEL_SPEC_COMMON_TOP_DIM_SZ       1
#define MEL_SPEC_COMMON_LOW_DIM_SZ       4
#define MEL_SPEC_COMMON_MIN_LOG10_IN     1e-10
#define MEL_SPEC_COMMON_DB_DYNAMIC_RANGE 80
#define MEL_SPEC_COMMON_CENTRE           true
#define MEL_SPEC_COMMON_PAD_MODE         MEL_SPEC_PAD_MODE_REFLECT

#define MEL_SPEC_SMALL_N_FFT             512
#define MEL_SPEC_SMALL_N_SAMPLES         6400
#define MEL_SPEC_SMALL_N_MEL             64
#define MEL_SPEC_SMALL_N_FRAMES          26
#define MEL_SPEC_SMALL_HOP               256
#define MEL_SPEC_SMALL_SCALE             3.325621485710144e-1
#define MEL_SPEC_SMALL_ZERO_POINT        12
#define MEL_SPEC_SMALL_PRE_DB_OFFSET     0
#define MEL_SPEC_SMALL_TRIM_START        0
#define MEL_SPEC_SMALL_TRIM_END          0
#define MEL_SPEC_SMALL_FRAME_DIM_SZ      MEL_SPEC_SMALL_N_FRAMES
#define MEL_SPEC_SMALL_TOP_DIM_SZ        MEL_SPEC_COMMON_TOP_DIM_SZ
#define MEL_SPEC_SMALL_LOW_DIM_SZ        MEL_SPEC_COMMON_LOW_DIM_SZ
#define MEL_SPEC_SMALL_MIN_LOG10_IN      MEL_SPEC_COMMON_MIN_LOG10_IN
#define MEL_SPEC_SMALL_DB_DYNAMIC_RANGE  MEL_SPEC_COMMON_DB_DYNAMIC_RANGE
#define MEL_SPEC_SMALL_CENTRE            MEL_SPEC_COMMON_CENTRE
#define MEL_SPEC_SMALL_PAD_MODE          MEL_SPEC_COMMON_PAD_MODE

#define MEL_SPEC_LARGE_N_FFT             1024
#define MEL_SPEC_LARGE_N_SAMPLES         84800
#define MEL_SPEC_LARGE_N_MEL             128
#define MEL_SPEC_LARGE_N_FRAMES          166
#define MEL_SPEC_LARGE_HOP               512
#define MEL_SPEC_LARGE_SCALE             3.921553026884794e-3
#define MEL_SPEC_LARGE_ZERO_POINT        128
#define MEL_SPEC_LARGE_PRE_DB_OFFSET     1e-8
#define MEL_SPEC_LARGE_TRIM_START        4
#define MEL_SPEC_LARGE_TRIM_END          4
#define MEL_SPEC_LARGE_FRAME_DIM_SZ      158
#define MEL_SPEC_LARGE_TOP_DIM_SZ        MEL_SPEC_COMMON_TOP_DIM_SZ
#define MEL_SPEC_LARGE_LOW_DIM_SZ        MEL_SPEC_COMMON_LOW_DIM_SZ
#define MEL_SPEC_LARGE_MIN_LOG10_IN      MEL_SPEC_COMMON_MIN_LOG10_IN
#define MEL_SPEC_LARGE_DB_DYNAMIC_RANGE  MEL_SPEC_COMMON_DB_DYNAMIC_RANGE
#define MEL_SPEC_LARGE_CENTRE            MEL_SPEC_COMMON_CENTRE
#define MEL_SPEC_LARGE_PAD_MODE          MEL_SPEC_COMMON_PAD_MODE

#endif // _MEL_SPEC_SETTINGS_H
