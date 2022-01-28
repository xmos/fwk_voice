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

from get_polar_response import get_polar_response

TESTS = {
    'rt60':[0.2, 0.5, 0.9],
    'noise_level':[-20],
    'noise_band':[8000]
    }


def sweep_ic_delay(delay_min, delay_max, delay_step):
    delays = list(range(delay_min, delay_max + 1, delay_step))
    results = []

    for delay in delays:
        delay_result = []
        for rt60 in TESTS['rt60']:
            for level in TESTS['noise_level']:
                for band in TESTS['noise_band']:
                    angles, responses = get_polar_response('test', 360, 20, band, level, rt60, delay)
                    responses = responses[0]
                    reponses_av = sum(responses) / len(responses)
                    delay_result.append(reponses_av)
                    # print(delay, rt60, level, band, reponses_av)

        delay_result_av = sum(delay_result) / len(delay_result)
        np.set_printoptions(precision=2)
        print("Delay: {}, average result: {}".format(delay, delay_result_av))
        results.append(delay_result_av)

    return delays, results


def plot_results(delays, results):
    plt.plot(delays, results, 'ro')
    plt.show()


def parse_arguments():
    parser = argparse.ArgumentParser()
    parser.add_argument("--delay_min", nargs="?", default=20, type=int, help="Min IC delay")
    parser.add_argument("--delay_max", nargs="?", default=220, type=int, help="Max IC delay")
    parser.add_argument("--step_size", nargs="?", default=40, type=int, help="Delay sweep step size")
    args = parser.parse_args()
    return args


def main():
    start_time = time.time()
    args = parse_arguments()
    delays, results = sweep_ic_delay(args.delay_min, args.delay_max, args.step_size)
    plot_results(delays, results)
    print("--- {0:.2f} seconds ---" .format(time.time() - start_time))


if __name__ == "__main__":
    main()
