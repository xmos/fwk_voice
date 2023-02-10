
import numpy as np
import data_processing.frame_preprocessor as fp
import os
import test_utils
import matplotlib.pyplot as plt

this_file_dir = os.path.dirname(os.path.realpath(__file__))
exe_dir = os.path.join(this_file_dir, '../../../../build/test/lib_vnr/vnr_unit_tests/feature_extraction/bin/')
xe = os.path.join(exe_dir, 'fwk_voice_test_vnr_priv_log2.xe')

def test_vnr_priv_log2(target):
    np.random.seed(1243)

    # No. of int32 values sent to dut as input per frame
    input_words_per_frame = fp.MEL_FILTERS*2 # MEL_FILTERS float_s32_t values 

    # No. of int32 output values expected from dut per frame
    output_words_per_frame = fp.MEL_FILTERS # MEL_FILTERS uq8_24 values. Exponent fixed to -24

    input_data = np.empty(0, dtype=np.int32)
    input_data = np.append(input_data, np.array([input_words_per_frame, output_words_per_frame], dtype=np.int32))
    min_int = -2**31
    max_int = 2**31
    test_frames = 2048

    ref_output_float = np.empty(0, dtype=np.float64)
    ref_output_int = np.empty(0, dtype=np.int32)
    dut_output_float = np.empty(0, dtype=np.float64)
    for itt in range(0,test_frames):
        hr = np.random.randint(5)
        data = np.zeros(fp.MEL_FILTERS*2, dtype=np.int32)
        data[0::2] = np.random.randint(1, high=max_int, size=fp.MEL_FILTERS) # mant
        data[0::2] = data[0::2] >> hr
        data[1::2] = np.random.randint(-32, high=16) # exp
        data = np.array(data, dtype=np.int32)
        input_data = np.append(input_data, data)

        # Ref log2 implementation
        mant = data[0::2].astype(np.float64)
        exp = data[1::2]
        ref = mant * (2.0 ** exp)
        y = np.log2(ref)
        ref_output_float = np.append(ref_output_float, y)

    exe_name = xe
    if(target == "x86"): #Remove the .xe extension from the xe name to get the x86 executable
        exe_name = os.path.splitext(xe)[0]
    op = test_utils.run_dut(input_data, "test_vnr_priv_log2", exe_name)
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
        assert(np.allclose(dut, ref, rtol=0.05)), f"ERROR: test_vnr_priv_log2 frame {fr}. relative diff exceeds rtol=0.05"
        assert(np.max(np.abs(dut-ref)) < 0.005), f"ERROR: test_vnr_priv_log2 frame {fr}. log2 diff exceeds 0.005"

    percent_diff = np.abs((dut_output_float - ref_output_float)/ref_output_float)
    print(f"max log2 output diff = {np.max(np.abs(dut_output_float - ref_output_float))}")
    print("max diff percent = ",np.max(percent_diff)*100, "all_close = ",np.allclose(dut_output_float, ref_output_float, rtol=0.05))

