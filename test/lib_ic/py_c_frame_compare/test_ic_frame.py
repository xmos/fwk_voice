# Copyright 2022 XMOS LIMITED.
# This Software is subject to the terms of the XMOS Public Licence: Version 1.

import numpy as np
import scipy.io.wavfile
import audio_wav_utils as awu
import ctypes
import pytest
import sys, os

from build import ic_test_py
from ic_test_py import ffi
import ic_test_py.lib as ic_test_lib

package_dir = os.path.dirname(os.path.abspath(__file__))
att_path = os.path.join(package_dir,'../../../audio_test_tools/python/')
py_ic_path = os.path.join(package_dir,'../../../../lib_interference_canceller/python')

sys.path.append(att_path)
sys.path.append(py_ic_path)
import IC


proc_frame_length = 512
frame_advance = 240
num_phases = 10
input_file = "../../../examples/bare-metal/ic/input.wav"
output_file = "output.wav"

@pytest.fixture(params=[34])
def pre_test_stuff(request):
    return request.param


def float_s32_to_float(float_s32):
    return np.ldexp(float_s32.mant, float_s32.exp)

def int32_to_float(array_int32):
    array_float = array_int32.astype(np.float64) / (-np.iinfo(np.int32).min)
    return array_float

def float_to_int32(array_float):
    array_int32 = (array_float * np.iinfo(np.int32).max).astype(np.int32)
    return array_int32

class ic_comparison:
    def __init__(self):
        self.ic = IC.adaptive_interference_canceller(frame_advance, proc_frame_length, num_phases, 0, 
        mu = 0.36956599983386695,
        delta = 6.999999999379725e-05,
        delay = 0,
        K = 1,
        lamda = 0.9995117188,
        gamma = 2.0,
        leakage = 1.0,
        channel_delay = 180,
        remove_NQ = True,
        use_noise_minimisation = False,
        output_beamform_on_ch1 = True)

        ic_test_lib.test_init()

    def process_frame(self, frame):
        all_channels_output, Error_ap = self.ic.process_frame(frame)
        self.ic.adapt(Error_ap)
        output_py = all_channels_output[0,:]

        frame_int = float_to_int32(frame)
        y_data = ffi.from_buffer(frame_int[0].data)
        x_data = ffi.from_buffer(frame_int[1].data)
        output_c = np.zeros((240), dtype=np.int32)
        output_c_ptr = ffi.cast("int32_t *", ffi.from_buffer(output_c.data))
        ic_test_lib.test_filter(y_data, x_data, output_c_ptr)
        vad = int(0)
        ic_test_lib.test_adapt(0, output_c_ptr)

        return output_py, int32_to_float(output_c)

    # state = ic_test_lib.test_get_state()
    # print(float_s32_to_float(state.mu[0][0]))
    # print(float_s32_to_float(state.config_params.delta))


def test_frame_compare(pre_test_stuff):
    icc = ic_comparison()


    input_rate, input_wav_file = scipy.io.wavfile.read(input_file, 'r')
    input_wav_data, input_channel_count, file_length = awu.parse_audio(input_wav_file)

    output_wav_data = np.zeros((2, file_length))

    for frame_start in range(0, file_length-proc_frame_length*2, frame_advance):
        input_frame = awu.get_frame(input_wav_data, frame_start, frame_advance, np.zeros(2))

        if False:
            print ('# ' + str(frame_start // frame_advance))

        output_py, output_c = icc.process_frame(input_frame)

        output_wav_data[0, frame_start: frame_start + frame_advance] = output_py
        output_wav_data[1, frame_start: frame_start + frame_advance] = output_c
        
    scipy.io.wavfile.write(output_file, input_rate, output_wav_data.T)




