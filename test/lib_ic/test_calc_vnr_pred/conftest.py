import pytest
import os
this_file_dir = os.path.dirname(os.path.realpath(__file__))
import sys
sys.path.append(os.path.join(this_file_dir, "../../lib_vnr/vnr_unit_tests/feature_extraction"))
import test_utils

@pytest.fixture 
def tflite_model():
    return test_utils.get_model()

def pytest_generate_tests(metafunc):
    if "target" in metafunc.fixturenames:
        metafunc.parametrize("target", ['x86', 'xcore'])
