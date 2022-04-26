# Copyright 2022 XMOS LIMITED.
# This Software is subject to the terms of the XMOS Public Licence: Version 1.
import numpy as np
import sys
import soundfile as sf

frame_advance = 240

assert len(sys.argv) == 4, "usage: {} <input_wav_file> <output_gt_file> <ground_truth seconds>".format(sys.argv[0])

input_wav_file = sys.argv[1]
output_gt_file = sys.argv[2]
ground_truth = float(sys.argv[3])

frames = sf.info(input_wav_file, verbose=True).frames
ground_truth_list = [ground_truth] * int(frames / frame_advance)
ground_truth_list[0] = 0.0 #Make sure a non-zero GT triggers a GT change in the analysis tool

ground_truth_np = np.array(ground_truth_list)
ground_truth_np.tofile(output_gt_file, sep="\n")
