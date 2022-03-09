# Copyright 2022 XMOS LIMITED.
# This Software is subject to the terms of the XMOS Public Licence: Version 1.

import os, sys, shutil
import pytest
from pipeline_test_utils import process_file, get_wav_info, convert_input_wav, convert_keyword_wav
from conftest import pipeline_input_dir, results_log_file, quick_test_setting, quick_test_pass_threshold
from run_sensory import run_sensory
import time, fcntl


def test_pipelines(test, record_property):
    wav_file = test[0] 
    wav_name = os.path.basename(wav_file)
    target = test[1]

    input_file = os.path.join(pipeline_input_dir, wav_name)
    convert_input_wav(wav_file, input_file)

    chans, rate, samps, bits = get_wav_info(input_file)
    print(f"Processing a {samps//rate}s track")
    t0 = time.time()
    output_file = process_file(input_file, target)
    tot = time.time() - t0
    print(f"Processing took {tot:.2f}s")

    keyword_file = convert_keyword_wav(output_file, target)
    detections =run_sensory(keyword_file)
    # print(f"{wav_name} : {detections}", file=sys.stderr)

    with open(results_log_file, "a") as log:
        fcntl.flock(log, fcntl.LOCK_EX)
        log.write(f"{wav_name},{target},{detections},\n") 
        fcntl.flock(log, fcntl.LOCK_UN)


    record_property("Target", target)
    record_property("Wakewords", detections)

    if quick_test_setting != 0 and detections < quick_test_pass_threshold:
        assert 0, f"Quick test failed for file {wav_name}, target {target}. Expected {quick_test_pass_threshold} keywords, got {detections}"
            
