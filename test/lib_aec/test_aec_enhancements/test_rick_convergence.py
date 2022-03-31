# Copyright 2022 XMOS LIMITED.
# This Software is subject to the terms of the XMOS Public Licence: Version 1.

'''
The purpose of this test is to test the initial convergence behaviour. 
Pink noise is used as it gives a constant convergence rate.
The initial attenuation rate and maximum attenuation are tested.
The test is carried out with fixed and variable mu, which may indicate the cause of any convergence issues. 
If the test fails with fixed mu it is an indication that something may be wrong with the normalisation. 
If the test fails with variable mu, there may be a problem with the mu.
'''

import os
from pathlib import Path

hydra_audio_path = os.environ.get('hydra_audio_PATH', '~/hydra_audio')

import numpy as np
import scipy.io.wavfile

import soundfile as sf
import audio_wav_utils as awu

from common_utils import json_to_dict
import wav_test_functions as wtf
import run_xc
import configparser
parser = configparser.ConfigParser()
parser.read("parameters.cfg")
filter_dir = parser.get("Folders", "filter_dir")

import pytest


@pytest.mark.parametrize("adapt_config", ['AEC_ADAPTION_FORCE_ON', 'AEC_ADAPTION_AUTO'])
@pytest.mark.parametrize("channel_count", [1, 2])
def test_pink_convergence(adapt_config, channel_count):
    ''' test_pink_convergence - run mono/stereo pink noise convolved with a modelled impulse response
    check that the output has some attenuation and AEC filter does not have any discontinuities 
    and converges quickly, with and without a variable mu.
        
    pass/fail: check there is at least 10 dB of attenuation
    pass/fail: check the samples at frame edges are a similar magnitude to the sample in frame middle
    pass/fail: check the convergence rate over the first 2 seconds is greater than 10 dB/s
    pass/fail: check there is at least 35 dB maximum attenuation'''

    fs = 16000
    N = fs * 10
    testname = f"{(Path(__file__).stem)[5:]}_{adapt_config}_{channel_count}"

    phases = 10  # aec_parameters['phases']
    frame_advance = 240  # aec_parameters['frame_advance']
    fN = phases * frame_advance

    filename = "003_rick_mono"
    filepath = Path(hydra_audio_path, "acoustic_team_test_audio", "playback_audio", filename + ".wav")
    x, fs2 = sf.read(filepath)
    assert fs==fs2
    if x.ndim == 1:
        x = x[:, np.newaxis]
    if x.shape[1] < channel_count:
        # if we have a mono signal, take different time slices for different channels
        if x.shape[1] == 1:
            xo = x[:N]
            for ch in range(1, channel_count):
                xo = np.concatenate((xo[:N, :], x[ch*N:(ch+1)*N]), axis=1)
            x = xo
        else:
            x = np.tile(x, (1, channel_count // x.shape[1] + 1))
    x = x[:N, :channel_count]
    y = np.zeros((N, channel_count)) # microphone signal

    # load impulse response
    filename1 = "000_LAB_XTS_DUTL_fs16kHz"
    filename2 = "001_LAB_XTS_DUTR_fs16kHz"

    filepath = Path(hydra_audio_path, "acoustic_team_test_audio", "impulse", filename1 + ".npy")
    h1 = np.load(filepath)
    hN = h1.shape[0]

    if channel_count == 1:
        h = h1[:, 0]
        y = np.atleast_2d(np.convolve(h, x[:,0], 'full')[:N]).T
    if channel_count == 2:
        filepath = Path(hydra_audio_path, "acoustic_team_test_audio", "impulse", filename2 + ".npy")
        h2 = np.load(filepath)
        for n in range(channel_count):
            y[:,0] += np.convolve(h1[:,n],x[:, n], 'full')[:N]
            y[:,1] += np.convolve(h2[:,n],x[:, n], 'full')[:N]
    elif channel_count > 2:
        assert False

    # run AEC
    in_data = np.concatenate((y, x), axis=1).T
    nFrames = (N-hN-1) // frame_advance -1
    in_data_32bit = (np.asarray(in_data * np.iinfo(np.int32).max, dtype=np.int32)).T
    
    #run XC
    print("Run AEC XC")
    filter_td_file = f"{filter_dir}/{testname}_h_td_xc.npy"
    filter_fd_file = f"{filter_dir}/{testname}_H_fd_xc.npy"
    dut_input_file, dut_output_file = run_xc.run_aec_xc(in_data_32bit[:,:channel_count], in_data_32bit[:,channel_count:], f"{testname}", adapt=nFrames, h_hat_dump=filter_fd_file, adapt_mode=run_xc.adapt_mode_dict[adapt_config], num_y_channels=channel_count, num_x_channels=channel_count)
    rate, output_wav_file = scipy.io.wavfile.read(dut_output_file, 'r')
    error = output_wav_file 
    _, leq_error = wtf.leq_smooth(error[:, 0], fs, 0.05)
    time = np.arange(len(leq_error))*0.05
    Hxmos = run_xc.get_h_hat(filter_fd_file, 'xc')[0,0]
    print('Hxmos.shape = ',Hxmos.shape)
    h = np.fft.irfft(Hxmos)
    hxmos = np.zeros(frame_advance*phases)
    for p in range(phases):
        hxmos[p*frame_advance: frame_advance*(p+1)] = h[p, :frame_advance]
    np.save(filter_td_file, hxmos)

    disco_res = wtf.disco_check(hxmos, phases, frame_advance)
    convergence_rate = wtf.calc_convergence_rate(time, leq_error)
    time_20dB = wtf.calc_attenuation_time(time, leq_error, -20)
    time_30dB = wtf.calc_attenuation_time(time, leq_error, -30)
    time_40dB = wtf.calc_attenuation_time(time, leq_error, -40)
    max_atten = wtf.calc_max_attenuation(leq_error)
    assert disco_res
    assert max_atten < -40
    assert convergence_rate > 12

    # plot
    if __name__ == "__main__":
        import matplotlib.pyplot as plt
        plt.figure()
        plt.title("%s input signal, %s"%(filename, adapt_config))
        plt.plot(time, leq_error - leq_error[0])
        plt.xlabel("Time (s)")
        plt.ylabel("Attenuation (dB)")
        plt.ylim([-40, 10])
        plt.xlim([0, time[-1]])
        #plt.show()


if __name__ == "__main__":
    test_pink_convergence('AEC_ADAPTION_AUTO', 1)
