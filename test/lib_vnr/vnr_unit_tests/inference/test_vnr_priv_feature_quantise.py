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
xe = os.path.join(exe_dir, 'avona_test_vnr_priv_feature_quantise.xe')

def test_vnr_priv_feature_quantise(tflite_model):
    np.random.seed(1243)
    vnr_obj = vnr.Vnr(model_file=tflite_model) 

    input_data = np.empty(0, dtype=np.int32)
    input_words_per_frame = (fp.PATCH_WIDTH * fp.MEL_FILTERS)+1 # 96 mantissas and 1 exponent
    output_words_per_frame = (fp.PATCH_WIDTH * fp.MEL_FILTERS)/4 # 96 int8 values
    input_data = np.append(input_data, np.array([input_words_per_frame, output_words_per_frame], dtype=np.int32))

    min_mant, min_exp = test_utils.get_feature_patch_min_limit(tflite_model) # Exponent and mantissa of the minimum limit of the normalised patch. Quantisation will not work correctly for anything below this
    print(f"min_mant = {min_mant}, min_exp = {min_exp}, {hex(min_mant)}, min = {math.ldexp(min_mant, min_exp)}")
    min_int = -2**31
    max_int = 0 # Normalised features are all negative with a max of 0
    test_frames = 2048
    ref_output = np.empty(0, dtype=np.int8)
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
        quant_patch = test_utils.quantise_patch(tflite_model, this_patch)
        ref_output = np.append(ref_output, quant_patch)

    op = test_utils.run_dut(input_data, "test_vnr_priv_feature_quantise", xe)
    dut_output = op.view(np.int8)

    for fr in range(0,test_frames):
        ref = ref_output[fr*(fp.PATCH_WIDTH * fp.MEL_FILTERS) : (fr+1)*(fp.PATCH_WIDTH * fp.MEL_FILTERS)]
        dut = dut_output[fr*(fp.PATCH_WIDTH * fp.MEL_FILTERS) : (fr+1)*(fp.PATCH_WIDTH * fp.MEL_FILTERS)]
        diff = np.max(np.abs(ref-dut))
        assert(diff < 1), "ERROR: test_vnr_priv_feature_quantise frame {fr}. diff exceeds 0"

    print("max_diff = ",np.max(np.abs(ref_output-dut_output)))

if __name__ == "__main__":
    test_vnr_priv_feature_quantise(os.path.abspath("../../test_wav_vnr/model/model_output_0_0_2/model_qaware.tflite"))
