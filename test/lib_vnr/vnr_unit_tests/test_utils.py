import numpy as np
import data_processing.frame_preprocessor as fp
import xscope_fileio
import xtagctl
import os
import tempfile
import sys
sys.path.append(os.path.join(os.getcwd(), "../../shared/python"))
import py_vs_c_utils as pvc
import scipy.io.wavfile

def run_dut(input_data, test_name, xe):
    tmp_folder = tempfile.mkdtemp(dir=".", suffix=os.path.basename(test_name))
    prev_path = os.getcwd()
    os.chdir(tmp_folder)
    input_data.astype(np.int32).tofile("input.bin")
    with xtagctl.acquire("XCORE-AI-EXPLORER") as adapter_id:
        xscope_fileio.run_on_target(adapter_id, xe)
        with open("output.bin", "rb") as fdut:
            dut_output = np.fromfile(fdut, dtype=np.int32)
    os.chdir(prev_path)
    os.system("rm -r {}".format(tmp_folder))
    return dut_output

def double_to_int32(x, exp):
    y = x.astype(np.float64) * (2.0 ** -exp)
    y = y.astype(np.int32)
    return y

def int32_to_double(x, exp):
    y = x.astype(np.float64) * (2.0 ** exp)
    return y

def get_closeness_metric(ref, dut):
    tmp_folder = tempfile.mkdtemp(dir=".")
    prev_path = os.getcwd()
    os.chdir(tmp_folder)
    output_file = "temp.wav"
    output_wav_data = np.zeros((2, len(ref)))
    output_wav_data[0,:] = ref
    output_wav_data[1,:] = dut
    scipy.io.wavfile.write(output_file, 16000, output_wav_data.T)
    arith_closeness, geo_closeness, c_delay, peak2ave = pvc.pcm_closeness_metric(output_file, verbose=False)
    os.chdir(prev_path)
    os.system("rm -r {}".format(tmp_folder))
    return arith_closeness, geo_closeness
