# Copyright 2022 XMOS LIMITED.
# This Software is subject to the terms of the XMOS Public Licence: Version 1.
from builtins import range
from builtins import object
import numpy as np
import scipy.signal
import audio_generation

sample_rate = 16000

frame_advance = 240


def convolve(input_x, input_y, filter_x, filter_y):
    """ Convolves each input with their respective filter 
    
    Takes into account changes in the filter per frame.
    """

    input_len = min(len(input_x), len(input_y))
    audio_x = convolve_1ch(input_x, filter_x, input_len)
    audio_y = convolve_1ch(input_y, filter_y, input_len)
    return audio_x, audio_y


def convolve_1ch(input_audio, filt, input_len):
    num_frames = int(input_len // 240)
    output_audio = np.array([])
    cur_filter = filt.get_filter(0)
    for i in range(1, num_frames+1):
        next_filter = filt.get_filter(i)
        if not np.array_equal(cur_filter, next_filter) or i == num_frames:
            convolution = scipy.signal.convolve(input_audio, cur_filter)
            convolution_slice = convolution[len(output_audio):i*frame_advance]
            #print("Time: {}".format(i*frame_advance / sample_rate))
            #print("Output shape: {}, Input shape: {}".format(output_audio.shape, convolution_slice.shape))
            output_audio = np.concatenate((output_audio, convolution_slice))
            cur_filter = next_filter

    #print("Output shape: {}".format(output_audio.shape))
    return output_audio


class Filter(object):
    def __init__(self):
        raise NotImplementedError

    def get_filter(self, frame_num):
        return self._filter


class Identity(Filter):
    def __init__(self):
        self._filter = np.ones(1)


class OneImpulse(Filter):
    def __init__(self, index):
        self._filter = np.zeros((max(50, index+1),))
        self._filter[index] = 1


class Diffuse(Filter):
    def __init__(self, seed=0, rt60=0.3):
        a = 3.0 * np.log(10.0) / rt60
        t = np.arange(2.0 * rt60 * sample_rate) / sample_rate
        np.random.seed(seed)
        self._filter = 0.01 * np.random.randn(t.shape[0]) * np.exp(-a*t)


class ShortEcho(Filter):
    def __init__(self):
        self._filter = audio_generation.get_h('short')


class ZeroAt(Filter):
    def __init__(self, zero_time=5):
        self._zero_time = zero_time

    def get_filter(self, frame_num):
        if frame_num * frame_advance > self._zero_time * sample_rate:
            return np.zeros(1)
        else:
            return np.ones(1)


class MovingSource(Filter):
    def __init__(self, move_frequency=1, max_samples_moved=10):
        self._filter = np.zeros(max_samples_moved)
        self._filter[0] = 1
        self._max_samples_moved = max_samples_moved
        self._move_frequency = move_frequency

    def get_filter(self, frame_num):
        i = int(frame_num * frame_advance / (sample_rate * self._move_frequency))
        if i % 2 == 0:
            move = i % self._max_samples_moved
        else:
            move = self._max_samples_moved - (i % self._max_samples_moved)
        self._filter = np.zeros(self._max_samples_moved)
        self._filter[move] = 1
        return self._filter
