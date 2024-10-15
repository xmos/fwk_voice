# Copyright 2022 XMOS LIMITED.
# This Software is subject to the terms of the XMOS Public Licence: Version 1.

from __future__ import print_function
from builtins import zip
import pytest
from compare_c_py import get_attenuation_c_py
import numpy as np

@pytest.mark.parametrize("noise_band", [4000, 8000])
@pytest.mark.parametrize("noise_db", [-60, -20, -6])
def test_ns_attenuation(noise_band, noise_db):
    test_name = f"ns_attn_test_{noise_band}_{noise_db}"
    print(f"--- {test_name} ---")

    attenuation_c, attenuation_py = get_attenuation_c_py(test_name, int(noise_band), int(noise_db))

    c_res = np.array(attenuation_c)
    py_res = np.array(attenuation_py)
    for (atten_bin_py, atten_bin_c) in zip(py_res, c_res):
        assert atten_bin_py - 1 < atten_bin_c
        assert atten_bin_c > 0
