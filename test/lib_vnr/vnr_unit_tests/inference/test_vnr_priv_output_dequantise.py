import numpy as np
import data_processing.frame_preprocessor as fp
import py_vnr.vnr as vnr
import os
import sys
sys.path.append(os.path.abspath("../feature_extraction"))
import test_utils
import tensorflow as tf
import math

def test_vnr_priv_output_dequantise(tflite_model):
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

    op = test_utils.run_dut(input_data, "test_vnr_priv_output_dequantise", os.path.abspath('../../../../build/test/lib_vnr/vnr_unit_tests/inference/bin/avona_test_vnr_priv_output_dequantise.xe'))
    dut_mant = op[0::2]
    dut_exp = op[1::2]
    d = dut_mant.astype(np.float64) * (2.0 ** dut_exp)
    dut_output_double = np.append(dut_output_double, d)
    for fr in range(0,test_frames):
        dut = dut_mant[fr]
        ref = int(ref_output_double[fr] * (2.0 ** -dut_exp[fr]))
        diff = np.abs(ref-dut)
        assert(diff < 1)
    
    print("max_diff = ",np.max(np.abs(ref_output_double - dut_output_double)))


if __name__ == "__main__":
    test_vnr_priv_output_dequantise(os.path.abspath("../../test_wav_vnr/model/model_output_0_0_2/model_qaware.tflite"))
