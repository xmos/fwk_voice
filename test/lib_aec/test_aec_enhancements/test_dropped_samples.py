# Copyright 2022 XMOS LIMITED.
# This Software is subject to the terms of the XMOS Public Licence: Version 1.

'''
The purpose of these test is to check reconvergence behaviour after samples have been dropped from the reference audio
A number of samples are removed from the reference causing the filter delay to become incorrect, requiring readaptation.
'''

import os
from pathlib import Path

hydra_audio_path = os.environ.get('hydra_audio_PATH', '~/hydra_audio')

import numpy as np
import scipy.signal as spsig
import scipy.io.wavfile

import soundfile as sf
import audio_wav_utils as awu

from common_utils import json_to_dict
import wav_test_functions as wtf
import run_xc

import pytest

@pytest.mark.parametrize("drop_amount", [1, 3, 10])
@pytest.mark.parametrize("room", ["lab", "board"])

def test_dropped_samples(drop_amount, room):
    ''' test_dropped_samples - run a  mono pink noise convolved with a modelled impulse response
    remove drop_amount samples after 10 seconds, and check for the length of time taken for 10 dB reconvergence
        
    pass/fail: check it takes less than 0.75 seconds for 10 dB reconvergence after the dropped samples'''

    np.random.seed(42)
    fs = 16000
    N = fs * 20
    testname = f"{(Path(__file__).stem)[5:]}_{drop_amount}_{room}"

    y_channel_count = 1
    x_channel_count = 1

    phases = 10  # aec_parameters['phases']
    frame_advance = 240  # aec_parameters['frame_advance']
    fN = phases * frame_advance

    # load impulse response
    if room == "lab":
        filename1 = "000_LAB_XTS_DUTL_fs16kHz"
    elif room == "board":
        filename1 = "006_BOARD_XTS_DUTL_fs16kHz"
    else:
        assert False

    filepath = Path(hydra_audio_path, "acoustic_team_test_audio", "impulse", filename1 + ".npy")
    h1 = np.load(filepath)
    hN = h1.shape[0]
    h = h1[:,0]

    filename = "003_rick_mono"
    filepath = Path(hydra_audio_path, "acoustic_team_test_audio", "playback_audio", filename + ".wav")
    u, fs3 = sf.read(filepath)
    u = u[:,0]
    assert fs==fs3
  
    d = spsig.convolve(u, h, 'full')[:N]
    if fN > hN:
        d = d[hN-1:hN-fN]
    else:
        d = d[hN-1:N]

    # ideal results
    f_ideal = h[:fN]
    y_ideal = spsig.convolve(f_ideal, u, 'full')[hN-1:N]
    _, in_leq = wtf.leq_smooth(y_ideal, fs, 0.05)

    # set the dropped samples
    decim_ratio = 3
    fs2 = decim_ratio*fs    
    drop_start = 10*fs2
    drop_stop = 15*fs2
    drop_rate = int(5*fs2)
    
    # upsample, set dropped to nan, remove and downsample
    u2 = spsig.resample_poly(u[hN-1:], decim_ratio, 1)
    for n in range(drop_amount):
        u2[(drop_start+n):(drop_stop+n):drop_rate] = np.nan
    u2 = u2[~np.isnan(u2)]
    u = spsig.resample_poly(u2, 1, decim_ratio)

    # run AEC
    #XC expects 4ch input
    in_data = np.stack((d, u[:N-hN+1]), axis=0) 
    in_data_32bit = (np.asarray(in_data * np.iinfo(np.int32).max, dtype=np.int32)).T
    nFrames = (N-hN-1) // frame_advance -1

    #run XC
    print("Run AEC XC")
    dut_input_file, dut_output_file = run_xc.run_aec_xc(in_data_32bit[:,:y_channel_count], in_data_32bit[:,y_channel_count:], testname, adapt_mode=run_xc.adapt_mode_dict['AEC_ADAPTION_AUTO'], num_y_channels=y_channel_count, num_x_channels=x_channel_count)
    rate, output_wav_file = scipy.io.wavfile.read(dut_output_file, 'r')
    error = output_wav_file 
    _, leq_error = wtf.leq_smooth(error[:, 0], fs, 0.05)
    time = np.arange(len(leq_error))*0.05
    # find max deconvergence point
    drop_idx = np.searchsorted(time, drop_start/fs2)
    drop_idx = np.argmax(leq_error[drop_idx-10:drop_idx+10]) + (drop_idx-10)

    # calculate reconvergence time
    reconv_time = wtf.calc_attenuation_time(time[drop_idx:], leq_error[drop_idx:], -10) - time[drop_idx]
    print("XC: reconv_time: %.2f seconds"%reconv_time)
    assert reconv_time < 3, "XC reconv_time error"

    # plot
    if __name__ == "__main__":
        import matplotlib.pyplot as plt
        plt.figure()
        plt.title("%s input signal, drop %d samples @ %d kHz at %.2f seconds"%(filename, drop_amount, fs2/1000, drop_start/fs2 ))
        plt.plot(time, leq_error - leq_error[0])
        plt.xlabel("Time (s)")
        plt.ylabel("Attenuation (dB)")
        plt.ylim([-40, 10])
        plt.xlim([0, time[-1]])
        #plt.show()


if __name__ == "__main__":
    test_dropped_samples(10, "lab")
