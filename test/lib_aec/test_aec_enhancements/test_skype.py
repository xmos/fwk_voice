# Copyright 2022 XMOS LIMITED.
# This Software is subject to the terms of the XMOS Public Licence: Version 1.
import os
import sys

import numpy as np
import scipy.io.wavfile
import subprocess
import shutil
import soundfile as sf

from common_utils import json_to_dict
import wav_test_functions as wtf
import audio_wav_utils as awu
import run_xc
import pytest
from pathlib import Path

hydra_audio_path = os.environ.get('hydra_audio_PATH', '~/hydra_audio')
import configparser
parser = configparser.ConfigParser()
parser.read("parameters.cfg")
filter_dir = parser.get("Folders", "filter_dir")

@pytest.mark.parametrize("channel_count", [1, 2])
def test_skype(channel_count):
    ''' test_skype - run a  mono skype signal convolved with a modelled impulse response
    check that the output has some attenuation and AEC filter does not have any discontinuities
        
    pass/fail: check there is at least 10 dB of attenuation
    pass/fail: check the samples at frame edges are a similar magnitude to the sample in frame middle'''

    testname = f"{(Path(__file__).stem)[5:]}_{channel_count}"
    fs = 16000
    N = fs * 15

    np.random.seed(100)

    phases = 10  # aec_parameters['phases']
    frame_advance = 240  # aec_parameters['frame_advance']

    filename = "007_skype"
    filepath = Path(hydra_audio_path, "acoustic_team_test_audio", "playback_audio", filename + ".wav")
    x, fs2 = sf.read(filepath)
    x = x[:N, :channel_count]
    if x.ndim == 1:
        x = x[:, np.newaxis]
    assert fs==fs2
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
            y[:,0] += np.convolve(h1[:,n], x[:,n], 'full')[:N]
            y[:,1] += np.convolve(h2[:,n], x[:,n], 'full')[:N]
    elif channel_count > 2:
        assert False


    # run AEC
    in_data = np.concatenate((y, x), axis=1).T
    nFrames = (N-hN-1) // frame_advance -1

    #run XC
    in_data_32bit = (np.asarray(in_data * np.iinfo(np.int32).max, dtype=np.int32)).T
    print("Run AEC XC. nFrames = ", nFrames)
    filter_td_file = f"{filter_dir}/{testname}_h_td_xc.npy"
    filter_fd_file = f"{filter_dir}/{testname}_H_fd_xc.npy"
    dut_input_file, dut_output_file = run_xc.run_aec_xc(in_data_32bit[:,:channel_count], in_data_32bit[:,channel_count:], testname, adapt=nFrames, h_hat_dump=filter_fd_file, adapt_mode=run_xc.adapt_mode_dict['AEC_ADAPTION_FORCE_ON'], num_y_channels=channel_count, num_x_channels=channel_count)

    rate, output_wav_file = scipy.io.wavfile.read(dut_output_file, 'r')
    error_xc = output_wav_file[:,0]
    _, leq_error_xc = wtf.leq_smooth(error_xc, fs, 0.05)

    Hxmos = run_xc.get_h_hat(filter_fd_file, 'xc')[0,0]
    print('Hxmos.shape = ',Hxmos.shape)
    #Hxmos = np.load('skype_H_fd_xc.npy')[0,0]
    h = np.fft.irfft(Hxmos)
    hxmos_xc = np.zeros(frame_advance*phases)
    for p in range(phases):
        hxmos_xc[p*frame_advance: frame_advance*(p+1)] = h[p, :frame_advance]
    np.save(filter_td_file, hxmos_xc)
    
    print('Check XC')
    disco_res_xc = wtf.disco_check(hxmos_xc, phases, frame_advance)
    assert(disco_res_xc)
    # check for deconvergence
    assert leq_error_xc[-1] < leq_error_xc[20]


if __name__ == "__main__":
    test_skype(2)
