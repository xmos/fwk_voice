# Copyright 2023 XMOS LIMITED.
# This Software is subject to the terms of the XMOS Public Licence: Version 1.
import pytest
from compare_mel_wav import compare_mel_spec


@pytest.mark.parametrize("a_param", (1,2,3))
def test_simple(a_param):
    
    input_file = "../../lib_vnr/test_wav_vnr/data_16k/8842-302203-0010001.wav"
    number_of_blocks = 4
    size = "small"
    output_file_python = "mel_py.txt"
    output_file_c = "mel_c.txt"
    quantise = True
    to_decibel = True
    subtract_mean = True

    equal = compare_mel_spec(
        input_file,
        number_of_blocks,
        size,
        output_file_python,
        output_file_c,
        quantise,
        to_decibel,
        subtract_mean
        )

    assert equal
