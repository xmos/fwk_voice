import numpy as np
import data_processing.frame_preprocessor as fp
import py_vnr.vnr as vnr
import os
import sys
sys.path.append(os.path.abspath("../feature_extraction"))
import test_utils
import tensorflow as tf
import math
import matplotlib.pyplot as plt

def test_vnr_inference(tflite_model):
    np.random.seed(1243)
    vnr_obj = vnr.Vnr(model_file=tflite_model) 

    input_data = np.empty(0, dtype=np.int32)
    input_words_per_frame = (fp.PATCH_WIDTH * fp.MEL_FILTERS)+1 # 96 mantissas and 1 exponent
    output_words_per_frame = 2
    input_data = np.append(input_data, np.array([input_words_per_frame, output_words_per_frame], dtype=np.int32))

    min_mant, min_exp = test_utils.get_feature_patch_min_limit(tflite_model) # Exponent and mantissa of the minimum limit of the normalised patch. Quantisation will not work correctly for anything below this
    print(f"min_mant = {min_mant}, min_exp = {min_exp}, {hex(min_mant)}, min = {math.ldexp(min_mant, min_exp)}")
    min_int = -2**31
    max_int = 0 # Normalised features are all negative with a max of 0
    test_frames = 4096
    ref_output_double = np.empty(0, dtype=np.float64)
    dut_output_double = np.empty(0, dtype=np.float64)
    for itt in range(0,test_frames):
        # By setting high=1 we enure no value is greater than 0 since max normalised output is 0
        data = np.random.randint(min_int, high=max_int+1, size=fp.PATCH_WIDTH * fp.MEL_FILTERS)
        exp = np.random.randint(-31, high=min_exp+8) # exp
        # Make sure no value is less than ldexp(min_mant, min_exp)
        if exp >= min_exp:
            #print(f"fr {itt}, exp = ",exp)
            for i in range(len(data)):
                rsh = exp-min_exp
                if(data[i] <= (min_mant >> rsh)):                    
                    data[i] = min_mant >> rsh
        input_data = np.append(input_data, exp)
        input_data = np.append(input_data, data)
        # Ref implementation
        this_patch = test_utils.int32_to_double(data, exp)
        this_patch = this_patch.reshape(1, 1, fp.PATCH_WIDTH, fp.MEL_FILTERS)
        ref_output_double = np.append(ref_output_double, vnr_obj.run(this_patch))

    op = test_utils.run_dut(input_data, "test_vnr_inference", os.path.abspath('../../../../build/test/lib_vnr/vnr_unit_tests/inference/bin/avona_test_vnr_inference.xe'))
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
    test_vnr_inference(os.path.abspath("../../test_wav_vnr/model/model_output_0_0_2/model_qaware.tflite"))
