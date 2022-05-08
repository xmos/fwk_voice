import numpy as np
import data_processing.frame_preprocessor as fp
import py_vnr.vnr as vnr
import os
import test_utils
import matplotlib.pyplot as plt

def test_vnr_priv_make_slice(tflite_model):
    np.random.seed(1243)
    vnr_obj = vnr.Vnr(model_file=tflite_model) 

    input_data = np.empty(0, dtype=np.int32)
    input_words_per_frame = fp.FRAME_ADVANCE #No. of int32 values sent to dut as input per frame
    output_words_per_frame = fp.MEL_FILTERS # MEL_FILTERS fixed_s32_t values. Exponent fixed to -24

    fd_frame_len = int(fp.NFFT/2 + 1)
    input_data = np.append(input_data, np.array([input_words_per_frame, output_words_per_frame], dtype=np.int32))    
    min_int = -2**31
    max_int = 2**31
    test_frames = 2048

    x_data = np.zeros(fp.FRAME_LEN, dtype=np.float64)    
    ref_output_float = np.empty(0, dtype=np.float64)
    ref_output_int = np.empty(0, dtype=np.int32)
    dut_output_float = np.empty(0, dtype=np.float64)
    for itt in range(0,test_frames):
        # Generate input data
        hr = np.random.randint(8)
        data = np.random.randint(min_int, high=max_int, size=fp.FRAME_ADVANCE)
        data = np.array(data, dtype=np.int32)
        data = data >> hr
        input_data = np.append(input_data, data)

        # Ref form input frame implementation
        new_x_frame = data.astype(np.float64) * (2.0 ** -31) 
        x_data = np.roll(x_data, -fp.FRAME_ADVANCE, axis = 0)
        x_data[fp.FRAME_LEN - fp.FRAME_ADVANCE:] = new_x_frame
        X_spect = np.fft.rfft(x_data, fp.NFFT)
        new_slice = vnr_obj.make_slice(X_spect)

        ref_output_float = np.append(ref_output_float, new_slice)
        
    op = test_utils.run_dut(input_data, "test_vnr_priv_make_slice", os.path.abspath('../../../../build/test/lib_vnr/vnr_unit_tests/feature_extraction/bin/avona_test_vnr_priv_make_slice.xe'))
    dut_output_int = op.astype(np.int32)
    dut_mant = op.astype(np.float64)
    dut_exp = -24 # dut output is always 8.24
    d = test_utils.int32_to_double(dut_mant, dut_exp)
    dut_output_float = np.append(dut_output_float, d)
    ref_output_int = test_utils.double_to_int32(ref_output_float, -24)
    for fr in range(0, test_frames):
        dut = dut_output_float[fr*fp.MEL_FILTERS : (fr+1)*fp.MEL_FILTERS]
        ref = ref_output_float[fr*fp.MEL_FILTERS : (fr+1)*fp.MEL_FILTERS]
        percent_diff = np.abs((dut_output_float - ref_output_float)/ref_output_float)
        assert(np.allclose(dut, ref, rtol=0.05)), "ERROR: test_vnr_priv_make_slice relative diff exceeds rtol=0.05"
        assert(np.max(np.abs(dut-ref)) < 0.005), f"ERROR: test_vnr_priv_make_slice frame {fr}. make_slice diff exceeds 0.005"

    percent_diff = np.abs((dut_output_float - ref_output_float)/ref_output_float)
    print(f"max make_slice output diff = {np.max(np.abs(dut_output_float - ref_output_float))}")
    print(f"max diff percent = {np.max(percent_diff)*100}%", " max int diff = ", "all_close = ",np.allclose(dut_output_float, ref_output_float, rtol=0.05))
    plt.plot(ref_output_float)
    plt.plot(dut_output_float)
    #plt.show()

