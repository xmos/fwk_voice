# Copyright 2022 XMOS LIMITED.
# This Software is subject to the terms of the XMOS Public Licence: Version 1.
import configparser
import os.path
import pytest

parser = configparser.ConfigParser()
parser.read("parameters.cfg")
results_dir = parser.get("Folders", "results_dir")
plot_dir = os.path.join(results_dir, "plots")
log_dir = os.path.join(results_dir, "logs")

with open(os.path.join(results_dir, "all_tests.txt"), 'r') as f:
    all_tests = f.readlines()
with open(os.path.join(results_dir, "failed_tests.txt"), 'r') as f:
    failed_tests = f.readlines()

# Parametrize with test files
@pytest.mark.parametrize("test", all_tests)
def test_evaluate_results(test):
    assert not test in failed_tests
