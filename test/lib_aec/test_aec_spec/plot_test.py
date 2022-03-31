# Copyright 2022 XMOS LIMITED.
# This Software is subject to the terms of the XMOS Public Licence: Version 1.
import matplotlib
import matplotlib.pyplot as plt
import numpy as np
from aec_test_utils import read_wav, get_h_hat_impulse_response
from audio_generation import get_filenames, get_h


def plot(test_id, audio_in, audio_ref, audio_out, start_time, end_time,
         output_filename, sample_rate=16000):
    start = start_time * sample_rate
    end = end_time * sample_rate
    plt.figure(figsize=(12,10))

    plt.subplot(221)
    plt.title("Spectrogram of AudioOut")
    plt.specgram(audio_out, Fs=sample_rate, scale='dB')
    plt.ylabel("Frequency (Hz)")
    plt.xlabel("Time (s)")

    plt.subplot(222)
    plt.title("FFT of AudioOut[%ds:%ds]" % (start_time, end_time))
    plt.magnitude_spectrum(audio_out[start:end], Fs=sample_rate,
                           scale='dB')

    plt.subplot(223)
    plt.title("FFT of Reference[%ds:%ds]" % (start_time, end_time))
    plt.magnitude_spectrum(audio_ref[start:end], Fs=sample_rate,
                           scale='dB')

    plt.subplot(224)
    plt.title("FFT of AudioIn[%ds:%ds]" % (start_time, end_time))
    plt.magnitude_spectrum(audio_in[start:end], Fs=sample_rate,
                           scale='dB')

    plt.suptitle("%s (Test, Echo, Reference, Headroom Bits)"\
                 % test_id)
    plt.tight_layout()
    plt.subplots_adjust(top=0.9)
    plt.savefig(output_filename)
    plt.close()


def plot_impulseresponse(test_id, audio_out, echo_type, h_hat_ir,
                         headroom, start_time, output_filename,
                         sample_rate=16000):
    start = start_time * sample_rate
    end = int((start_time + 0.3) * sample_rate)
    N = end-start
    x = np.linspace(0, 1000 * N / sample_rate, N)
    plt.figure(figsize=(12,8))

    plt.suptitle("%s (Test, Echo, Reference, Headroom Bits)"\
                 % test_id)
    plt.subplot(131)
    ylim = np.max(np.abs(audio_out[start+50:end])) * 1.1
    plt.ylim(-ylim, ylim)
    plt.title("AEC Output")
    plt.ylabel("Amplitude")
    plt.xlabel("ms")
    plt.plot(x, audio_out[start:end])

    plt.subplot(132)
    plt.title("h_hat internal")
    plt.ylabel("Amplitude")
    plt.xlabel("ms")
    plt.plot(x, np.pad(h_hat_ir, (0,abs(N-len(h_hat_ir))), 'constant')[:N])

    plt.subplot(133)
    plt.title("h_hat external")
    plt.ylabel("Amplitude")
    plt.xlabel("ms")
    echo = get_h(echo_type, normalise=False)
    plt.plot(x, np.pad(echo, (0,abs(N-len(echo))), 'constant')[:N])

    plt.tight_layout()
    plt.subplots_adjust(top=0.9)
    plt.savefig(output_filename)
    plt.close()


def plot_test(plot_filename, test_id, in_filename, ref_filename, out_filename,
              settle_time):
    print("Plot Filename: %s" % plot_filename)
    in_data = read_wav(in_filename)
    ref_data = read_wav(ref_filename)
    out_data = read_wav(out_filename)[:,0]
    # TODO: Read start/end times from config
    start_time = settle_time
    end_time = settle_time + 1
    plot(test_id, in_data, ref_data, out_data, start_time, end_time,
         plot_filename)


def plot_impulseresponse_test(plot_filename, test_id, echo_type, h_hat_ir,
                              headroom, out_filename, settle_time):
    print("Plot Filename: %s" % plot_filename)
    out_data = read_wav(out_filename)[:,0]
    # TODO: Read start/end times from config
    start_time = settle_time + 1
    plot_impulseresponse(test_id, out_data, echo_type, h_hat_ir, int(headroom),
                         start_time, plot_filename)


def plot_h_hat(h_hat, y_channel, x_channel):
    """Plots the impulse response of h_hat.

    h_hat is an array internal to the aec with a shape as follows:
    (y_channel_count, x_channel_count, max_phase_count, f_bin_count)

    Args:
        h_hat: h_hat array
        y_channel: y_channel to plot
        x_channel: x_channel to plot

    Returns:
        None
    """
    h_hat_ir = get_h_hat_impulse_response(h_hat, y_channel, x_channel)
    plt.plot(h_hat_ir)
    plt.show()
