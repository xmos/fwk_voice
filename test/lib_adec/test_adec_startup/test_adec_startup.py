# Copyright 2022 XMOS LIMITED.
# This Software is subject to the terms of the XMOS Public Licence: Version 1.
import sys
import os
import shutil
import scipy.io.wavfile
import pytest
import subprocess
import numpy as np
import tempfile
import xscope_fileio
import xtagctl
import io
import re
from contextlib import redirect_stdout

maximum_adec_delay_ms = 1000
maximum_adec_estimation_time_ms = 3500

this_file_path = os.path.dirname(os.path.realpath(__file__))
xcore_binary = os.path.join(this_file_path, "../../../build/test/lib_adec/test_adec_startup/bin/fwk_voice_test_adec_startup.xe")

@pytest.fixture
def input_vectors():
    hydra_audio_base_dir = os.path.expanduser("~/hydra_audio/")
    try:
        hydra_audio_base_dir = os.environ['hydra_audio_PATH']
    except:
        print(f'Warning: hydra_audio_PATH environment variable not set. Using local path {hydra_audio_base_dir}')
    test_files = [os.path.abspath(os.path.join(hydra_audio_base_dir, "adec_regression/startup_test_case/david_b_vestel.wav"))]

    return test_files


def analyse_cancellation(output_file, de_end_frame):
    rate, data = scipy.io.wavfile.read(output_file)
    num_seconds_post = 5
    sos_pre_adapt = np.mean(np.square(data[de_end_frame-16000 : de_end_frame , 0]))
    sos_post_adapt = np.mean(np.square(data[de_end_frame+(num_seconds_post * 16000) : de_end_frame+((num_seconds_post+1) * 16000) , 0]))
    cancel_dB = 10 * np.log10(sos_post_adapt/sos_pre_adapt)
    print(f"AEC cancellation after {num_seconds_post}s ADEC is approximatedly {cancel_dB:.2f}dB", file=sys.stderr)
    assert cancel_dB > 3



def test_adec_startup(input_vectors):
    test_file = input_vectors[0]
    tmp_dir = tempfile.mkdtemp(prefix='tmp_', dir='.')
    prev_dir = os.getcwd()
    os.chdir(tmp_dir)
    print(f"Using tmpdir: {tmp_dir}")
    xe_file = "test_wav_adec_startup.xe"
    input_name = "input.wav"
    try:
        shutil.copyfile(xcore_binary, xe_file)
    except FileNotFoundError:
        print(f'file: {xcore_binary} not found')
    try:
        shutil.copyfile(test_file, input_name)
    except FileNotFoundError:
        print(f'file: {test_file} not found')
    # Create empty arguments file for test_wav_adec
    fp = open("args.bin", "wb")
    fp.close()

    print ("Waiting for xcore simulation run to complete..")
    with xtagctl.acquire("XCORE-AI-EXPLORER") as adapter_id:
        print(f"Running on {adapter_id}")
        try:
            with open("stdo.txt", "w+") as ff:
                xscope_fileio.run_on_target(adapter_id, xe_file, stdout=ff)
                ff.seek(0)
                stdo = ff.readlines()
        except Exception as e:
            print(e, file=sys.stderr)
            assert 0, f"FAILURE RUNNING: xscope_fileio.run_on_target({adapter_id} , {xe_file})"
    xcore_stdo = []
    for line in stdo:
        m = re.search(r'^\s*\[DEVICE\]', line)
        if m is not None:
            xcore_stdo.append(re.sub(r'\[DEVICE\]\s*', '', line))
    
    transitions = []
    for line in xcore_stdo:
        match = re.search(r'!!ADEC STATE CHANGE!!\s*Frame:\s*([0-9]+)\s*old:\s([A-Z]+)\s*new:\s([A-Z]+)', line)
        if match is not None:
            transitions.append({'frame':int(match[1]), 'old_state':match[2], 'new_state':match[3]})
    print("transitions = ",transitions)
    frame_duration_ms = 15.0
    for tr in transitions:
        if tr['old_state'] == 'AEC' and tr['new_state'] == 'DE':
            transition_to_de_ms = (tr['frame'] + 1) * frame_duration_ms 
            break
    for tr in transitions:
        if tr['old_state'] == 'DE' and tr['new_state'] == 'AEC':
            transition_out_of_de_ms = (tr['frame'] + 1) * frame_duration_ms 
            break
    adec_estimation_time_ms = transition_out_of_de_ms - transition_to_de_ms 
    print(f"first transition to DE: {transition_to_de_ms}ms, ADEC estimation time: {adec_estimation_time_ms}ms", file=sys.stderr)

    #Now calc 1s RMS of various bits to see AEC convergence
    analyse_cancellation("output.wav", int(transition_out_of_de_ms / 1000 * 16000))
    assert transition_to_de_ms < maximum_adec_delay_ms, f"ADEC too late: {transition_to_de_ms} max {maximum_adec_delay_ms}"
    assert adec_estimation_time_ms < maximum_adec_estimation_time_ms, f"ADEC took too long to estimate: {adec_estimation_time_ms} max {maximum_adec_estimation_time_ms}"
    os.chdir(prev_dir)
    shutil.rmtree(tmp_dir)
