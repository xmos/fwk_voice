import numpy as np
import data_processing.frame_preprocessor as fp
import py_vnr.vnr as vnr
import os
import test_utils
import matplotlib.pyplot as plt

this_file_dir = os.path.dirname(os.path.realpath(__file__))
exe_dir = os.path.join(this_file_dir, '../../../../build/test/lib_vnr/vnr_unit_tests/feature_extraction/bin/')
xe = os.path.join(exe_dir, 'fwk_voice_test_vnr_priv_make_slice.xe')

def test_vnr_priv_make_slice(target, tflite_model):
    np.random.seed(1243)
    vnr_obj = vnr.Vnr(model_file=tflite_model) 

    input_data = np.empty(0, dtype=np.int32)
    input_words_per_frame = fp.FRAME_ADVANCE + 1 #No. of int32 values sent to dut as input per frame
    output_words_per_frame = fp.MEL_FILTERS # MEL_FILTERS uq8_24 values. Exponent fixed to -24

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
        enable_highpass = np.random.randint(2)
        # Generate input data
        hr = np.random.randint(8)
        data = np.random.randint(min_int, high=max_int, size=fp.FRAME_ADVANCE)
        data = np.array(data, dtype=np.int32)
        data = data >> hr
        input_data = np.append(input_data, data)
        input_data = np.append(input_data, enable_highpass);

        # Ref form input frame implementation
        new_x_frame = data.astype(np.float64) * (2.0 ** -31) 
        x_data = np.roll(x_data, -fp.FRAME_ADVANCE, axis = 0)
        x_data[fp.FRAME_LEN - fp.FRAME_ADVANCE:] = new_x_frame
        X_spect = np.fft.rfft(x_data, fp.NFFT)
        new_slice = vnr_obj.make_slice(X_spect, enable_highpass)

        ref_output_float = np.append(ref_output_float, new_slice)
        
    exe_name = xe
    if(target == "x86"): #Remove the .xe extension from the xe name to get the x86 executable
        exe_name = os.path.splitext(xe)[0]
    op = test_utils.run_dut(input_data, "test_vnr_priv_make_slice", exe_name)
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

