# Copyright 2022 XMOS LIMITED.
# This Software is subject to the terms of the XMOS Public Licence: Version 1.

import numpy as np
import scipy.io.wavfile
import audio_wav_utils as awu
import sys
import json
import re

# Grab a python config from a JSON file
def json_to_dict(config_file):
    datastore = None
    with open(config_file, "r") as f:
        input_str = f.read()
        # Remove '//' comments
        json_str = re.sub(r'//.*\n', '\n', input_str)
        datastore = json.loads(json_str)
    return datastore

# Turn a float32 from C into an np float scalar
def float_s32_to_float(float_s32):
    return np.ldexp(float_s32.mant, float_s32.exp)

# turn an int32 np array (Q1.31) into float
def int32_to_float(array_int32):
    array_float = np.array(array_int32).astype(np.float64) / (2**31)
    return array_float

# turn an uint8 np array (Q0.8) into float
def uint8_to_float(array_uint8):
    array_float = np.array(array_uint8).astype(np.float64) / (2**8)
    return array_float

# turn a float into an int32 np array (Q1.31)
def float_to_int32(array_float):
    array_int32 = np.clip((np.array(array_float) * (2**31)), np.iinfo(np.int32).min, np.iinfo(np.uint32).max).astype(np.int32)
    return array_int32

# turn a float np array into uint8 (Q0.8)
def float_to_uint8(array_float):
    array_uint8 = np.clip((np.array(array_float) * (2**8)), np.iinfo(np.uint8).min, np.iinfo(np.uint8).max).astype(np.uint8)
    return array_uint8


# compare a two channel wav file and quantify how close they are
# Any file that is 1 sample out in delay will show low results of 0.20 or worse
# Arithmetic closeness is more sensitive than geo_closeness
# Anything in the 0.90 region or more is extremely close indeed
def pcm_closeness_metric(input_file, verbose=True):
    input_rate, input_wav_file = scipy.io.wavfile.read(input_file, 'r')
    input_wav_data, input_channel_count, file_length = awu.parse_audio(input_wav_file)
    assert input_channel_count == 2, f"This function works on a 2 channel file only, you supplied {input_channel_count}.."

    dtype = type(input_wav_data[0][0])
    ch_0 = input_wav_data[0]
    ch_1 = input_wav_data[1]

    #Extract a section from the middle and do full cross correlation to estimate delay
    num_samples_to_correlate = 16000
    if num_samples_to_correlate > file_length:
        if verbose:
            print(f"Warning - insufficient samples {file_length} to estimate delay. Need {num_samples_to_correlate}.", file=sys.stderr)
        c_delay = None
        peak2ave = None
    else:
        ch_0_extract = ch_0[file_length//2 - num_samples_to_correlate//2 : file_length//2 + num_samples_to_correlate//2]
        ch_1_extract = ch_1[file_length//2 - num_samples_to_correlate//2 : file_length//2 + num_samples_to_correlate//2]
        correlation = np.correlate(ch_0_extract, ch_1_extract, "same")#same pads to get out size = input size
        argmax = np.argmax(np.abs(correlation))
        peak = np.abs(correlation[argmax])
        average = np.mean(np.abs(correlation))
        peak2ave = peak / average
        c_delay = num_samples_to_correlate // 2 - argmax #delay relative to channel 0 (python)
        if verbose:
            print(f"C stream delay samples: {c_delay}, pk2ave: {peak2ave:.2f}")

    #Calculate arithmetic closeness - normalised absolute diff between all samples
    diff = np.fabs(np.subtract(ch_0, ch_1))
    mean_diff = np.mean(diff)
    normalisation = np.mean(np.fabs(ch_0))
    arith_closeness = 1 - mean_diff/normalisation if normalisation > mean_diff else 0 #clamp to 0 if negative
    if verbose:
        print(f"arithcloseness: {100*arith_closeness:.2f}%")

    #Calculate geomertric difference by correlating (dot product of two arrays)
    corr = np.correlate(ch_0, ch_1, "valid")[0] #full or same takes a LONG time for large signals
    a_corr_0 = np.correlate(ch_0, ch_0, "valid")[0]
    a_corr_1 = np.correlate(ch_1, ch_1, "valid")[0]
    geo_closeness =  corr / a_corr_0 if a_corr_0 > a_corr_1 else corr / a_corr_1
    if verbose:
        print(f"geocloseness: {100*geo_closeness:.2f}%")

    return arith_closeness, geo_closeness, c_delay, peak2ave


# Draw a simple graph
def basic_line_graph(name, data):
    import matplotlib.pyplot as plt

    markersize = 2
    plt.clf()
    if np.mean(data[:,0]) > np.mean(data[:,1]): #make sure larger values are behind so we can see smaller at front
        plt.plot(data[:,0], label="Python", linestyle="", marker=".", markersize=markersize, color="orange")
        plt.plot(data[:,1], label="Avona", linestyle="", marker=".", markersize=markersize, color="blue")
    else:
        plt.plot(data[:,1], label="Python", linestyle="", marker=".", markersize=markersize, color="orange")
        plt.plot(data[:,0], label="Avona", linestyle="", marker=".", markersize=markersize, color="blue")
    plt.title(name)
    plt.legend()
    filename = f'{name}.png'
    plt.savefig(filename, bbox_inches='tight', dpi=600)
    return filename
