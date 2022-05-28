import pytest
import os
this_file_path = os.path.dirname(os.path.realpath(__file__))
@pytest.fixture 
def tflite_model():
    return os.path.join(this_file_path, "../test_wav_vnr/model/model_output_0_0_2/model_qaware.tflite")

def pytest_generate_tests(metafunc):
    if "target" in metafunc.fixturenames:
        metafunc.parametrize("target", ['x86', 'xcore'])
