# Copyright 2022 XMOS LIMITED.
# This Software is subject to the terms of the XMOS Public Licence: Version 1.
import configparser
import subprocess
import os.path
import pytest
from aec_test_utils import get_test_instances, files_exist, read_config

import os
import tempfile
import scipy.io.wavfile
import numpy as np
import shutil
import tempfile
import xscope_fileio
import xtagctl
import io
from contextlib import redirect_stdout
import re
import glob

parser = configparser.ConfigParser()
parser.read("parameters.cfg")

in_dir   = parser.get("Folders", "in_dir")
out_dir = parser.get("Folders", "out_dir")

y_channel_count = parser.get("Config", "y_channel_count")
x_channel_count = parser.get("Config", "x_channel_count")
phases = parser.get("Config", "phases")

aec_xe = os.path.abspath(glob.glob(f"{parser.get('Binaries', 'aec_xc_dir')}/bin/*.xe")[0])


dut_in_wav = "input.wav"
dut_out_wav = "output.wav"
runtime_args_file = "args.bin"
dut_H_hat_file = "H_hat.bin"
def run_aec_xc(audio_in, audio_ref, audio_out, adapt=-1, h_hat_dump=None):
    rate, y_data = scipy.io.wavfile.read(audio_in)
    rate, x_data = scipy.io.wavfile.read(audio_ref)
    if(y_data.ndim == 1):
        y_data = np.atleast_2d(y_data).T
        x_data = np.atleast_2d(x_data).T
    data = np.hstack((y_data, x_data)) #mic+ref
    scipy.io.wavfile.write(dut_in_wav, rate, data)

    tmp_folder = tempfile.mkdtemp()
    scipy.io.wavfile.write(os.path.join(tmp_folder, dut_in_wav), rate, data)
    
    prev_path = os.getcwd()
    os.chdir(tmp_folder)    
        
    with open(runtime_args_file, "wb") as ref_file:
        ref_file.write(f"stop_adapting {adapt}".encode('utf-8'))

    with xtagctl.acquire("XCORE-AI-EXPLORER") as adapter_id:
        xscope_fileio.run_on_target(adapter_id, aec_xe)

    os.chdir(prev_path)
    #test_check_output expects a 2 channel output despite building AEC for 1 y channel, so convert dut output to 2ch
    rate, data = scipy.io.wavfile.read(os.path.join(tmp_folder, dut_out_wav))
    if(data.ndim == 1):
        data = np.atleast_2d(data).T
    data = np.hstack((data, data))
    scipy.io.wavfile.write(audio_out, rate, data)
    if h_hat_dump != None:
        shutil.copy2(os.path.join(tmp_folder, dut_H_hat_file), h_hat_dump)
    shutil.rmtree(tmp_folder, ignore_errors=True)    


@pytest.fixture
def test_type(request):
    test_name = request.node.name
    test_type = test_name[len("test_process_"):test_name.index('[')]
    return test_type


@pytest.fixture
def test_config(test_type):
    return read_config(test_type)


@pytest.mark.parametrize('test', get_test_instances('simple', in_dir, out_dir))
def test_process_simple(test):
    run_aec_xc(test['in_filename'], test['ref_filename'],
               test['out_filename'])


@pytest.mark.parametrize('test', get_test_instances('multitone', in_dir,
                                                    out_dir))
def test_process_multitone(test):
    run_aec_xc(test['in_filename'], test['ref_filename'],
               test['out_filename'])


@pytest.mark.parametrize('test', get_test_instances('excessive', in_dir,
                                                    out_dir))
def test_process_excessive(test):
    run_aec_xc(test['in_filename'], test['ref_filename'],
               test['out_filename'])


@pytest.mark.parametrize('test', get_test_instances('impulseresponse', in_dir,
                                                    out_dir))
def test_process_impulseresponse(test, test_config):
    stop_adapt_frame = (test_config['settle_time'] * 16000) // 240
    h_hat_xc = os.path.join(out_dir, test['id'] + "-h_hat.py")

    run_aec_xc(test['in_filename'], test['ref_filename'],
               test['out_filename'], stop_adapt_frame,
               h_hat_xc)


@pytest.mark.parametrize('test', get_test_instances('smallimpulseresponse',
                                                    in_dir, out_dir))
def test_process_smallimpulseresponse(test, test_config):
    stop_adapt_frame = (test_config['settle_time'] * 16000) // 240
    h_hat_xc = os.path.join(out_dir, test['id'] + "-h_hat.py")

    run_aec_xc(test['in_filename'], test['ref_filename'],
               test['out_filename'], stop_adapt_frame,
               h_hat_xc)


@pytest.mark.parametrize('test', get_test_instances('bandlimited', in_dir,
                                                    out_dir))
def test_process_bandlimited(test, test_config):
    stop_adapt_frame = (test_config['settle_time'] * 16000) // 240
    run_aec_xc(test['in_filename'], test['ref_filename'],
               test['out_filename'], stop_adapt_frame)
