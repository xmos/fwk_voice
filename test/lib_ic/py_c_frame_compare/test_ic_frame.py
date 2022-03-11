# Copyright 2022 XMOS LIMITED.
# This Software is subject to the terms of the XMOS Public Licence: Version 1.

import numpy as np
import pytest
import sys

from build import ic_test_py
from ic_test_py import ffi
import ic_test_py.lib as ic_test_lib



@pytest.fixture(params=[33,34])
def usb_model_output(request):
    host_ticks = request.param
   
    return (host_ticks, 2, 3)


def a_fumc(inval):
    val = ic_test_lib.test_frame(inval)
    print(inval, val)
    # state = ffi.new("grr_t *")

def test_frame(usb_model_output):
    host_ticks, consecutive_nudges, fifo_trace = usb_model_output

    
    a_fumc(host_ticks)


