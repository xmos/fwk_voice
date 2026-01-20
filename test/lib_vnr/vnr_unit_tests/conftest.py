import pytest
from pathlib import Path
import numpy as np
import py_voice.modules.vnr as vnr
import test_utils
from run_dut import run_dut

tflite_model = Path(__file__).parents[3] / "lib_voice" / "src" / "vnr" / "model" / "trained_model.tflite"
vnr_conf = Path(__file__).parents[4] / "py_voice" / "py_voice" / "config" / "components" / "vnr_only.json"
bin_dir_path = Path(__file__).parents[3] / "build" / "test" / "lib_vnr" / "vnr_unit_tests" / "bin"

@pytest.fixture(scope="session")
def model_details():
    return test_utils.get_model_details(tflite_model)

@pytest.fixture(scope="session")
def quantise(model_details):
    def _quantise(this_patch):
        return test_utils.quantise_patch(this_patch, model_details[0])

    return _quantise

@pytest.fixture(scope="session")
def dequantise(model_details):
    def _dequantise(output_data):
        return test_utils.dequantise_output(output_data, model_details[1])

    return _dequantise

@pytest.fixture
def vnr_obj():
    return vnr.vnr(vnr_conf, model_file=str(tflite_model))

@pytest.fixture
def dut_runner(request, target):
    exe_path = bin_dir_path / request.node.originalname

    def _run_dut(input_bin):
        op, _ = run_dut(input_bin, exe_path, target)
        return op

    return _run_dut

@pytest.fixture
def rng():
    return np.random.default_rng(1243)

def pytest_generate_tests(metafunc):
    if "target" in metafunc.fixturenames:
        metafunc.parametrize("target", ['native', 'xs3a'])
