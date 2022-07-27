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
import matplotlib.pyplot as plt

exe_dir = os.path.join(this_file_dir, '../../../../build/test/lib_vnr/vnr_unit_tests/inference/bin/')
xe = os.path.join(exe_dir, 'fwk_voice_test_vnr_inference.xe')

def test_vnr_inference(target, tflite_model):
    np.random.seed(1243)
    vnr_obj = vnr.Vnr(model_file=tflite_model) 

    input_data = np.empty(0, dtype=np.int32)
    input_words_per_frame = (fp.PATCH_WIDTH * fp.MEL_FILTERS)+1 # 96 mantissas and 1 exponent
    output_words_per_frame = 2
    input_data = np.append(input_data, np.array([input_words_per_frame, output_words_per_frame], dtype=np.int32))

    min_int = -2**31
    max_int = 0 # Normalised features are all negative with a max of 0
    test_frames = 4096
    ref_output_double = np.empty(0, dtype=np.float64)
    dut_output_double = np.empty(0, dtype=np.float64)
    for itt in range(0,test_frames):
        # By setting high=1 we enure no value is greater than 0 since max normalised output is 0
        data = np.random.randint(min_int, high=max_int+1, size=fp.PATCH_WIDTH * fp.MEL_FILTERS)
        exp = np.random.randint(-31, high=0) # exp
        input_data = np.append(input_data, exp)
        input_data = np.append(input_data, data)
        # Ref implementation
        this_patch = test_utils.int32_to_double(data, exp)
        this_patch = this_patch.reshape(1, 1, fp.PATCH_WIDTH, fp.MEL_FILTERS)
        ref_output_double = np.append(ref_output_double, vnr_obj.run(this_patch))

    exe_name = xe
    if(target == "x86"): #Remove the .xe extension from the xe name to get the x86 executable
        exe_name = os.path.splitext(xe)[0]
    op = test_utils.run_dut(input_data, "test_vnr_inference", exe_name)
    dut_mant = op[0::2]
    dut_exp = op[1::2]
    d = dut_mant.astype(np.float64) * (2.0 ** dut_exp)
    dut_output_double = np.append(dut_output_double, d)

    for fr in range(0,test_frames):
        dut = dut_output_double[fr]
        ref = ref_output_double[fr]
        diff = np.abs(ref-dut)
        assert(diff < 0.05), f"ERROR: test_vnr_inference frame {fr}. diff {diff} exceeds threshold"
    
    print("max_diff = ",np.max(np.abs(ref_output_double - dut_output_double)))
    arith_closeness, geo_closeness = test_utils.get_closeness_metric(ref_output_double, dut_output_double)
    print(f"arith_closeness = {arith_closeness}, geo_closeness = {geo_closeness}")
    assert(geo_closeness > 0.98), "inference output geo_closeness below pass threshold"
    assert(arith_closeness > 0.95), "inference output arith_closeness below pass threshold"

    plt.plot(ref_output_double, label="ref")
    plt.plot(dut_output_double, label="dut")
    plt.legend(loc="upper right")
    #plt.show()

if __name__ == "__main__":
    test_vnr_inference("xcore", test_utils.get_model())
