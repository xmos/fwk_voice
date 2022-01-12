# Copyright 2019-2021 XMOS LIMITED.
# This Software is subject to the terms of the XMOS Public Licence: Version 1.
import os
import sys
import numpy as np
import matplotlib.pyplot as plt
import subprocess
from shutil import copyfile, rmtree
from pathlib import Path

import audio_wav_utils as awu
from prepare_aec_input_file import prepare_input_file
from delay_analyser import delay_analyser
from delay_analyser import FRAME_ADVANCE 
import tempfile
import soundfile as sf
import glob

import xscope_fileio
import xtagctl
import io
from contextlib import redirect_stdout
import re


source_wav_file_rate = 48000
voice_sample_rate = 16000

#Locations of the xcore version of the code
xcore_working_dir = '../test_wav_ap'

aec_input_file = "stage_a_input_16k.wav"
adec_test_xe =os.path.abspath(glob.glob(f'../../../build/test/lib_adec/test_adec/bin/*.xe')[0])


def generate_random_delay_changes(number, spacing, min_s, max_s):
  delays = []
  for change in range(number):
    this_delay = np.random.uniform(min_s, max_s, 1)[0] * source_wav_file_rate
    this_time_samps = spacing * (1 + change) * source_wav_file_rate
    delays.append((int(this_time_samps), int(this_delay)))
  return delays

def run_test(pipeline_config, info, path_to_regression_files, input_audio_files, far_end_delay_changes, test_length_s=70, run_xcore=False, volume_changes=None):
  tmp_dir = tempfile.mkdtemp(prefix='tmp_', dir='.')
  os.chdir(tmp_dir)

  xe_name = adec_test_xe
  print('xe name = ',xe_name)

  config_file = 'two_mic_stereo.json' if pipeline_config == 'alt_arch' else 'two_mic_stereo_prev_arch.json'
  print(f'config_file: {config_file}')

  ground_truth_file = "ground_truth.txt"
  delay_estimate_file = "delay_estimate.txt"

  if far_end_delay_changes is not None:
    gt_changes = len(far_end_delay_changes)
    input_audio_dir = os.path.join(path_to_regression_files, 'input_audio_to_room_model')
    model_dir = os.path.join(path_to_regression_files, 'room_model')
    output_audio_dir = '.'

    prepare_input_file(input_audio_files, input_audio_dir, model_dir, output_audio_dir, far_end_delay_changes, max_seconds=test_length_s, volume_changes=volume_changes)
    test_name = info + ", " + input_audio_files[0] + ", " + input_audio_files[1]
  else:
    copyfile(input_audio_files, aec_input_file)
    ground_truth_file = input_audio_files.strip(".wav") + (".delays")
    copyfile(ground_truth_file, "ground_truth.txt")

    gt_changes = 0
    test_name = info + ", " + input_audio_files.split('/')[-1]

  # Run xcore/axe version of simulator
  if run_xcore:
    #Run hardware in loop simulation (debug only as requires attached board. Also requires xplay to be on system path)
    print ("run_xcore = ", run_xcore, ", tmp_dir = ", tmp_dir)
    copyfile("stage_a_input_16k.wav", "input.wav") #Axe sim has fixed file name input

    with xtagctl.acquire("XCORE-AI-EXPLORER") as adapter_id:
        f = io.StringIO()      
        print(f"Running on {adapter_id}")
        with redirect_stdout(f):
            xscope_fileio.run_on_target(adapter_id, xe_name)
        xcore_stdo = []
        #ignore lines that don't contain [DEVICE]. Remove everything till and including [DEVICE] if [DEVICE] is present
        for line in f.getvalue().splitlines():
            m = re.search(r'\[DEVICE\] xc_sim_delays:\s*(.*)', line)
            if m is not None:
                xcore_stdo.append(m.group(1))            
        xcore_stdo = np.asarray(xcore_stdo, dtype=np.float)
        estimates = xcore_stdo / float(voice_sample_rate)
    
    #Scale estimates file to seconds
    xc_sim_de_file = "xc_sim_delays_s.txt"
    print("estimates = ",estimates)
    estimates.tofile(xc_sim_de_file, sep="\n")

    xcore_delay_analyser = delay_analyser(FRAME_ADVANCE, ground_truth_file, xc_sim_de_file)
    report_summary = xcore_delay_analyser.analyse_events()
    report_summary['test_name'] = "Xcore: " + test_name
    report_summary['gt_changes'] = gt_changes

    graph_file_name = test_name + "_xcore.png"
    xcore_delay_analyser.graph_delays(file_name=graph_file_name)
    copyfile(graph_file_name, "../" + graph_file_name)

  os.chdir("..")
  rmtree(tmp_dir)

  return report_summary


