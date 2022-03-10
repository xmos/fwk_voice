# Copyright 2019-2021 XMOS LIMITED.
# This Software is subject to the terms of the XMOS Public Licence: Version 1.
import numpy as np
import pytest

from usb_model import model_usb

HIGH = 48014
NOM = 48000
LOW = 47986

NUM_USB_CHANS = 4
NUM_SOFS = 1000 * 10 # 10 Seconds


@pytest.fixture(params=[int(i) for i in range(47995, 48006)])
def usb_model_output(request):
    host_ticks = request.param
    consecutive_nudges, fifo_trace = model_usb(NUM_SOFS,
                                               fifo_target_level=0,
                                               host_ticks=host_ticks,
                                               num_usb_chans=NUM_USB_CHANS,
                                               device_ticks_low=LOW,
                                               device_ticks_nom=NOM,
                                               device_ticks_high=HIGH)
    return (host_ticks, consecutive_nudges, fifo_trace)


def test_consecutive_nudges(usb_model_output):
    host_ticks, consecutive_nudges, fifo_trace = usb_model_output

    if host_ticks > NOM:
        frac = (host_ticks - NOM) / (HIGH - NOM)
    elif host_ticks < NOM:
        frac = (NOM - host_ticks) / (NOM - LOW)
    else:
        frac = 0

    for n in range(2, NUM_SOFS):
        if (n-1)/n > frac:
            max_consecutive_nudges = n + 1
            break

    assert np.max(consecutive_nudges) <= max_consecutive_nudges


def test_fifo_lower_limit(usb_model_output):
    host_ticks, consecutive_nudges, fifo_trace = usb_model_output

    assert np.all(fifo_trace <= NUM_USB_CHANS)


def test_fifo_upper_limit(usb_model_output):
    host_ticks, consecutive_nudges, fifo_trace = usb_model_output

    assert np.all(fifo_trace >= -NUM_USB_CHANS)


