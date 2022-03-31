# Copyright 2022 XMOS LIMITED.
# This Software is subject to the terms of the XMOS Public Licence: Version 1.
import pytest

from run_test import run_test

test_name = 'test_wav_delay_estimator_controller'

#See conftest.py for the list of tests that is generated into test_config

def test_wav_delay_estimator_controller(test_config):
    #Run test runs a single instance
    report = run_test(test_config['pipeline_config'], test_config['info'], test_config['path_to_regression_files'], test_config['input_audio_files'], test_config['far_end_delay_changes'], test_length_s=test_config['test_length_s'], run_target=test_config['run_target'], volume_changes=test_config['volume_changes'])

    assert_message = "{} | {} | far_end_changes: {} | len:{} | run_target:{} **".format(test_config['info'], test_config['input_audio_files'], test_config['far_end_delay_changes'], test_config['test_length_s'], test_config['run_target'])

    assert (report["false_negatives"] <= test_config['allowable_false_negatives']), assert_message
    assert (report["false_positives"] <= test_config['allowable_false_positives']), assert_message
    assert (report["inaccurate_estimates"] <= report["false_positives"] + report["false_negatives"]) , assert_message
    assert (report["last_value_inrange"] == True), assert_message
