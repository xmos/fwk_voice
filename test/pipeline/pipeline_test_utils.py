# Copyright 2022 XMOS LIMITED.
# This Software is subject to the terms of the XMOS Public Licence: Version 1.

import numpy as np
import os, shutil, tempfile, sys
import xscope_fileio, xtagctl
import subprocess
import re
from conftest import pipeline_bins, pipeline_output_base_dir, keyword_input_base_dir, xtag_aquire_timeout_s
import re
from pathlib import Path

def process_xcore(xe_file, input_file, output_file):
    input_file = os.path.abspath(input_file)

    tmp_folder = tempfile.mkdtemp(suffix=os.path.basename(__file__))
    prev_path = os.getcwd()
    os.chdir(tmp_folder)
    shutil.copyfile(input_file, "input.wav")

    #Make sure we can wait for 2 processing occurances to finish
    with xtagctl.acquire("XCORE-AI-EXPLORER", timeout=xtag_aquire_timeout_s) as adapter_id:
        with open("stdout.txt", "w+") as ff:
            try:
                xscope_fileio.run_on_target(adapter_id, xe_file, stdout=ff)
            except Exception as e:
                print(e, file=sys.stderr)
                assert 0, f"FAILURE RUNNING: xscope_fileio.run_on_target({adapter_id} , {xe_file})"
            ff.seek(0)
            stdout = ff.readlines()

    shutil.copyfile("output.wav", output_file)
    os.chdir(prev_path)
    shutil.rmtree(tmp_folder)

    return stdout

def process_x86(bin_file, input_file, output_file):
    cmd = (bin_file, input_file, output_file)
    stdout = subprocess.check_output(cmd, text=True, stderr=subprocess.STDOUT)
    return stdout

def process_file(input_file, arch, target="xcore"):
    wav_name = os.path.basename(input_file)
    output_file = os.path.join(pipeline_output_base_dir + "_" + arch + "_" + target, wav_name)

    pipeline_bin = pipeline_bins[arch][target]
    stdout = ""
    if target == "xcore":
        stdout = process_xcore(pipeline_bin, input_file, output_file)
    else:
        stdout = process_x86(pipeline_bin, input_file, output_file)

    return output_file, stdout


def get_wav_info(input_file):
    chans = int(subprocess.check_output(("soxi", "-c", input_file)))
    rate = int(subprocess.check_output(("soxi", "-r", input_file)))
    samps = int(subprocess.check_output(("soxi", "-s", input_file)))
    bits = int(subprocess.check_output(("soxi", "-b", input_file)))

    return chans, rate, samps, bits

def convert_input_wav(input_file, output_file):
    chans, rate, samps, bits = get_wav_info(input_file)
    if not os.path.isfile(output_file): #optimisation. Only convert once if running test a lot locally
        extra_args = "" #"trim 0 5" #to test with short wavs
        if chans == 6:
            #for 6 channel wav file, first 4 channels are the mic input and last 2 channels are far-end audio
            subprocess.run(f"sox {input_file} -r 16000 -b 32 {output_file} remix 1 4 5 6 {extra_args}".split()) #read mic in from channel spaced ~ 100mm, hence channel index 1 and 4(assuming 33mm spacing between mics)
        elif chans == 8:
            # for 8 channel wav file, first 2 channels are comms and asr outputs, followed by 4 channels of mic input
            # and last 2 channels are far-end audio
            subprocess.run(f"sox {input_file} -r 16000 -b 32 {output_file} remix 3 6 7 8 {extra_args}".split())
        elif chans == 4:
            # for 4 channel wav file, first 2 channels are mic, followed by 2 channels of reference input
            # and last 2 channels are far-end audio
            subprocess.run(f"sox {input_file} -r 16000 -b 32 {output_file} {extra_args}".split())
        else:
            assert False, f"Error: input wav format not supported - chans:{chans}"
    return output_file

def convert_keyword_wav(input_file, arch, target):
    wav_name = os.path.basename(input_file)
    keyword_file = os.path.join(keyword_input_base_dir + "_" + arch + "_" + target, wav_name)
    # Strip off comms channel leaving just ASR. Sensory needs a 16b wav file
    subprocess.run(f"sox {input_file} -b 16 {keyword_file} remix 1".split())
    return keyword_file

def log_vnr(stdo, input_file, arch, target): # Read VNR predicitions from stdo and store in .npy files of the same name as input files
    xcore_stdo = []
    if target == "xcore":
        for line in stdo:
            m = re.search(r'^\s*\[DEVICE\]', line)
            if m is not None:
                xcore_stdo.append(re.sub(r'\[DEVICE\]\s*', '', line))
    else:
        for line in stdo.split('\n'):
            xcore_stdo.append(line)
    
    vnr_output_pred = np.empty(0, dtype=np.float64)
    vnr_input_pred = np.empty(0, dtype=np.float64)
    for line in xcore_stdo:
        match = re.search(r'VNR INPUT PRED:\s*([-0-9]+)\s*([-0-9]+)', line)
        if match != None:
            vnr_mant = float(match.group(1))
            vnr_exp = float(match.group(2))
            vnr = vnr_mant * (2.0 ** vnr_exp)
            vnr_input_pred = np.append(vnr_input_pred, vnr)

        match = re.search(r'VNR OUTPUT PRED:\s*([-0-9]+)\s*([-0-9]+)', line)
        if match != None:
            vnr_mant = float(match.group(1))
            vnr_exp = float(match.group(2))
            vnr = vnr_mant * (2.0 ** vnr_exp)
            vnr_output_pred = np.append(vnr_output_pred, vnr)
    
    if(len(vnr_input_pred) > 0):
        filename = f"vnr_input_pred_{os.path.splitext(Path(os.path.basename(input_file)))[0]}.npy"
        filename = os.path.join(keyword_input_base_dir + "_" + arch + "_" + target, filename)
        np.save(filename, vnr_input_pred)

    if(len(vnr_output_pred) > 0):
        filename = f"vnr_output_pred_{os.path.splitext(Path(os.path.basename(input_file)))[0]}.npy"
        filename = os.path.join(keyword_input_base_dir + "_" + arch + "_" + target, filename)
        np.save(filename, vnr_output_pred)
