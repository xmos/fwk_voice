import numpy as np
import data_processing.frame_preprocessor as fp
import py_vnr.vnr as vnr
import os
import test_utils
import matplotlib.pyplot as plt

this_file_dir = os.path.dirname(os.path.realpath(__file__))
exe_dir = os.path.join(this_file_dir, '../../../../build/test/lib_vnr/vnr_unit_tests/feature_extraction/bin/')
xe = os.path.join(exe_dir, 'fwk_voice_test_vnr_priv_normalise_patch.xe')

def test_vnr_priv_add_new_slice(target, tflite_model):
    np.random.seed(1243)
    vnr_obj = vnr.Vnr(model_file=tflite_model) 

    input_data = np.empty(0, dtype=np.int32)
    input_words_per_frame = fp.MEL_FILTERS #No. of int32 values sent to dut as input per frame
    output_words_per_frame = (fp.PATCH_WIDTH * fp.MEL_FILTERS)+1

    input_data = np.append(input_data, np.array([input_words_per_frame, output_words_per_frame], dtype=np.int32))    
    min_int = -2**31
    max_int = 2**31
    test_frames = 2048

    ref_output_float = np.empty(0, dtype=np.float64)
    for itt in range(0,test_frames):
        # Generate input data
        hr = np.random.randint(8)
        data = np.random.randint(min_int, high=max_int, size=fp.MEL_FILTERS)
        data = np.array(data, dtype=np.int32)
        data = data >> hr
        input_data = np.append(input_data, data)

        # Ref form input frame implementation
        ref_new_slice = test_utils.int32_to_double(data, -24)
        vnr_obj.add_new_slice(ref_new_slice, buffer_number=0)
        normalised_patch = vnr_obj.normalise_patch(vnr_obj.feature_buffers[0])
        ref_output_float = np.append(ref_output_float, normalised_patch)

    exe_name = xe
    if(target == "x86"): #Remove the .xe extension from the xe name to get the x86 executable
        exe_name = os.path.splitext(xe)[0]
    op = test_utils.run_dut(input_data, "test_vnr_priv_normalise_patch", exe_name)

    exp_indices = np.arange(0, len(op), output_words_per_frame) # Every (257*2 + 1)th value starting from index 0 is the exponent
    exp_indices = exp_indices.astype(np.int32)
    dut_exp = op[exp_indices]
    dut_mants = np.delete(op, exp_indices)
    assert len(dut_exp) == test_frames
    
    max_diff = 0
    for fr in range(0,test_frames):
        r = ref_output_float[fr*(fp.PATCH_WIDTH * fp.MEL_FILTERS) : (fr+1)*(fp.PATCH_WIDTH * fp.MEL_FILTERS)]
        ref = test_utils.double_to_int32(r, dut_exp[fr])
        dut = dut_mants[fr*(fp.PATCH_WIDTH * fp.MEL_FILTERS) : (fr+1)*(fp.PATCH_WIDTH * fp.MEL_FILTERS)]
        diff = np.max(np.abs(ref-dut))
        assert diff<2, "ERROR: test_vnr_priv_normalise_patch diff exceeds threshold of 2"
        max_diff = max(max_diff, diff)

    print("max_diff = ",max_diff)
