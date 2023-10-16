# Copyright 2023 XMOS LIMITED.
# This Software is subject to the terms of the XMOS Public Licence: Version 1.
import pytest
from build_mel import clean_ffi
from compare_mel_wav import compare_mel_spec
from pathlib import Path


@pytest.fixture(scope="session")
def clean_build():
    pass
    # clean_ffi()


@pytest.mark.parametrize("size", ("small", "large"))
@pytest.mark.parametrize("quantise", (True, False))
@pytest.mark.parametrize("to_decibel", (True, False))
@pytest.mark.parametrize("subtract_mean", (True, False))
# @pytest.mark.parametrize("size", ("small",))
# @pytest.mark.parametrize("quantise", (False,))
# @pytest.mark.parametrize("to_decibel", (True,))
# @pytest.mark.parametrize("subtract_mean", (True,))
def test_simple(clean_build, size, quantise, to_decibel, subtract_mean):
    
    input_file = Path(__file__).resolve().parent / "../../lib_vnr/test_wav_vnr/data_16k/8842-302203-0010001.wav"
    number_of_blocks = 2
    output_file_python = "mel_py.txt"
    output_file_c = "mel_c.txt"

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
