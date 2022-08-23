
import numpy as np
import data_processing.frame_preprocessor as fp
import os
import test_utils
import py_vnr.vnr as vnr
import matplotlib.pyplot as plt

this_file_dir = os.path.dirname(os.path.realpath(__file__))
exe_dir = os.path.join(this_file_dir, '../../../../build/test/lib_vnr/vnr_unit_tests/feature_extraction/bin/')
xe = os.path.join(exe_dir, 'fwk_voice_test_vnr_priv_mel_compute.xe')

def test_vnr_priv_mel_compute(target, tflite_model):
    np.random.seed(1243)
    vnr_obj = vnr.Vnr(model_file=tflite_model) 
    
    input_data = np.empty(0, dtype=np.int32)

    fd_frame_len = int(fp.NFFT/2 + 1)
    # No. of int32 values sent to dut as input per frame
    input_words_per_frame = fd_frame_len*2 + 1 #fd_frame_len complex values and 1 exponent per frame

    # No. of int32 output values expected from dut per frame (257 complex data values and 1 exponent)
    output_words_per_frame = fp.MEL_FILTERS*2 #float_s32_t y[MEL_FILTERS]

    input_data = np.append(input_data, np.array([input_words_per_frame, output_words_per_frame], dtype=np.int32))
    min_int = -2**31
    max_int = 2**31
    test_frames = 2048
    
    ref_output_float = np.empty(0, dtype=np.float64)
    for itt in range(0,test_frames):
        hr = np.random.randint(5)
        exp = np.random.randint(-32, high=-8)
        data = np.random.randint(min_int, high=max_int, size=fd_frame_len*2)
        data = np.array(data, dtype=np.int32)
        data = data >> hr
        input_data = np.append(input_data, exp)
        input_data = np.append(input_data, data)

        # Ref Mel filtering implementation
        X_spect = test_utils.int32_to_double(data, exp)
        X_spect = X_spect.astype(np.float64).view(np.complex128)

        out_spect = np.abs(X_spect)**2
        out_spect = np.dot(out_spect, vnr_obj.mel_fbank)

        ref_output_float = np.append(ref_output_float, out_spect)

    exe_name = xe
    if(target == "x86"): #Remove the .xe extension from the xe name to get the x86 executable
        exe_name = os.path.splitext(xe)[0]
    op = test_utils.run_dut(input_data, "test_vnr_priv_mel_compute", exe_name)
    mant = op[0::2].astype(np.float64)
    exp = op[1::2]
    dut_output_float = mant * (2.0 ** exp)
    dut_output_int = mant
    ref_output_int = (ref_output_float * (2.0 ** -exp)).astype(np.int32)
    for fr in range(0, test_frames):
        ref = ref_output_int[fr*fp.MEL_FILTERS : (fr+1)*fp.MEL_FILTERS] 
        dut = dut_output_int[fr*fp.MEL_FILTERS : (fr+1)*fp.MEL_FILTERS]
        diff = np.max(np.abs(ref - dut))
        assert(diff<16), f"ERROR: test_vnr_priv_mel_compute diff {diff} exceeds threshold"
    
    assert(np.allclose(dut_output_float, ref_output_float))
    print(f"allclose = {np.allclose(dut_output_float, ref_output_float)}")
    print(f"Max diff = {np.max(np.abs(dut_output_int - ref_output_int))}")

