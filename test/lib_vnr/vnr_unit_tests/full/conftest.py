import pytest
import os

@pytest.fixture 
def tflite_model():
    return os.path.abspath("../../test_wav_vnr/model/model_output_0_0_2/model_qaware.tflite")
