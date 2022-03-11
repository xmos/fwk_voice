# Copyright 2022 XMOS LIMITED.
# This Software is subject to the terms of the XMOS Public Licence: Version 1.

import numpy as np
import pytest
import sys


import build
print("Poo", dir(build), file=sys.stderr)

from build import ic_test_py
from ic_test_py import ffi
import ic_test_py.lib as ic_test_lib



@pytest.fixture(params=[int(i) for i in range(47995, 48006)])
def usb_model_output(request):
    host_ticks = request.param
   
    return (1, 2, 3)


def a_fumc():
    # self.pid_state = ffi.new("struct pid_state_t *")
    val = ic_test_lib.test_frame(33)
    print(33, val)

def test_frame(usb_model_output):
    host_ticks, consecutive_nudges, fifo_trace = usb_model_output

    
    a_fumc()


