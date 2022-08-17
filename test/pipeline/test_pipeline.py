# Copyright 2022 XMOS LIMITED.
# This Software is subject to the terms of the XMOS Public Licence: Version 1.

import os, sys, shutil
import pytest
from pipeline_test_utils import process_file, convert_keyword_wav, log_vnr
from conftest import pipeline_input_dir, results_log_file, full_pipeline_run, quick_test_pass_thresholds, get_wav_info
from run_sensory import run_sensory

if sys.platform != "darwin":
    from run_amazon_wwe import run_amazon_wwe

import time, fcntl


def test_pipelines(test, record_property):
    wav_file = test[0] 
    wav_name = os.path.basename(wav_file)
    arch = test[1]
    target = test[2]
    
    input_file = os.path.join(pipeline_input_dir, wav_name)

    chans, rate, samps, bits = get_wav_info(input_file)
    print(f"Processing a {samps//rate}s track")
    t0 = time.time()
    output_file, stdo = process_file(input_file, arch, target)
    tot = time.time() - t0
    print(f"Processing took {tot:.2f}s")

    if not os.path.isfile(output_file): 
        return 

    keyword_file = convert_keyword_wav(output_file, arch, target)
    sensory_old_detections =run_sensory(keyword_file)
    sensory_new_detections =run_sensory(keyword_file, old_model=False)
    if sys.platform != "darwin":
        amazon_detections = run_amazon_wwe(keyword_file)
    else:
        amazon_detections = 0
    print(f"{wav_name} : kwd sensory detections {sensory_old_detections}, Amazon wwe detections {amazon_detections}", file=sys.stderr)
    print(f"outputfile = {output_file}, keyword_file = {keyword_file}")

    # Log vnr input and output predictions if present in the stdout. To log vnr prediction, compile the pipeline code with PRINT_VNR_PREDICTION defined as 1
    log_vnr(stdo, input_file, arch, target)

    with open(results_log_file, "a") as log:
        fcntl.flock(log, fcntl.LOCK_EX)
        log.write(f"{wav_name},{arch},{target},{sensory_old_detections},{sensory_new_detections},{amazon_detections}\n") 
        fcntl.flock(log, fcntl.LOCK_UN)


    record_property("Target", target)
    record_property("Pipeline architecture", arch)
    record_property("Sensory Wakewords", sensory_old_detections)
    record_property("Amazon Wakewords", amazon_detections)

    #Fail only if in quicktest mode
    if full_pipeline_run == 0:
        if arch == "alt_arch" and target != "python": # Only test keywords on quick run on full pipeline alt_arch. Python pipeline doesn't exist for alt-arch at the moment.
            passed = True
            for key in quick_test_pass_thresholds:
                if key in keyword_file:
                    pass_mark = quick_test_pass_thresholds[key]
                    if amazon_detections < pass_mark:
                        print(f"Quick test failed for file {wav_name}, architecture {arch}, target {target}. Expected {pass_mark} keywords, got {sensory_old_detections}", file=sys.stderr)
                        passed = False
            assert passed
    return True
