# Copyright 2022 XMOS LIMITED.
# This Software is subject to the terms of the XMOS Public Licence: Version 1.
from builtins import str
from builtins import range
import sys
import os

package_dir = os.path.dirname(os.path.abspath(__file__))

from audio_generation import get_band_limited_noise, write_data
import subprocess
import time
import numpy as np
import scipy.io.wavfile as wavfile
import audio_wav_utils as awu
import argparse
import shutil
import glob
import configparser
import xtagctl
import xscope_fileio

parser = configparser.ConfigParser()
parser.read("config.cfg")
c_ns_xe = os.path.abspath(glob.glob(f'{parser.get("Binaries", "c_path")}/bin/*.xe')[0])
xc_ns_xe = os.path.abspath(glob.glob(f'{parser.get("Binaries", "xc_path")}/bin/*.xe')[0])
print(os.path.abspath(c_ns_xe))
print(os.path.abspath(xc_ns_xe))

X_CH_COUNT = 0
Y_CH_COUNT = 1

SAMPLE_RATE = 16000
SAMPLE_COUNT = 160080


def generate_test_audio(filename, audio_dir, max_freq, db=-20, samples=SAMPLE_COUNT):
    try:
        os.makedirs(audio_dir)
    except os.error as e:
        pass
    noise = get_band_limited_noise(0, max_freq, samples=samples, db=db, sample_rate=SAMPLE_RATE)
    write_data(noise, os.path.join(audio_dir, filename), sample_rate=SAMPLE_RATE)


def process_xe(filemane, output_file, audio_dir, ns_xe):

    prev_path = os.getcwd()
    os.chdir(audio_dir)

    with xtagctl.acquire("XCORE-AI-EXPLORER") as adapter_id:
        xscope_fileio.run_on_target(adapter_id, ns_xe)

    old_file = os.path.join(prev_path, audio_dir, 'output.wav')
    new_file = os.path.join(prev_path, audio_dir, output_file)
    os.rename(old_file, new_file)

    os.chdir(prev_path)  


def get_attenuation(input_file, output_file, audio_dir="."):
    in_rate, in_wav_file = wavfile.read(os.path.join(audio_dir, input_file))
    out_rate, out_wav_file = wavfile.read(os.path.join(audio_dir, output_file))

    in_wav_data, in_channel_count, in_file_length = awu.parse_audio(in_wav_file)
    out_wav_data, out_channel_count, out_file_length = awu.parse_audio(out_wav_file)

    # Calculate EWM of audio power in 1s window
    in_power = np.power(in_wav_data[0, :], 2)
    out_power = np.power(out_wav_data[0, :], 2)

    attenuation = []

    for i in range(int(len(in_power)/SAMPLE_RATE)):
        window_start = i * SAMPLE_RATE
        window_end = window_start + SAMPLE_RATE
        av_in_power = np.mean(in_power[window_start:window_end])
        av_out_power = np.mean(out_power[window_start:window_end])
        new_atten = 10 * np.log10(av_in_power / av_out_power)
        attenuation.append(new_atten)

    return attenuation


def get_attenuation_c_xc(test_id, noise_band, noise_db):
    input_file = "input.wav" # Required by test_wav_ns.xe

    output_file_c = "output_c.wav"
    output_file_xc = "output_xc.wav"

    audio_dir = test_id
    generate_test_audio(input_file, audio_dir, noise_band, db=noise_db)

    process_xe(input_file, output_file_c, audio_dir, c_ns_xe)
    process_xe(input_file, output_file_xc, audio_dir, xc_ns_xe)

    attenuation_c = get_attenuation(input_file, output_file_c, audio_dir)
    attenuation_xc = get_attenuation(input_file, output_file_xc, audio_dir)

    print("    C NS: {}".format(["%.2f"%item for item in attenuation_c]))
    print("    XC NS: {}".format(["%.2f"%item for item in attenuation_xc]))

    return attenuation_c, attenuation_xc


def parse_arguments():
    parser = argparse.ArgumentParser()
    parser.add_argument("noise_band", nargs='?', default=8000, type=int, help="Noise freq bandwidth")
    parser.add_argument("noise_level", nargs='?',default=-20, type=int, help="Nominal noise level (dBFS)")

    parser.parse_args()
    args = parser.parse_args()
    return args


def main():
    start_time = time.time()
    args = parse_arguments()
    get_attenuation_c_xc("test", args.noise_band, args.noise_level)
    print(("--- {0:.2f} seconds ---" .format(time.time() - start_time)))


if __name__ == "__main__":
    main()
