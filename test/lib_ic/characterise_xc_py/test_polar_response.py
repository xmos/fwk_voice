# Copyright 2018-2021 XMOS LIMITED.
# This Software is subject to the terms of the XMOS Public Licence: Version 1.

from builtins import zip
import pytest
from get_polar_response import get_polar_response

ANGLE_ROI = 360
ANGLE_STEP_SIZE = 20
RT60 = 0.3
NOISE_BAND = 8000
NOISE_LEVEL = -20
IC_DELAY = 80


# @pytest.mark.parametrize
def test_polar_reponse():
    angles, results_py = get_polar_response("pytest_audio",
                                         ANGLE_ROI,
                                         ANGLE_STEP_SIZE,
                                         NOISE_BAND,
                                         NOISE_LEVEL,
                                         RT60,
                                         IC_DELAY)
    for angle, attenuation in zip(angles, results_py):
        assert attenuation > 3


def test_compare_polar_reponse():
    angles, results = get_polar_response("pytest_audio",
                                         ANGLE_ROI,
                                         120,
                                         NOISE_BAND,
                                         NOISE_LEVEL,
                                         RT60,
                                         IC_DELAY,
                                         run_xc=True)
    for (i, atten_py, atten_xc) in zip(angles, results[0], results[1]):
        assert abs(atten_py - atten_xc) < 1, "Angle: {}, PY {}, XC {}".format(i, atten_py, atten_xc)
