# Copyright 2022 XMOS LIMITED.
# This Software is subject to the terms of the XMOS Public Licence: Version 1.

''' impulse response change checker
this test is intended to check the ability of the
shadow filter to detect a change in the impulse response 
and speed up filter adaptation accordingly

some mono white noise convolved with a modelled impulse response is run
a change in the impulse response happens midway through the signal
the convergence time and total attenuation is monitored after change
        
pass/fail: check if the convergence rate is at least 7 dB/s

'''
import os
from pathlib import Path

hydra_audio_path = os.environ.get('hydra_audio_PATH', '~/hydra_audio')

import numpy as np
import scipy.io.wavfile
import scipy.signal as spsig
import soundfile as sf
import audio_wav_utils as awu

from common_utils import json_to_dict
import wav_test_functions as wtf
import run_xc

import pytest

def conv_impulse_array(x, h, fade_len):
    n_impulses = len(h)
    N = len(x)

    y = [0]*n_impulses
    for n in range(n_impulses):
        y[n] = spsig.convolve(x, h[n], 'full')[:N]

    y_len = len(y[0])
    y_out = np.zeros_like(y[0])
    sec_l = y_len//n_impulses
    for n in range(n_impulses):
        if n > 0:
            y[n][:n*sec_l - fade_len//2] = 0.0
            y[n][n*sec_l - fade_len//2:n*sec_l + fade_len//2] *= np.arange(fade_len)/fade_len
        
        if n < n_impulses - 1:
            y[n][(n+1)*sec_l + fade_len//2:] = 0.0
            y[n][(n+1)*sec_l - fade_len//2:(n+1)*sec_l + fade_len//2] *= np.flip(np.arange(fade_len)/fade_len)

        y_out += y[n]
    
    return y_out, y
   

@pytest.mark.parametrize("adapt_config", ['AEC_ADAPTION_FORCE_ON', 'AEC_ADAPTION_AUTO'])
def test_impulse_response_change(adapt_config):

    fs = 16000
    N = fs * 20
    np.random.seed(500)  
    testname = f"{(Path(__file__).stem)[5:]}_{adapt_config}"

    y_channel_count = 1
    x_channel_count = 1

    phases = 10  # aec_parameters['phases']
    frame_advance = 240  # aec_parameters['frame_advance']
    fN = phases * frame_advance

    # load impulse response
    filename1 = "000_LAB_XTS_DUTL_fs16kHz"
    filepath = Path(hydra_audio_path, "acoustic_team_test_audio", "impulse", filename1 + ".npy")
    h1 = np.load(filepath)

    n_impulses = 2
    fade_len = int(0.0*fs)
    h = [0]*n_impulses
    for n in range(n_impulses):
        h[n] = h1[:,n]
    hN = len(h[0])
    fN = 10 * 240

    # filename = "006_Pink"
    # filepath = Path(hydra_audio_path, "acoustic_team_test_audio", "point_noise", filename + ".wav")
    # u, fs2 = sf.read(filepath)
    # u = u[:N]
    # assert fs==fs2
  
    # filename = "white"
    u = np.random.randn(N)
    
    if u.ndim == 1:
        u = u[:, np.newaxis]    
    
    if u.shape[0] < N:
        u = np.tile(u, (N // u.shape[0] + 1, 1))

    u = u[:N, 0]

    d, _ = conv_impulse_array(u, h, fade_len)

    if fN > hN:
        d = d[hN-1:hN-fN]
    else:
        d = d[hN-1:]

    d = d * 0.01 #20dB attenuation
    u = u * 0.2
    # run AEC
    in_data = np.stack((d, u[hN-1:N]), axis=0)
    in_data_32bit = (np.asarray(in_data * np.iinfo(np.int32).max, dtype=np.int32)).T
    
    #run XC
    print("Run AEC XC")
    dut_input_file, dut_output_file = run_xc.run_aec_xc(in_data_32bit[:,:y_channel_count], in_data_32bit[:,y_channel_count:], testname, adapt_mode=run_xc.adapt_mode_dict[adapt_config], num_y_channels=y_channel_count, num_x_channels=x_channel_count)

    rate, output_wav_file = scipy.io.wavfile.read(dut_output_file, 'r')
    error_xc = output_wav_file[:,0] 
    _, leq_error = wtf.leq_smooth(error_xc, fs, 0.05)
    change_index, = np.where(leq_error == leq_error.max())
    leq_e = leq_error[int(change_index):]
    t = np.arange(len(leq_e))*0.05
    reconvergence_rate = wtf.calc_convergence_rate(t, leq_e)
    print(f"XC reconvergence_rate: {reconvergence_rate}")
    # test    
    assert reconvergence_rate > 12,"XC reconvergence_rate error"

    # plot
    if __name__ == "__main__":
        import matplotlib.pyplot as plt
        plt.figure()
        plt.title("white noise input signal,impulse response change @ %d seconds"%(N//fs//2))
        plt.plot(time, leq_error - leq_error[0])
        plt.xlabel("Time (s)")
        plt.ylabel("Attenuation (dB)")
        plt.ylim([-50, 10])
        plt.xlim([0, time[-1]])
        #plt.show()
        


if __name__ == "__main__":
    test_impulse_response_change('AEC_ADAPTION_AUTO')
