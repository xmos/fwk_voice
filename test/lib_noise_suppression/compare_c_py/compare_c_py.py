# Copyright 2018-2021 XMOS LIMITED.
# This Software is subject to the terms of the XMOS Public Licence: Version 1.
from builtins import str
from builtins import range
import sys
import os

package_dir = os.path.dirname(os.path.abspath(__file__))
path1 = os.path.join(package_dir,'../../../audio_test_tools/python/')
path2 = os.path.join(package_dir,'../../python/')

sys.path.append(path1)
sys.path.append(path2)

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
sup_xe =os.path.abspath(glob.glob(f'{parser.get("Binaries", "xe_path")}/bin/*.xe')[0])
print(os.path.abspath(sup_xe))

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


def process_c(filemane, output_file, audio_dir):
    #shutil.copy2(input_file, os.path.join(audio_dir, filemane))

    prev_path = os.getcwd()
    os.chdir(audio_dir)    
    
    with xtagctl.acquire("XCORE-AI-EXPLORER") as adapter_id:
        xscope_fileio.run_on_target(adapter_id, sup_xe)

    os.chdir(prev_path)    
    #shutil.copy2(os.path.join(tmp_folder, "output.wav"), output_file)


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


def get_attenuation_c_py(test_id, noise_band, noise_db):
    input_file = "input.wav" # Required by test_wav_suppression.xe

    output_file_c = "output.wav"

    audio_dir = test_id
    generate_test_audio(input_file, audio_dir, noise_band, db=noise_db)

    process_c(input_file, output_file_c, audio_dir)

    attenuation_c = get_attenuation(input_file, output_file_c, audio_dir)
    print("    C SUP: {}".format(["%.2f"%item for item in attenuation_c]))

    return attenuation_c


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
    #get_attenuation_xc_py("test", args.noise_band, args.noise_level)
    print(("--- {0:.2f} seconds ---" .format(time.time() - start_time)))


if __name__ == "__main__":
    main()