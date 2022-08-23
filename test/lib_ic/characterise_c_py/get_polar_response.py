# Copyright 2022 XMOS LIMITED.
# This Software is subject to the terms of the XMOS Public Licence: Version 1.
from __future__ import division
from __future__ import print_function
from builtins import range
import sys
import os
import time
import argparse
import numpy as np
import matplotlib.pyplot as plt
import datetime

from characterise_c_py import generate_test_audio, process_py, process_c,\
                                get_attenuation, rt60_type


def get_polar_response(test_id, angle_roi, step_size, noise_band, noise_db,
                        rt60, x_channel_delay, run_c=False):
    audio_dir = test_id
    angles = list(range(0, 1 + angle_roi, step_size))
    results_py = []
    results_c = []

    for angle in angles:
        input_file = "{}_{}.wav".format(test_id, angle)
        output_file_py = "output_{}_py.wav".format(input_file)
        output_file_c = "output_{}_c.wav".format(input_file)

        angle_radians = angle * np.pi / 180
        generate_test_audio(input_file, audio_dir, noise_band, noise_db, angle_radians, rt60)
        process_py(input_file, output_file_py, audio_dir)
        attenuation_py = get_attenuation(input_file, output_file_py, audio_dir)
        results_py.append(attenuation_py[-2])

        if run_c:
            process_c(input_file, output_file_c, audio_dir)
            attenuation_c = get_attenuation(input_file, output_file_c, audio_dir)
            results_c.append(attenuation_c[-2])

    return angles, [results_py, results_c]


def polar_plot(filename, description, angles, results):
    ax = plt.subplot(111, projection="polar")
    r = np.array(angles) * np.pi / 180
    if results[0]:
        ax.plot(r, results[0], label='Python Attenuation (dB)')
    if results[1]:
        ax.plot(r, results[1], label='C Attenuation (dB)')
    ax.grid(True)
    ax.legend(bbox_to_anchor=(0.5, -0.1), loc="upper center")
    ax.set_title("IC Polar Response {}".format(description), va="bottom")
    plt.savefig(filename, bbox_inches="tight")


def parse_arguments():
    parser = argparse.ArgumentParser()
    parser.add_argument("output_file", help="File name of output plot, eg polar_plot.png")
    parser.add_argument("-c", action='store_true', help="Run C model")
    parser.add_argument("--angle_roi", nargs="?", default=360, type=int, help="Angle region-of-interest to sweep (e.g. 180)")
    parser.add_argument("--step_size", nargs="?", default=20, type=int, help="Angle step size in polar sweep")
    parser.add_argument("--rt60", nargs="?", default=0.3, type=rt60_type, help="RT60 of environment")
    parser.add_argument("--noise_band", nargs="?", default=8000, type=int, help="Noise freq bandwidth")
    parser.add_argument("--noise_level", nargs="?",default=-20, type=int, help="Nominal noise level (dBFS)")
    parser.add_argument("--ic_delay", nargs="?",default=80, type=int, help="IC x channel delay")
    args = parser.parse_args()
    return args


def main():
    start_time = time.time()
    args = parse_arguments()
    test_id = "room_{}Hz_{}dB_{}s_ICdelay_{}".format(args.noise_band, args.noise_level, args.rt60, args.ic_delay)
    angles, results = get_polar_response(test_id, args.angle_roi, args.step_size,
                                            args.noise_band, args.noise_level,
                                            args.rt60, args.ic_delay,
                                            args.c)

    polar_plot(args.output_file, test_id, angles, results)
    print("--- {0:.2f} seconds ---".format(time.time() - start_time))

if __name__ == "__main__":
    main()
