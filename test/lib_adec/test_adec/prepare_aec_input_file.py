# Copyright 2022 XMOS LIMITED.
# This Software is subject to the terms of the XMOS Public Licence: Version 1.
import sys
from impulse_3510 import do_simulation
from delay_estimator_utils import apply_delay_changes, get_file_energy_stats, ema_filter, moving_average_filter, extract_audio_and_gt_section, do_delay_estimate
from delay_estimator_utils import frame_advance, frame_size, input_file_fs, voice_sample_rate, frame_rate, aec_phases
from shutil import copyfile
import os
import subprocess
import matplotlib.pyplot as plt
import numpy as np
import soundfile as sf

def prepare_input_file(input_audio_files, input_audio_dir, model_dir, output_audio_dir, far_end_delay_changes, max_seconds=False, volume_changes=None):

  #optionally cut audio files to make processing quicker
  if max_seconds != False:
    do_simulation(input_audio_dir, input_audio_files, model_dir, output_audio_dir, max_frames=int(max_seconds * 48000), volume_changes=volume_changes)
  else:
    do_simulation(input_audio_dir, input_audio_files, model_dir, output_audio_dir, volume_changes=volume_changes)

  input_audio_file = 'simulated_room_output.wav'
  input_audio = os.path.join(output_audio_dir, input_audio_file)

  output_audio_file = 'stage_a_input_48k.wav'
  output_audio = os.path.join(output_audio_dir, output_audio_file)
  copyfile(input_audio, output_audio)

  ground_truth = apply_delay_changes(output_audio, output_audio, far_end_delay_changes)
  aec_input_file = "stage_a_input_16k.wav"


  #Resample to 16kHz as required by python VTB models
  subprocess.call("sox {} -r 16000 {}".format(output_audio, aec_input_file).split(" "))
  ground_truth = ground_truth[::int(input_file_fs / voice_sample_rate)] / (int(input_file_fs / voice_sample_rate))

  #subsample the ground truth array (from one per 16k sample) to one per frame advance (240)
  indicies = [i for i in range(frame_size, ground_truth.shape[0], frame_advance)]
  ground_truth_by_vtb_frame = np.take(ground_truth, indicies)

  #Change to seconds
  ground_truth_by_vtb_frame = ground_truth_by_vtb_frame / voice_sample_rate

  #Write ground truth
  ground_truth_by_vtb_frame.tofile("ground_truth.txt", sep="\n")



if __name__ == "__main__":
  far_end_delay_changes = [(48000*0, 48000 * -0.00), (48000*10, 48000 * 0.10), (48000*25, 48000 * -0.10), (48000*40, 48000 * -0.15), (48000*55, 48000 * 0.0)]
  prepare_input_file( ['happy.wav', 'busy_bar_5min_48k.wav'], 'adec_regression/input_audio_to_room_model', '.', far_end_delay_changes)
