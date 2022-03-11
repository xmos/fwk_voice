# Copyright 2022 XMOS LIMITED.
# This Software is subject to the terms of the XMOS Public Licence: Version 1.

import numpy as np
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

@pytest.fixture(params=[33,34])
def pre_test_stuff(request):
   
    return request.param


def float_s32_to_float(float_s32):
    return np.ldexp(float_s32.mant, float_s32.exp)

def a_func(inval):
    ic = IC.adaptive_interference_canceller(240, 512, 10, 0, 
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

    frame = np.ones((2, 240), dtype=np.int32) * 10000000
    frame_fp = frame.astype(np.float64) / (-np.iinfo(np.int32).min)
    all_channels_output, Error_ap = ic.process_frame(frame_fp)
    ic.adapt(Error_ap)
    output_py = all_channels_output[0]

    ic_test_lib.test_init()

    state = ic_test_lib.test_get_state()
    print(float_s32_to_float(state.mu[0][0]))
    print(float_s32_to_float(state.config_params.delta))

    c_output = np.zeros((240))
    print("&***",frame[0].ctypes.data_as(ctypes.POINTER(ctypes.c_int32)))

    y_data = ffi.from_buffer(frame[0].data)
    x_data = ffi.from_buffer(frame[1].data)
    output = np.zeros(240, dtype=np.int32);
    ic_test_lib.test_filter(y_data, x_data, ffi.from_buffer(output.data))
    
    vad = int(0)
    ic_test_lib.test_adapt(0, ffi.from_buffer(output.data))
    output_c = output.astype(np.float64) / (-np.iinfo(np.int32).min)

    print(output_py) 
    print(output_c) 


def test_frame(pre_test_stuff):

    a_func(pre_test_stuff)


