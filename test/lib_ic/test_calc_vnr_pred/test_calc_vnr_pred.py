import numpy as np
import py_vs_c_utils as pvc
from run_dut import run_dut
from py_voice.modules import ic
from py_voice.config import config
from pathlib import Path

xe = Path(__file__).parents[3]/ "build" / "test" / "lib_ic" / "test_calc_vnr_pred" / "bin" / "fwk_voice_test_calc_vnr_pred.xe"
ap_config_file = Path(__file__).parents[2] / "shared" / "config" / "ic_conf_no_adapt_control.json"
ap_conf = config.get_config_dict(ap_config_file)

def test_calc_vnr_pred(target):
    np.random.seed(12345)
    ap_conf["ic"]["adaption_config"] = "ADAPTION_AUTO"
    ifc = ic.ic(ap_conf)

    # No. of int32 values sent to dut as input per frame
    input_words_per_frame = (1+(ifc.f_bin_count*2))*2 # Y_data exponent followed by ifc.f_bin_count complex values, followed by Error exponent followed by ifc.f_bin_count complex values
    output_words_per_frame = 2 # DUT outputs 1 float_s32_t value -> input_vnr_pred
    input_data = np.array([input_words_per_frame, output_words_per_frame], dtype=np.int32)
    
    ref_input_vnr_pred = np.empty(0, dtype=np.float64)
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
        data = pvc.int32_to_double(data, exp)
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
        data = pvc.int32_to_double(data, exp)
        Error_ap = data.astype(np.float64).view(np.complex128)
        Error_ap = Error_ap.reshape((1, len(Error_ap))) # Reference Error values

        # Call Reference calc_vnr_pred()
        _, _ = ifc.calc_vnr_pred(Error_ap)
        ref_input_vnr_pred = np.append(ref_input_vnr_pred, ifc.input_vnr_pred[0])
    
    # Run DUT
    op, _ = run_dut(input_data, xe, target)

    dut_input_vnr_pred = pvc.float_s32_arr_to_double(op)
    
    # Compare dut-ref
    np.testing.assert_allclose(dut_input_vnr_pred, ref_input_vnr_pred, rtol=0, atol=0.005)
    print(f"input_vnr_pred diff = {np.max(np.abs(ref_input_vnr_pred - dut_input_vnr_pred))}")
    
    input_vnr_arith_closeness, input_vnr_geo_closeness = pvc.get_closeness_metric(ref_input_vnr_pred, dut_input_vnr_pred)    
    print(f"input_vnr_arith_closeness {input_vnr_arith_closeness}, input_vnr_geo_closeness {input_vnr_geo_closeness}")  
    assert(input_vnr_arith_closeness > 0.90)
    assert(input_vnr_geo_closeness > 0.90)

if __name__ == "__main__":
    test_calc_vnr_pred("native")
