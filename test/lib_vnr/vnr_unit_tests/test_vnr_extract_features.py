import numpy as np
import data_processing.frame_preprocessor as fp
import py_vnr.vnr as vnr
import py_vnr.run_wav_vnr as rwv
import os
import test_utils
import matplotlib.pyplot as plt
import scipy.io.wavfile
import sys
sys.path.append(os.path.join(os.getcwd(), "../../shared/python"))
import py_vs_c_utils as pvc

tflite_model = os.path.abspath("../test_wav_vnr/model/model_output_0_0_2/model_qaware.tflite")
def test_vnr_extract_features():
    np.random.seed(1243)
    vnr_obj = vnr.Vnr(model_file=tflite_model) 

    input_data = np.empty(0, dtype=np.int32)
    input_words_per_frame = fp.FRAME_ADVANCE #No. of int32 values sent to dut as input per frame
    output_words_per_frame = (fp.PATCH_WIDTH * fp.MEL_FILTERS)+1

    fd_frame_len = int(fp.NFFT/2 + 1)
    input_data = np.append(input_data, np.array([input_words_per_frame, output_words_per_frame], dtype=np.int32))    
    min_int = -2**31
    max_int = 2**31
    test_frames = 1024
    ref_output_float = np.empty(0, dtype=np.float64)
    dut_output_float = np.empty(0, dtype=np.float64)

    x_data = np.zeros(fp.FRAME_LEN, dtype=np.float64)    
    for itt in range(0,test_frames):
        # Generate input data
        hr = np.random.randint(8)
        data = np.random.randint(min_int, high=max_int, size=fp.FRAME_ADVANCE)
        data = np.array(data, dtype=np.int32)
        data = data >> hr
        input_data = np.append(input_data, data)
        new_x_frame = data.astype(np.float64) * (2.0 ** -31) 

        # Ref form input frame implementation
        x_data = np.roll(x_data, -fp.FRAME_ADVANCE, axis = 0)
        x_data[fp.FRAME_LEN - fp.FRAME_ADVANCE:] = new_x_frame
        normalised_patch = rwv.extract_features(x_data, vnr_obj)
        ref_output_float = np.append(ref_output_float, normalised_patch)

    op = test_utils.run_dut(input_data, "test_vnr_extract_features", os.path.abspath('../../../build/test/lib_vnr/vnr_unit_tests/bin/avona_test_vnr_extract_features.xe'))

    exp_indices = np.arange(0, len(op), output_words_per_frame) # Every (257*2 + 1)th value starting from index 0 is the exponent
    exp_indices = exp_indices.astype(np.int32)
    dut_exp = op[exp_indices]
    dut_mants = np.delete(op, exp_indices)
    assert len(dut_exp) == test_frames
    
    for fr in range(0,test_frames):
        dut = test_utils.int32_to_double(dut_mants[fr*(fp.PATCH_WIDTH * fp.MEL_FILTERS) : (fr+1)*(fp.PATCH_WIDTH * fp.MEL_FILTERS)], dut_exp[fr])
        ref = ref_output_float[fr*(fp.PATCH_WIDTH * fp.MEL_FILTERS) : (fr+1)*(fp.PATCH_WIDTH * fp.MEL_FILTERS)]
        #print(dut_exp[fr])
        dut_output_float = np.append(dut_output_float, dut)
        #print(dut_output_float)
        #print(np.abs(np.divide(ref_output_float - dut_output_float, ref_output_float+np.finfo(float).eps)))
        output_file = "temp.wav"
        # new_slice
        output_wav_data = np.zeros((2, len(ref)))
        output_wav_data[0,:] = ref
        output_wav_data[1,:] = dut
        scipy.io.wavfile.write(output_file, 16000, output_wav_data.T)
        arith_closeness, geo_closeness, c_delay, peak2ave = pvc.pcm_closeness_metric(output_file)
        assert(geo_closeness > 0.998), "new_slice geo_closeness below pass threshold"
        assert(arith_closeness > 0.998), "new_slice arith_closeness below pass threshold"
    
    diff = np.abs((dut_output_float - ref_output_float))
    percent_diff = np.abs((dut_output_float - ref_output_float)/(ref_output_float+np.finfo(float).eps))
    
    print(f"max diff = {np.max(diff)}")
    print(f"max diff percent = {np.max(percent_diff)*100}%")

    plt.plot(ref_output_float)
    plt.plot(dut_output_float)
    #plt.show()

if __name__ == "__main__":
    test_vnr_extract_features()
