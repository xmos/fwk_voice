#!/usr/bin/env python
# Copyright 2023 XMOS LIMITED.
# This Software is subject to the terms of the XMOS Public Licence: Version 1.
import os
import time
import argparse
import numpy as np
import struct
import matplotlib.pyplot as plt
import librosa as lr
import librosa.display as lrdisplay
from scipy.io import wavfile
from xscope_py3 import Endpoint, QueueConsumer

MEL_SPEC_SMALL_FS = 16000
MEL_SPEC_SMALL_N_FFT = 512
MEL_SPEC_SMALL_N_SAMPLES = 6400
MEL_SPEC_SMALL_N_MEL = 64
MEL_SPEC_SMALL_N_FRAMES = 26
MEL_SPEC_SMALL_HOP = 256
MEL_SPEC_SMALL_SCALE = 3.325621485710144e-1
MEL_SPEC_SMALL_ZERO_POINT = 12
MEL_SPEC_SMALL_TOP_DIM_SZ = 1
MEL_SPEC_SMALL_LOW_DIM_SZ = 4
MEL_SPEC_SMALL_MIN_LOG10_IN = 1e-10
MEL_SPEC_SMALL_DB_DYNAMIC_RANGE = 80

MEL_SPEC_LARGE_FS = 16000
MEL_SPEC_LARGE_N_FFT = 1024
MEL_SPEC_LARGE_N_SAMPLES = 84800
MEL_SPEC_LARGE_N_MEL = 128
MEL_SPEC_LARGE_N_FRAMES = 158
MEL_SPEC_LARGE_HOP = 512
MEL_SPEC_LARGE_SCALE = 3.921553026884794e-3
MEL_SPEC_LARGE_ZERO_POINT = 128
MEL_SPEC_LARGE_TOP_DIM_SZ = 1
MEL_SPEC_LARGE_LOW_DIM_SZ = 4
MEL_SPEC_LARGE_MIN_LOG10_IN = 1e-10
MEL_SPEC_LARGE_DB_DYNAMIC_RANGE = 80


class TestEndpoint(Endpoint):
    def __init__(self):
        super().__init__()

    def send_blob(self, blob):
        CHUNK_SIZE = 128
        SLEEP_DURATION = 0.025

        self.ready = False
        for i in range(0, len(blob), CHUNK_SIZE):
            self.publish(blob[i : i + CHUNK_SIZE])
            time.sleep(SLEEP_DURATION)


def convert_to_float(value):
    return struct.unpack(">f", int(value).to_bytes(4, byteorder="big", signed=True))[0]


def get_flat_output(opts):
    ep = TestEndpoint()
    output = QueueConsumer(ep, "Mel Output")
    execution_time = QueueConsumer(ep, "Execution Time")

    n_samples_total = opts["n_samples"] * opts["number_of_blocks"]

    try:
        if ep.connect():
            print("Failed to connect")
        else:
            print("Connected")
            print("Sending file to device: " + opts["input_file"])

            # load data from input file
            _, input_extension = os.path.splitext(opts["input_file"])
            if input_extension == ".wav":
                # load wav file data
                _, data = wavfile.read(opts["input_file"])
            else:
                # assume raw 16-bit data
                data = np.fromfile(opts["input_file"], dtype="int16")

            ep.send_blob(data.flatten().tobytes()[: (n_samples_total * 2)])

            time.sleep(10)
            print("DONE")

            return np.array(output.next(0)), execution_time.next(0)

    except KeyboardInterrupt:
        pass


def get_opts(args):
    opts = {
        "fs": None,
        "n_fft": None,
        "n_samples": None,
        "n_mel": None,
        "n_frames": None,
        "hop": None,
    }
    if args.size == "small":
        opts["fs"] = MEL_SPEC_SMALL_FS
        opts["n_fft"] = MEL_SPEC_SMALL_N_FFT
        opts["n_samples"] = MEL_SPEC_SMALL_N_SAMPLES
        opts["n_mel"] = MEL_SPEC_SMALL_N_MEL
        opts["n_frames"] = MEL_SPEC_SMALL_N_FRAMES
        opts["hop"] = MEL_SPEC_SMALL_HOP
    else:
        opts["fs"] = MEL_SPEC_LARGE_FS
        opts["n_fft"] = MEL_SPEC_LARGE_N_FFT
        opts["n_samples"] = MEL_SPEC_LARGE_N_SAMPLES
        opts["n_mel"] = MEL_SPEC_LARGE_N_MEL
        opts["n_frames"] = MEL_SPEC_LARGE_N_FRAMES
        opts["hop"] = MEL_SPEC_LARGE_HOP

    opts["input_file"] = args.input_file
    opts["number_of_blocks"] = args.number_of_blocks
    opts["size"] = args.size
    opts["render"] = args.render
    opts["output_file"] = args.output_file

    return opts


def write_output(opts, op):
    with open(opts["output_file"], "w") as f:
        f.write("\n")
    for data_block in range(opts["number_of_blocks"]):
        with open(opts["output_file"], "a") as f:
            for mel in op[data_block, :, :][-1::-1]:
                f.write("".join([f"{x:4}, " for x in mel]))
                f.write("\n")
            f.write("\n")


def render_plot(opts, op):
    _, axs = plt.subplots(
        1, opts["number_of_blocks"], squeeze=False, sharex="col", sharey="all"
    )

    for c_idx in range(opts["number_of_blocks"]):
        for r_idx in range(1):
            lrdisplay.specshow(
                op[c_idx, :, :],
                sr=opts["fs"],
                y_axis="mel",
                x_axis="time",
                fmax=(opts["fs"] / 2),
                hop_length=opts["hop"],
                #n_fft=opts["n_fft"],
                ax=axs[r_idx][c_idx],
            )
    plt.show()


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description="Demonstrate Mel spectrogram")
    parser.add_argument("-i", "--input-file", action="store")
    parser.add_argument("-b", "--number-of-blocks", action="store", type=int)
    parser.add_argument("-s", "--size", action="store", choices=("small", "large"))
    parser.add_argument("-r", "--render", action="store_true")
    parser.add_argument("-o", "--output-file", action="store", default="out.txt")

    args = parser.parse_args()
    opts = get_opts(args)

    op_flat, exec_time = get_flat_output(opts)
    op = op_flat.reshape(opts["number_of_blocks"], opts["n_mel"], opts["n_frames"])

    print(f"Execution time, per block: {[x * 10 for x in exec_time]}ns")

    write_output(opts, op)

    if opts["render"]:
        render_plot(opts, op)
