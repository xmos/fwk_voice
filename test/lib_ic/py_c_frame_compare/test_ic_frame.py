# Copyright 2022 XMOS LIMITED.
# This Software is subject to the terms of the XMOS Public Licence: Version 1.

import numpy as np
import scipy.io.wavfile
import audio_wav_utils as awu
import pytest
import sys
from pathlib import Path

from build import ic_test_py
from ic_test_py import ffi
import ic_test_py.lib as ic_test_lib

sys.path.append(str(Path(__file__).parents[2] / "shared" / "python")) # For py_vs_c_utils
import py_vs_c_utils as pvc 

from py_voice.modules import ic
from py_voice.config import config

ap_config_file = Path(__file__).parents[2] / "shared" / "config" / "ic_conf_no_adapt_control.json"
ap_conf = config.get_config_dict(ap_config_file)

proc_frame_length = ap_conf["general"]["proc_frame_length"]
frame_advance = ap_conf["general"]["frame_advance"]

input_file = "../../../examples/bare-metal/ic/input.wav"
output_file = "output.wav"

@pytest.fixture(params=[34])
def pre_test_stuff(request):
    return request.param


class ic_comparison:
    def __init__(self):

        self.ic = ic.ic(ap_conf)

        ic_test_lib.test_init()

    def process_frame(self, frame):
        #we need to delay the y for python as not done in model
        #first copy the input data for C ver before we modfiy it
        frame_int = pvc.float_to_int32(frame)
        mt_data = {}
        output_py, out_metadata = self.ic.process_frame(frame, mt_data)
 
        #Grab a pointer to the data storage of the numpy arrays
        y_data = ffi.cast("int32_t *", ffi.from_buffer(frame_int[0].data))
        x_data = ffi.cast("int32_t *", ffi.from_buffer(frame_int[1].data))
        output_c = np.zeros((240), dtype=np.int32)
        output_c_ptr = ffi.cast("int32_t *", ffi.from_buffer(output_c.data))

        ic_test_lib.test_filter(y_data, x_data, output_c_ptr)

        vnr = [0,0]
        ic_test_lib.test_adapt(vnr)

        #state = ic_test_lib.test_get_state()
        return output_py, pvc.int32_to_float(output_c)

def test_frame_compare():
    icc = ic_comparison()

    input_rate, input_wav_file = scipy.io.wavfile.read(input_file, 'r')
    input_wav_data, input_channel_count, file_length = awu.parse_audio(input_wav_file)

    output_wav_data = np.zeros((2, file_length))

    for frame_start in range(0, file_length-proc_frame_length*2, frame_advance):
        input_frame = awu.get_frame(input_wav_data, frame_start, frame_advance)[0:2,:]

        output_py, output_c = icc.process_frame(input_frame)

        output_wav_data[0, frame_start: frame_start + frame_advance] = output_py
        output_wav_data[1, frame_start: frame_start + frame_advance] = output_c

    #Write a copy of the output for post analysis if needed
    scipy.io.wavfile.write(output_file, input_rate, pvc.float_to_int32(output_wav_data.T))

    arith_closeness, geo_closeness, c_delay, peak2ave = pvc.pcm_closeness_metric(output_file)
    assert arith_closeness > 0.98
    assert geo_closeness > 0.99
    assert c_delay == 0
