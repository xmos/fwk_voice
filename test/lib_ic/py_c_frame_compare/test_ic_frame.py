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
py_ic_path = os.path.join(package_dir,'../../../../py_ic/py_ic')
pvc_path = os.path.join(package_dir, '../../shared/python')

sys.path.append(att_path)
sys.path.append(py_ic_path)
sys.path.append(pvc_path)
import IC
import py_vs_c_utils as pvc 


proc_frame_length = 512
frame_advance = 240
num_phases = 10
x_channel_delay = 180 #For python only, this is already compiled into the C lib
input_file = "../../../examples/bare-metal/ic/input.wav"
output_file = "output.wav"

@pytest.fixture(params=[34])
def pre_test_stuff(request):
    return request.param


class ic_comparison:
    def __init__(self):
        self.ic = IC.adaptive_interference_canceller(frame_advance, proc_frame_length, num_phases, 
        mu = 0.36956599983386695,
        delta = 7.450580593454381e-09, #two_mic_stereo.json
        K = 1,
        lamda = 0.9995117188,
        gamma = 2.0,
        leakage = 0.995,
        y_channel_delay = x_channel_delay,
        remove_NQ = True,
        adaptation_config = 'IC_ADAPTION_FORCE_ON'
        )

        ic_test_lib.test_init()


    def process_frame(self, frame):
        #we need to delay the y for python as not done in model
        #first copy the input data for C ver before we modfiy it
        frame_int = pvc.float_to_int32(frame)

        output_py, Error_ap = self.ic.process_frame(frame)
        self.ic.adapt(Error_ap)

        #Grab a pointer to the data storage of the numpy arrays
        y_data = ffi.cast("int32_t *", ffi.from_buffer(frame_int[0].data))
        x_data = ffi.cast("int32_t *", ffi.from_buffer(frame_int[1].data))
        output_c = np.zeros((240), dtype=np.int32)
        output_c_ptr = ffi.cast("int32_t *", ffi.from_buffer(output_c.data))

        ic_test_lib.test_filter(y_data, x_data, output_c_ptr)

        vad = int(0)
        ic_test_lib.test_adapt(vad, output_c_ptr)

        state = ic_test_lib.test_get_state()
        #print('mu_c = ', pvc.float_s32_to_float(state.mu[0][0]), ', nu_py = ', self.ic.mu)
        #print('leakage_c = ', pvc.float_s32_to_float(state.ic_adaption_controller_state.adaption_controller_config.leakage_alpha), ', leakage_py = ', self.ic.leakage)
        # print(pvc.float_s32_to_float(state.config_params.delta))
        return output_py, pvc.int32_to_float(output_c)


def test_frame_compare(pre_test_stuff):
    icc = ic_comparison()

    input_rate, input_wav_file = scipy.io.wavfile.read(input_file, 'r')
    input_wav_data, input_channel_count, file_length = awu.parse_audio(input_wav_file)
    delays = np.zeros(input_channel_count) #we do delay of y channel in process_frame above and in C rather than awu.get_frame

    output_wav_data = np.zeros((2, file_length))

    for frame_start in range(0, file_length-proc_frame_length*2, frame_advance):
        input_frame = awu.get_frame(input_wav_data, frame_start, frame_advance, delays)[0:2,:]

        if False:
            print ('# ' + str(frame_start // frame_advance))

        output_py, output_c = icc.process_frame(input_frame)

        output_wav_data[0, frame_start: frame_start + frame_advance] = output_py
        output_wav_data[1, frame_start: frame_start + frame_advance] = output_c
        
    #Write a copy of the output for post analysis if needed
    scipy.io.wavfile.write(output_file, input_rate, pvc.float_to_int32(output_wav_data.T))

    arith_closeness, geo_closeness, c_delay, peak2ave = pvc.pcm_closeness_metric(output_file)
    assert arith_closeness > 0.98
    assert geo_closeness > 0.99
    assert c_delay == 0
