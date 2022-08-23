import numpy as np
import data_processing.frame_preprocessor as fp
import py_vnr.vnr as vnr
import os
import sys
this_file_dir = os.path.dirname(os.path.realpath(__file__))
sys.path.append(os.path.join(this_file_dir, "../feature_extraction"))
import test_utils
import tensorflow as tf
import math

exe_dir = os.path.join(this_file_dir, '../../../../build/test/lib_vnr/vnr_unit_tests/inference/bin/')
xe = os.path.join(exe_dir, 'fwk_voice_test_vnr_priv_output_dequantise.xe')

def test_vnr_priv_output_dequantise(target, tflite_model):
    np.random.seed(1243)
    vnr_obj = vnr.Vnr(model_file=tflite_model) 

    input_data = np.empty(0, dtype=np.int32)
    input_words_per_frame = 1 # 1 int32 value out of which only the 1st byte is relevant since inference output is a single byte
    output_words_per_frame = 2 # 1 float_s32_t value
    input_data = np.append(input_data, np.array([input_words_per_frame, output_words_per_frame], dtype=np.int32))

    min_int = -128
    max_int = 127
    test_frames = 2048
    ref_output_double = np.empty(0, dtype=np.float64)
    dut_output_double = np.empty(0, dtype=np.float64)
    for itt in range(0,test_frames):
        data = np.random.randint(min_int, high=max_int+1, size=1)
        input_data = np.append(input_data, data)

        # Reference dequantise implementation
        data = data.astype(dtype=np.int8)
        dequant_output = test_utils.dequantise_output(tflite_model, data)
        ref_output_double = np.append(ref_output_double, dequant_output)

    exe_name = xe
    if(target == "x86"): #Remove the .xe extension from the xe name to get the x86 executable
        exe_name = os.path.splitext(xe)[0]
    op = test_utils.run_dut(input_data, "test_vnr_priv_output_dequantise", exe_name)
    dut_mant = op[0::2]
    dut_exp = op[1::2]
    d = dut_mant.astype(np.float64) * (2.0 ** dut_exp)
    dut_output_double = np.append(dut_output_double, d)
    for fr in range(0,test_frames):
        dut = dut_mant[fr]
        ref = int(ref_output_double[fr] * (2.0 ** -dut_exp[fr]))
        diff = np.abs(ref-dut)
        assert(diff < 1), "ERROR: test_vnr_priv_output_dequantise frame {fr}. diff exceeds threshold"
    
    print("max_diff = ",np.max(np.abs(ref_output_double - dut_output_double)))


if __name__ == "__main__":
    test_vnr_priv_output_dequantise("xcore", test_utils.get_model())
