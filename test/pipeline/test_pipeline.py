# Copyright 2022 XMOS LIMITED.
# This Software is subject to the terms of the XMOS Public Licence: Version 1.

import os, sys, shutil
import pytest
from pipeline_test_utils import process_file, get_wav_info, convert_input_wav, convert_keyword_wav
from conftest import pipeline_input_dir, results_log_file
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

    # record_list = run_pipelines.run(filename_plus_config['targets_enabled'], filename_plus_config['config'],\
    #               input_file=filename_plus_config['filename'], output_dir=filename_plus_config['output_dir'])


    # for r in record_list:
        #record_property("Wav", r['filename'])
            # ("Wakewords", r['filename'] + '.'+r['result'])
        #record_property("TimeElapsed", r['time_diff'])
            
    return True