# Copyright 2022 XMOS LIMITED.
# This Software is subject to the terms of the XMOS Public Licence: Version 1.

import os, sys, shutil
import pytest
from pipeline_test_utils import process_file, get_wav_info, convert_input_wav, convert_keyword_wav
from conftest import pipeline_input_dir, results_log_file, full_pipeline_run, quick_test_pass_thresholds
from run_sensory import run_sensory
import time, fcntl


def test_pipelines(test, record_property):
    wav_file = test[0] 
    wav_name = os.path.basename(wav_file)
    arch = test[1]
    target = test[2]

    input_file = os.path.join(pipeline_input_dir, wav_name)
    if not os.path.isfile(input_file): 
        convert_input_wav(wav_file, input_file)

    chans, rate, samps, bits = get_wav_info(input_file)
    print(f"Processing a {samps//rate}s track")
    t0 = time.time()
    output_file = process_file(input_file, arch, target)
    tot = time.time() - t0
    print(f"Processing took {tot:.2f}s")

    keyword_file = convert_keyword_wav(output_file, arch, target)
    detections =run_sensory(keyword_file)
    print(f"{wav_name} : {detections}", file=sys.stderr)

    with open(results_log_file, "a") as log:
        fcntl.flock(log, fcntl.LOCK_EX)
        log.write(f"{wav_name},{arch},{target},{detections},\n") 
        fcntl.flock(log, fcntl.LOCK_UN)


    record_property("Target", target)
    record_property("Pipeline architecturer", arch)
    record_property("Wakewords", detections)

    #Fail only if in quicktest mode
    if full_pipeline_run == 0:
        passed = True
        for key in quick_test_pass_thresholds:
            if key in keyword_file:
                pass_mark = quick_test_pass_thresholds[key]
                if detections < pass_mark:
                    print(f"Quick test failed for file {wav_name}, architecture {arch}, target {target}. Expected {pass_mark} keywords, got {detections}", file=sys.stderr)
                    passed = False
        assert passed
    return True
