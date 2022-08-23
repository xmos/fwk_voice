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
xe = os.path.join(exe_dir, 'fwk_voice_test_vnr_priv_feature_quantise.xe')

def test_vnr_priv_feature_quantise(target, tflite_model):
    np.random.seed(1243)
    vnr_obj = vnr.Vnr(model_file=tflite_model) 

    input_data = np.empty(0, dtype=np.int32)
    input_words_per_frame = (fp.PATCH_WIDTH * fp.MEL_FILTERS)+1 # 96 mantissas and 1 exponent
    output_words_per_frame = (fp.PATCH_WIDTH * fp.MEL_FILTERS)/4 # 96 int8 values
    input_data = np.append(input_data, np.array([input_words_per_frame, output_words_per_frame], dtype=np.int32))

    min_int = -2**31
    max_int = 0 # Normalised features are all negative with a max of 0
    test_frames = 2048
    ref_output = np.empty(0, dtype=np.int8)
    for itt in range(0,test_frames):
        # By setting high=1 we enure no value is greater than 0 since max normalised output is 0
        data = np.random.randint(min_int, high=max_int+1, size=fp.PATCH_WIDTH * fp.MEL_FILTERS)
        exp = np.random.randint(-31, high=0) # exp
        input_data = np.append(input_data, exp)
        input_data = np.append(input_data, data)
        # Ref implementation
        this_patch = test_utils.int32_to_double(data, exp)
        quant_patch = test_utils.quantise_patch(tflite_model, this_patch)
        ref_output = np.append(ref_output, quant_patch)

    exe_name = xe
    if(target == "x86"): #Remove the .xe extension from the xe name to get the x86 executable
        exe_name = os.path.splitext(xe)[0]
    op = test_utils.run_dut(input_data, "test_vnr_priv_feature_quantise", exe_name)
    dut_output = op.view(np.int8)

    for fr in range(0,test_frames):
        ref = ref_output[fr*(fp.PATCH_WIDTH * fp.MEL_FILTERS) : (fr+1)*(fp.PATCH_WIDTH * fp.MEL_FILTERS)]
        dut = dut_output[fr*(fp.PATCH_WIDTH * fp.MEL_FILTERS) : (fr+1)*(fp.PATCH_WIDTH * fp.MEL_FILTERS)]
        diff = np.max(np.abs(ref-dut))
        assert(diff < 1), f"ERROR: test_vnr_priv_feature_quantise frame {fr}. diff {diff} exceeds 0"

    print("max_diff = ",np.max(np.abs(ref_output-dut_output)))

if __name__ == "__main__":
    test_vnr_priv_feature_quantise("xcore", test_utils.get_model())
