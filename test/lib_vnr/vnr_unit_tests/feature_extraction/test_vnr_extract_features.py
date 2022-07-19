import numpy as np
import data_processing.frame_preprocessor as fp
import py_vnr.vnr as vnr
import py_vnr.run_wav_vnr as rwv
import os
import test_utils
import matplotlib.pyplot as plt
import tensorflow as tf

this_file_dir = os.path.dirname(os.path.realpath(__file__))
exe_dir = os.path.join(this_file_dir, '../../../../build/test/lib_vnr/vnr_unit_tests/feature_extraction/bin/')
xe = os.path.join(exe_dir, 'fwk_voice_test_vnr_extract_features.xe')

def test_vnr_extract_features(target, tflite_model, verbose=False):
    np.random.seed(1243)
    vnr_obj = vnr.Vnr(model_file=tflite_model) 

    input_data = np.empty(0, dtype=np.int32)
    input_words_per_frame = fp.FRAME_ADVANCE + 1#No. of int32 values sent to dut as input per frame

    norm_patch_output_len = (fp.PATCH_WIDTH * fp.MEL_FILTERS)+1
    quant_patch_output_len = (fp.PATCH_WIDTH * fp.MEL_FILTERS)/4
    output_subsets_len = [norm_patch_output_len, quant_patch_output_len]
    output_words_per_frame =  norm_patch_output_len +  quant_patch_output_len #Both normalised and quantised patches sent as output

    input_data = np.append(input_data, np.array([input_words_per_frame, output_words_per_frame], dtype=np.int32))    
    min_int = -2**31
    max_int = 2**31
    test_frames = 1024
    ref_normalised_output = np.empty(0, dtype=np.float64)
    dut_normalised_output = np.empty(0, dtype=np.float64)

    x_data = np.zeros(fp.FRAME_LEN, dtype=np.float64)    
    for itt in range(0,test_frames):
        enable_highpass = np.random.randint(2)
        # Generate input data
        hr = np.random.randint(8)
        data = np.random.randint(min_int, high=max_int, size=fp.FRAME_ADVANCE)
        data = np.array(data, dtype=np.int32)
        data = data >> hr
        input_data = np.append(input_data, data)
        input_data = np.append(input_data, enable_highpass)
        new_x_frame = test_utils.int32_to_double(data, -31)

        # Ref form input frame implementation
        x_data = np.roll(x_data, -fp.FRAME_ADVANCE, axis = 0)
        x_data[fp.FRAME_LEN - fp.FRAME_ADVANCE:] = new_x_frame
        normalised_patch = rwv.extract_features(x_data, vnr_obj, hp=enable_highpass)
        ref_normalised_output = np.append(ref_normalised_output, normalised_patch)
        
    ref_quantised_output = test_utils.quantise_patch(tflite_model, ref_normalised_output)
    exe_name = xe
    if(target == "x86"): #Remove the .xe extension from the xe name to get the x86 executable
        exe_name = os.path.splitext(xe)[0]
    op = test_utils.run_dut(input_data, "test_vnr_extract_features", exe_name)
    # Deinterleave dut output into normalised and quantised patches
    sections = []
    ii = 0
    count = 0
    while count < len(op):
        count = count + output_subsets_len[ii]
        sections.append(int(count))
        ii = (ii+1)%2
    
    op_split = np.split(op, sections)
    op_norm_patch = np.concatenate(([a for a in op_split[0::2]]))
    op_quant_patch = np.concatenate(([a for a in op_split[1::2]]))
    dut_quantised_output = op_quant_patch.view(np.int8)
    
    # Deinterleave normalised_patch exponent and mantissas
    exp_indices = np.arange(0, len(op_norm_patch), norm_patch_output_len) # One exponent for 96 mantissas block
    exp_indices = exp_indices.astype(np.int32)
    dut_exp = op_norm_patch[exp_indices]
    dut_mants = np.delete(op_norm_patch, exp_indices)
    assert len(dut_exp) == test_frames
    
    for fr in range(0,test_frames):
        # Compare normalised output
        dut = test_utils.int32_to_double(dut_mants[fr*(fp.PATCH_WIDTH * fp.MEL_FILTERS) : (fr+1)*(fp.PATCH_WIDTH * fp.MEL_FILTERS)], dut_exp[fr])
        ref = ref_normalised_output[fr*(fp.PATCH_WIDTH * fp.MEL_FILTERS) : (fr+1)*(fp.PATCH_WIDTH * fp.MEL_FILTERS)]
        dut_normalised_output = np.append(dut_normalised_output, dut)
        
        diff = np.abs((dut - ref))
        relative_diff = np.abs((dut - ref)/(ref+np.finfo(float).eps))
        assert(np.max(diff) < 0.005), f"ERROR: test_vnr_extract_features. frame {fr} normalised features max diff exceeds threshold" 
        assert(np.max(relative_diff) < 0.15), f"ERROR: test_vnr_extract_features. frame {fr} normalised features max relative diff exceeds threshold" 
        if verbose:
            ii = np.where(relative_diff > 0.05) 
            if len(ii[0]):
                for index in ii[0]:
                    print(f"frame {fr}. diff = {diff[index]}, ref {ref[index]} dut {dut[index]}")
        
        arith_closeness, geo_closeness = test_utils.get_closeness_metric(ref, dut)
        assert(geo_closeness > 0.999), f"ERROR: frame {fr}. normalised_output geo_closeness below pass threshold"
        assert(arith_closeness > 0.999), f"ERROR: frame {fr}. normalised_output arith_closeness below pass threshold"

        # Compare quantised output
        dut = dut_quantised_output[fr*(fp.PATCH_WIDTH * fp.MEL_FILTERS) : (fr+1)*(fp.PATCH_WIDTH * fp.MEL_FILTERS)]
        ref = ref_quantised_output[fr*(fp.PATCH_WIDTH * fp.MEL_FILTERS) : (fr+1)*(fp.PATCH_WIDTH * fp.MEL_FILTERS)]
        diff = np.abs((dut - ref))
        assert(np.max(diff) < 2)
    
    diff = np.abs((dut_normalised_output - ref_normalised_output))
    percent_diff = np.abs((dut_normalised_output - ref_normalised_output)/(ref_normalised_output+np.finfo(float).eps))
    
    print(f"max diff normalised patch = {np.max(diff)}")
    print(f"max diff percent normalised patch = {np.max(percent_diff)*100}%")

    print(f"max diff quantised output = {np.max(np.abs(dut_quantised_output - ref_quantised_output))}")

    plt.plot(ref_normalised_output)
    plt.plot(dut_normalised_output)
    #plt.show()
