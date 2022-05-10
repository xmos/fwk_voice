import numpy as np
import data_processing.frame_preprocessor as fp
import py_vnr.vnr as vnr
import py_vnr.run_wav_vnr as rwv
import os
import sys
sys.path.append(os.path.abspath("../feature_extraction"))
import test_utils
import tensorflow as tf
import math
import matplotlib.pyplot as plt

def test_vnr_full(tflite_model):
    np.random.seed(1243)
    vnr_obj = vnr.Vnr(model_file=tflite_model) 

    input_data = np.empty(0, dtype=np.int32)
    input_words_per_frame = fp.FRAME_ADVANCE #No. of int32 values sent to dut as input per frame
    output_words_per_frame = 2

    input_data = np.append(input_data, np.array([input_words_per_frame, output_words_per_frame], dtype=np.int32))
    min_int = -2**31
    max_int = 2**31
    test_frames = 2048
    ref_output_double = np.empty(0, dtype=np.float64)
    dut_output_double = np.empty(0, dtype=np.float64)
    x_data = np.zeros(fp.FRAME_LEN, dtype=np.float64)    

    for itt in range(0,test_frames):
        # Generate input data
        hr = np.random.randint(8)
        data = np.random.randint(min_int, high=max_int, size=fp.FRAME_ADVANCE)
        data = np.array(data, dtype=np.int32)
        data = data >> hr
        input_data = np.append(input_data, data)
        new_x_frame = test_utils.int32_to_double(data, -31)

        # Ref VNR implementation
        x_data = np.roll(x_data, -fp.FRAME_ADVANCE, axis = 0)
        x_data[fp.FRAME_LEN - fp.FRAME_ADVANCE:] = new_x_frame
        this_patch = rwv.extract_features(x_data, vnr_obj)
        ref_output_double = np.append(ref_output_double, vnr_obj.run(this_patch))

    op = test_utils.run_dut(input_data, "test_vnr_full", os.path.abspath('../../../../build/test/lib_vnr/vnr_unit_tests/full/bin/avona_test_vnr_full.xe'))
    dut_mant = op[0::2]
    dut_exp = op[1::2]
    d = dut_mant.astype(np.float64) * (2.0 ** dut_exp)
    dut_output_double = np.append(dut_output_double, d)

    for fr in range(0,test_frames):
        dut = dut_output_double[fr]
        ref = ref_output_double[fr]
        diff = np.abs(ref-dut)
        assert(diff < 0.05), "ERROR: test_vnr_inference frame {fr}. diff exceeds threshold"
    
    print("max_diff = ",np.max(np.abs(ref_output_double - dut_output_double)))
    arith_closeness, geo_closeness = test_utils.get_closeness_metric(ref_output_double, dut_output_double)
    print(f"arith_closeness = {arith_closeness}, geo_closeness = {geo_closeness}")
    assert(geo_closeness > 0.99), "inference output geo_closeness below pass threshold"
    assert(arith_closeness > 0.99), "inference output arith_closeness below pass threshold"

    plt.plot(ref_output_double, label="ref")
    plt.plot(dut_output_double, label="dut")
    plt.legend(loc="upper right")
    #plt.show()

if __name__ == "__main__":
    test_vnr_full(os.path.abspath("../../test_wav_vnr/model/model_output_0_0_2/model_qaware.tflite"))
