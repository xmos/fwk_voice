import numpy as np
import os
import sys
import matplotlib.pyplot as plt

this_file_dir = os.path.dirname(os.path.realpath(__file__))
sys.path.append(os.path.join(this_file_dir, "../../lib_vnr/vnr_unit_tests/feature_extraction"))

import test_utils # Use vnr test's test_utils
import IC
from common_utils import json_to_dict
exe_dir = os.path.join(this_file_dir, '../../../build/test/lib_ic/test_calc_vnr_pred/bin/')
xe = os.path.join(exe_dir, 'fwk_voice_test_calc_vnr_pred.xe')

def test_calc_vnr_pred(target, tflite_model, show_plot=False):
    np.random.seed(12345)
    ic_parameters = json_to_dict("../../shared/config/ic_conf_no_adapt_control.json")
    ic_parameters["adaption_config"] = "IC_ADAPTION_AUTO"
    ic_parameters["vnr_model"] = str(tflite_model)
    ifc = IC.adaptive_interference_canceller(**ic_parameters)

    # No. of int32 values sent to dut as input per frame
    input_words_per_frame = (1+(ifc.f_bin_count*2))*2 # Y_data exponent followed by ifc.f_bin_count complex values, followed by Error exponent followed by ifc.f_bin_count complex values
    output_words_per_frame = 2*2 # DUT outputs 2 float_s32_t values. input_vnr_pred followed by output_vnr_pred
    input_data = np.empty(0, dtype=np.int32)
    input_data = np.append(input_data, np.array([input_words_per_frame, output_words_per_frame], dtype=np.int32))    
    
    ref_input_vnr_pred = np.empty(0, dtype=np.float64)
    ref_output_vnr_pred = np.empty(0, dtype=np.float64)
    test_frames = 2048
    min_int = -2**31
    max_int = 2**31
    for itt in range(0, test_frames): 
        # Generate random Y_data values
        hr = np.random.randint(5)
        exp = np.random.randint(-32, high=0)
        data = np.random.randint(min_int, high=max_int, size=ifc.f_bin_count*2)
        data = data >> hr
        data[1] = 0
        data[-1] = 0 #Im part of DC and nyquist is 0
        input_data = np.append(input_data, int(exp))
        input_data = np.append(input_data, data)
        data = test_utils.int32_to_double(data, exp)
        ifc.Y_data[0] = data.astype(np.float64).view(np.complex128) # Reference Y_data

        # Generate random Error values
        hr = np.random.randint(5)
        exp = np.random.randint(-32, high=0)
        data = np.random.randint(min_int, high=max_int, size=ifc.f_bin_count*2)
        data = data >> hr
        data[1] = 0
        data[-1] = 0 #Im part of DC and nyquist is 0
        input_data = np.append(input_data, int(exp))
        input_data = np.append(input_data, data)
        data = test_utils.int32_to_double(data, exp)
        Error_ap = data.astype(np.float64).view(np.complex128)
        Error_ap = Error_ap.reshape((1, len(Error_ap))) # Reference Error values

        # Call Reference calc_vnr_pred()
        input_vnr_pred, output_vnr_pred = ifc.calc_vnr_pred(Error_ap)
        ref_input_vnr_pred = np.append(ref_input_vnr_pred, input_vnr_pred[0])
        ref_output_vnr_pred = np.append(ref_output_vnr_pred, output_vnr_pred[0])
    
    # Run DUT
    exe_name = xe
    if(target == "x86"): #Remove the .xe extension from the xe name to get the x86 executable
        exe_name = os.path.splitext(xe)[0]
    op = test_utils.run_dut(input_data, "test_calc_vnr_pred", exe_name)
    # Parse Output from DUT. Inteleaved: input_pred_mant, input_pred_exp, output_pred_mant, output_pred_exp, input_pred_mant,..
    mants = op[0::2]
    input_pred_mants = mants[0::2]
    output_pred_mants = mants[1::2]
    exps = op[1::2]
    input_pred_exps = exps[0::2]
    output_pred_exps = exps[1::2]
    dut_input_vnr_pred = input_pred_mants.astype(np.float64) * (2.0 ** input_pred_exps) 
    dut_output_vnr_pred = output_pred_mants.astype(np.float64) * (2.0 ** output_pred_exps) 
    
    # Compare dut-ref
    input_vnr_pred_diff = np.max(np.abs(ref_input_vnr_pred - dut_input_vnr_pred))
    output_vnr_pred_diff = np.max(np.abs(ref_output_vnr_pred - dut_output_vnr_pred))
    print(f"input_vnr_pred diff = {input_vnr_pred_diff}")
    print(f"output_vnr_pred diff = {output_vnr_pred_diff}")
    assert(input_vnr_pred_diff < 0.005) 
    assert(output_vnr_pred_diff < 0.005) 
    
    input_vnr_arith_closeness, input_vnr_geo_closeness = test_utils.get_closeness_metric(ref_input_vnr_pred, dut_input_vnr_pred)    
    output_vnr_arith_closeness, output_vnr_geo_closeness = test_utils.get_closeness_metric(ref_output_vnr_pred, dut_output_vnr_pred)    
    print(f"input_vnr_arith_closeness {input_vnr_arith_closeness}, input_vnr_geo_closeness {input_vnr_geo_closeness}")  
    print(f"output_vnr_arith_closeness {output_vnr_arith_closeness}, output_vnr_geo_closeness {output_vnr_geo_closeness}")  
    assert(input_vnr_arith_closeness > 0.90)
    assert(input_vnr_geo_closeness > 0.90)
    assert(output_vnr_arith_closeness > 0.90)
    assert(output_vnr_geo_closeness > 0.90)
    
    # Plot
    fig,ax = plt.subplots(2)
    fig.set_size_inches(20, 10)
    ax[0].set_title('input_vnr_pred')
    ax[0].plot(ref_input_vnr_pred, label="ref")
    ax[0].plot(dut_input_vnr_pred, label="dut")
    ax[0].legend(loc="upper right")

    ax[1].set_title('output_vnr_pred')
    ax[1].plot(ref_output_vnr_pred, label="ref")
    ax[1].plot(dut_output_vnr_pred, label="dut")
    ax[1].legend(loc="upper right")
    fig_instance = plt.gcf()
    if show_plot:
        plt.show()
    fig.savefig(f"plot_{target}_test_calc_vnr_pred.png")

        

if __name__ == "__main__":
    test_calc_vnr_pred("xcore", test_utils.get_model(), show_plot=True)
