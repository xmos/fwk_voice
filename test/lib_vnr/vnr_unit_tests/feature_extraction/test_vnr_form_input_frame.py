import numpy as np
import data_processing.frame_preprocessor as fp
import os
import test_utils

this_file_dir = os.path.dirname(os.path.realpath(__file__))
exe_dir = os.path.join(this_file_dir, '../../../../build/test/lib_vnr/vnr_unit_tests/feature_extraction/bin/')
xe = os.path.join(exe_dir, 'fwk_voice_test_vnr_form_input_frame.xe')

def test_vnr_form_input_frame(target):
    np.random.seed(1243)
    input_data = np.empty(0, dtype=np.int32)
    input_words_per_frame = fp.FRAME_ADVANCE #No. of int32 values sent to dut as input per frame

    fd_frame_len = int(fp.NFFT/2 + 1)
    output_words_per_frame = fd_frame_len*2 + 1 # No. of int32 output values expected from dut per frame (257 complex data values and 1 exponent)

    input_data = np.append(input_data, np.array([input_words_per_frame, output_words_per_frame], dtype=np.int32))
    min_int = -2**31
    max_int = 2**31
    test_frames = 2048

    x_data = np.zeros(fp.FRAME_LEN, dtype=np.float64)    
    ref_output = np.empty(0, dtype=np.float64)
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
        X_spect = np.fft.rfft(x_data, fp.NFFT)
        ref_output = np.append(ref_output, X_spect)
    
    exe_name = xe
    if(target == "x86"): #Remove the .xe extension from the xe name to get the x86 executable
        exe_name = os.path.splitext(xe)[0]
    op = test_utils.run_dut(input_data, "test_vnr_form_input_frame", exe_name) # dut data has exponent followed by 257*2 data values
    
    # Separate out mantissas and exponents
    exp_indices = np.arange(0, len(op), output_words_per_frame) # Every (257*2 + 1)th value starting from index 0 is the exponent
    exp_indices = exp_indices.astype(np.int32)
    exp = op[exp_indices]
    mants = np.delete(op, exp_indices)
    frames = len(exp)

    max_diff_real = 0
    max_diff_imag = 0
    # Convert to floating point
    for i in range(frames):
        m = (mants[i*fd_frame_len*2 : (i+1)*(fd_frame_len*2)]) # Interleaved real and imaginary dut output
        ref_real = test_utils.double_to_int32(ref_output[i*fd_frame_len : (i+1)*fd_frame_len].real, exp[i])
        dut_real = m[0::2]
        diff = np.max(np.abs(ref_real - dut_real))
        max_diff_real = max(max_diff_real, diff)
        assert diff < 100, "test_vnr_form_input_frame: real diff exceeds 100"

        ref_imag = test_utils.double_to_int32(ref_output[i*fd_frame_len : (i+1)*fd_frame_len].imag, exp[i])
        dut_imag = m[1::2]
        diff = np.max(np.abs(ref_imag - dut_imag))
        assert diff < 100, "test_vnr_form_input_frame: imag diff exceeds 100"
        max_diff_imag = max(max_diff_imag, diff)

    print(f"max_diff: real {max_diff_real}, imag {max_diff_imag}")
