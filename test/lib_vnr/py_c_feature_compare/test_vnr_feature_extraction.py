# Copyright 2022 XMOS LIMITED.
# This Software is subject to the terms of the XMOS Public Licence: Version 1.

import numpy as np
import scipy.io.wavfile
import audio_wav_utils as awu
import ctypes
import pytest
import sys, os
import tempfile
import data_processing.frame_preprocessor as fp
import py_vnr.vnr as vnr
import py_vnr.run_wav_vnr as rwv

from build import vnr_test_py
from vnr_test_py import ffi
import vnr_test_py.lib as vnr_test_lib

package_dir = os.path.dirname(os.path.abspath(__file__))
pvc_path = os.path.join(package_dir, '../../shared/python')
sys.path.append(pvc_path)
import py_vs_c_utils as pvc 

input_file = os.path.join(package_dir, "../../../examples/bare-metal/vnr/test_stream_1.wav")
tflite_model = os.path.join(package_dir, "../../../modules/lib_vnr/python/model/model_output/trained_model.tflite")

def bfp_s32_to_float(bfp_struct, data):
    
    # bfp_s32_t in  ffi is stored as x[0], x[1] address, x[2] exp, x[3] hr, x[4] len. where x is an int32 array
    exp = bfp_struct[2]
    len = bfp_struct[4]
    data_float = data[:len].astype(np.float64) * (2.0 ** exp)
    return data_float

def get_closeness_metric(ref, dut):
    tmp_folder = tempfile.mkdtemp(dir=".")
    prev_path = os.getcwd()
    os.chdir(tmp_folder)
    output_file = "temp.wav"
    output_wav_data = np.zeros((2, len(ref)))
    output_wav_data[0,:] = ref
    output_wav_data[1,:] = dut
    scipy.io.wavfile.write(output_file, 16000, output_wav_data.T)
    arith_closeness, geo_closeness, c_delay, peak2ave = pvc.pcm_closeness_metric(output_file, verbose=False)
    os.chdir(prev_path)
    os.system("rm -r {}".format(tmp_folder))
    return arith_closeness, geo_closeness

class vnr_feature_comparison:
    def __init__(self):
        self.vnr_obj = vnr.Vnr(model_file=tflite_model) 
        self.x_data = np.zeros(fp.FRAME_LEN, dtype=np.float64)
        err = vnr_test_lib.test_init()

    def process_frame(self, new_x_frame):
        frame_int = pvc.float_to_int32(new_x_frame)

        # Ref
        self.x_data = np.roll(self.x_data, -fp.FRAME_ADVANCE, axis = 0)
        self.x_data[fp.FRAME_LEN - fp.FRAME_ADVANCE:] = new_x_frame
        # Features
        ref_features = rwv.extract_features(self.x_data, self.vnr_obj)
        # Inference
        ref_ie_output = self.vnr_obj.run(ref_features)
        ref_features = ref_features.flatten()
        
        # DUT
        dut_x_data = ffi.cast("int32_t *", ffi.from_buffer(frame_int[0].data))
        dut_features_bfp = np.zeros((20), dtype=np.int32)
        dut_features_bfp_ptr = ffi.cast("bfp_s32_t *", ffi.from_buffer(dut_features_bfp.data))
        dut_features_data = np.zeros((fp.PATCH_WIDTH * fp.MEL_FILTERS), dtype=np.int32)
        dut_features_data_ptr = ffi.cast("int32_t *", ffi.from_buffer(dut_features_data.data))        
        # Features       
        vnr_test_lib.test_vnr_features(dut_features_bfp_ptr, dut_features_data_ptr, dut_x_data)
        dut_features = bfp_s32_to_float(dut_features_bfp, dut_features_data)
        # Inference
        dut_ie_output = vnr_test_lib.test_vnr_inference(dut_features_bfp_ptr)
        
        return ref_features, dut_features, ref_ie_output[0], dut_ie_output
        

def test_frame_features():
    vnrc = vnr_feature_comparison()
    input_rate, input_wav_file = scipy.io.wavfile.read(input_file, 'r')
    input_wav_data, input_channel_count, file_length = awu.parse_audio(input_wav_file)
    
    ref_features_output = np.empty(0, dtype=np.float64)
    dut_features_output = np.empty(0, dtype=np.float64)
    ref_ie_output = np.empty(0, dtype=np.float64)
    dut_ie_output = np.empty(0, dtype=np.float64)
    for frame_start in range(0, file_length - fp.FRAME_ADVANCE, fp.FRAME_ADVANCE):
        # buffer the input data into STFT slices
        new_x_frame = awu.get_frame(input_wav_data, frame_start, fp.FRAME_ADVANCE)
        ref_features, dut_features, ref_ie, dut_ie = vnrc.process_frame(new_x_frame)
        ref_features_output = np.append(ref_features_output, ref_features)
        dut_features_output = np.append(dut_features_output, dut_features)
        ref_ie_output = np.append(ref_ie_output, ref_ie)
        dut_ie_output = np.append(dut_ie_output, dut_ie)
    
    # Compare features
    arith_closeness_features, geo_closeness_features = get_closeness_metric(ref_features_output, dut_features_output)
    print(f"Features: arith_closeness {arith_closeness_features}, geo_closeness {geo_closeness_features}")
    max_error_features = np.max(np.abs(ref_features_output - dut_features_output))
    print(f"Features: max_error = {max_error_features}")

    # Compare infrence output
    arith_closeness_ie, geo_closeness_ie = get_closeness_metric(ref_ie_output, dut_ie_output)
    print(f"Inference: arith_closeness {arith_closeness_ie}, geo_closeness {geo_closeness_ie}")
    max_error_ie = np.max(np.abs(ref_ie_output - dut_ie_output))
    print(f"Inference: max_error = {max_error_ie}")

    assert(max_error_features < 0.006), f"features, max ref-dut error {max_error} exceeds threshold"
    assert(arith_closeness_features > 0.999), f"features, arith_closeness {arith_closeness} less than pass threshold"
    assert(geo_closeness_features > 0.999), f"features, arith_closeness {arith_closeness} less than pass threshold"

    assert(max_error_ie < 0.05), f"Inference, max ref-dut error {max_error} exceeds threshold"
    assert(arith_closeness_ie > 0.99), f"Inference, arith_closeness {arith_closeness} less than pass threshold"
    assert(geo_closeness_ie > 0.99), f"Inference, arith_closeness {arith_closeness} less than pass threshold"
        


if __name__ == "__main__":
    test_frame_features()
