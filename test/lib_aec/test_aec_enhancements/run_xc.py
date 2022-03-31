# Copyright 2022 XMOS LIMITED.
# This Software is subject to the terms of the XMOS Public Licence: Version 1.
import numpy as np
import os
import tempfile
import shutil
import subprocess
import soundfile as sf
import xscope_fileio
import xtagctl
import io
from contextlib import redirect_stdout
import re
import scipy.io.wavfile
import configparser
import glob

parser = configparser.ConfigParser()
parser.read("parameters.cfg")
aec_xe =os.path.abspath(glob.glob(f'{parser.get("Binaries", "xe_path")}/bin/*.xe')[0])
print(os.path.abspath(aec_xe))
in_dir = parser.get("Folders", "in_dir")
out_dir = parser.get("Folders", "out_dir")

adapt_mode_dict = {'AEC_ADAPTION_AUTO':0, 'AEC_ADAPTION_FORCE_ON':1, 'AEC_ADAPTION_FORCE_OFF': 2}

dut_H_hat_file = "H_hat.bin"
runtime_args_file = "args.bin"
AEC_MAX_Y_CHANNELS = int(parser.get("Config", "y_channel_count"))
AEC_MAX_X_CHANNELS = int(parser.get("Config", "x_channel_count"))

def run_aec_xc(y_data, x_data, testname, adapt=-1, h_hat_dump=None, adapt_mode=adapt_mode_dict['AEC_ADAPTION_AUTO'], num_y_channels=AEC_MAX_Y_CHANNELS, num_x_channels=AEC_MAX_X_CHANNELS):
    input_file = f"{in_dir}/input_{testname}.wav"
    output_file = f"{out_dir}/output_{testname}.wav"
    #input wav file always has (AEC_MAX_Y_CHANNELS + AEC_MAX_X_CHANNELS) channels, as per the build time aec configuration. Changing AEC config at runtime shouldn't affect input packing
    tmp_folder = tempfile.mkdtemp()
    if(y_data.ndim == 1):
        y_data = np.atleast_2d(y_data).T
    if(x_data.ndim == 1):
        x_data = np.atleast_2d(x_data).T
    
    y_chans = y_data.shape[-1]
    x_chans = x_data.shape[-1]

    #All input wav files need to have AEC_MAX_Y_CHANNELS y channels and AEC_MAX_X_CHANNELS x channels since this is the configuration AEC is built with
    extra_y_chans = AEC_MAX_Y_CHANNELS - y_chans
    extra_x_chans = AEC_MAX_X_CHANNELS - x_chans
    #duplicate last column to get required no. of channels
    if extra_y_chans:
        extra_y = np.tile(y_data[:,[-1]], extra_y_chans)
        y_data = np.hstack((y_data, extra_y))
    if extra_x_chans:
        extra_x = np.tile(x_data[:,[-1]], extra_x_chans)
        x_data = np.hstack((x_data, extra_x))
    input_data = np.hstack((y_data, x_data))
    scipy.io.wavfile.write(input_file, 16000, input_data)
 
    #write runtime arguments into args.bin
    with open(runtime_args_file, "wb") as fargs:
        fargs.write(f"y_channels {num_y_channels}\n".encode('utf-8'))
        fargs.write(f"x_channels {num_x_channels}\n".encode('utf-8'))
        fargs.write(f"stop_adapting {adapt}\n".encode('utf-8'))
        fargs.write(f"adaption_mode {adapt_mode}\n".encode('utf-8'))
    
    shutil.copy2(input_file, os.path.join(tmp_folder, "input.wav"))
    shutil.copy2(runtime_args_file, os.path.join(tmp_folder, runtime_args_file))

    prev_path = os.getcwd()
    os.chdir(tmp_folder)    
    
    with xtagctl.acquire("XCORE-AI-EXPLORER") as adapter_id:
        xscope_fileio.run_on_target(adapter_id, aec_xe)

    os.chdir(prev_path)    
    shutil.copy2(os.path.join(tmp_folder, "output.wav"), output_file)
    if h_hat_dump is not None:
        shutil.copy2(os.path.join(tmp_folder, dut_H_hat_file), h_hat_dump)
    
    shutil.rmtree(tmp_folder, ignore_errors=True)    
    return input_file, output_file


def get_h_hat(filename, aec):
    """Gets H_hat from XC H_hat dump

    WARNING: This could be dangerous, the filename argument is parsed as
    python when aec = 'xc'.
    """
    H_hat = None

    if aec == 'xc':
        shutil.copy2(filename, "temp.py")
        from temp import H_hat
    else:
        with open(filename, "rb") as f:
            H_hat = np.load(f)
    assert H_hat is not None
    return H_hat


