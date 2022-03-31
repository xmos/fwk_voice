# Copyright 2022 XMOS LIMITED.
# This Software is subject to the terms of the XMOS Public Licence: Version 1.
'''
The purpose of this test is to test the Nyquist bin is present. 
White noise is used as it gives a constant convergence rate.
The maximum attenuation is tested, as this should be higher with the Nyquist bin present.
'''

import os
import sys

import numpy as np
import scipy.signal as spsig
import scipy.io.wavfile
import subprocess
import shutil
from pathlib import Path

import matplotlib.pyplot as plt

from common_utils import json_to_dict
import wav_test_functions as wtf
import audio_wav_utils as awu
import run_xc

import pytest

hydra_audio_path = os.environ.get('hydra_audio_PATH', '~/hydra_audio')

def calc_max_attenuation(output):
    attenuation = output - output[0]
    max_atten = np.min(attenuation)
    print("Max attenuation is %f dB"%(max_atten))
    return max_atten

def test_nyquist():
    ''' test_nyquist - run mono white noise convolved with a modelled impulse response
    If the Nyquist is present in the AEC, the maximum attenuation should be greater than 60dB for python and 80dB for XC
        
    pass/fail: check there is at least 60dB attenuation for python and 80db for XC'''
    testname = (Path(__file__).stem)[5:]

    fs = 16000
    N = fs * 10
    np.random.seed(500)    

    y_channel_count = 1
    x_channel_count = 1

    phases = 10  # aec_parameters['phases']
    frame_advance = 240  # aec_parameters['frame_advance']
    fN = phases * frame_advance

    # load impulse response
    filename1 = "000_LAB_XTS_DUTL_fs16kHz"
    filepath = Path(hydra_audio_path, "acoustic_team_test_audio", "impulse", filename1 + ".npy")
    h1 = np.load(filepath)
    hN = h1.shape[0]
    h = h1[:,0]

    filename = "white"
    u = np.random.randn(N)

    d = spsig.convolve(u, h, 'full')[:N]
    if fN > hN:
        d = d[hN-1:hN-fN]
    else:
        d = d[hN-1:]

    d = d * 0.01 #20dB attenuation
    u = u * 0.2
    
    # ideal results
    f_ideal = h[:fN]
    y_ideal = spsig.convolve(f_ideal, u, 'full')[hN-1:N]
    _, in_leq = wtf.leq_smooth(y_ideal, fs, 0.05)

    # run AEC
    in_data = np.stack((d, u[hN-1:N]), axis=0)
    in_data_32bit = (np.asarray(in_data * np.iinfo(np.int32).max, dtype=np.int32)).T

    print("Run AEC XC")
    dut_input_file, dut_output_file = run_xc.run_aec_xc(in_data_32bit[:,:y_channel_count], in_data_32bit[:,y_channel_count:], testname, adapt_mode=run_xc.adapt_mode_dict['AEC_ADAPTION_FORCE_ON'], num_y_channels=y_channel_count, num_x_channels=x_channel_count)
    rate, output_wav_file = scipy.io.wavfile.read(dut_output_file, 'r')
    error_xc = output_wav_file[:,0]    
    _, leq_error_xc = wtf.leq_smooth(error_xc, fs, 0.05)
    max_atten_xc = wtf.calc_max_attenuation(leq_error_xc)
    print('max_atten xc =',max_atten_xc)
    assert max_atten_xc < -31, "test_nyquist fails attenuation test for XC" #Enabling frequency smoothing brings attenuation from -61dB to -52dB. python behaves similarly


if __name__ == "__main__":
    test_nyquist()
