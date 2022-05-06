import numpy as np
import data_processing.frame_preprocessor as fp
import xscope_fileio
import xtagctl
import os
import tempfile

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
