# Copyright 2022 XMOS LIMITED.
# This Software is subject to the terms of the XMOS Public Licence: Version 1.

import os, sys, shutil
import pytest
from pipeline_test_utils import process_file, get_wav_info, convert_input_wav, convert_keyword_wav
from conftest import pipeline_input_dir, pipeline_output_dir, keyword_input_dir, results_log_file
from run_sensory import run_sensory
import time
import logging


def test_pipelines(test_audio, record_property): 
    #setup thread safe log file
    logger = logging.getLogger('log')
    logger.setLevel(logging.INFO)
    ch = logging.FileHandler(results_log_file)
    ch.setFormatter(logging.Formatter('%(message)s'))
    logger.addHandler(ch)


    wav_name = os.path.basename(test_audio)

    input_file = os.path.join(pipeline_input_dir, wav_name)
    convert_input_wav(test_audio, input_file)
    output_file = os.path.join(pipeline_output_dir, wav_name)

    chans, rate, samps, bits = get_wav_info(input_file)
    print(f"Processing a {samps//rate}s track")
    t0 = time.time()
    process_file(input_file, output_file, x86=True)
    tot = time.time() - t0
    print(f"Processing took {tot:.2f}s")

    keyword_file = os.path.join(keyword_input_dir, wav_name)
    convert_keyword_wav(output_file, keyword_file)

    detections =run_sensory(keyword_file)
    print(f"{wav_name} : {detections}")

    logger.info(f"{wav_name},{detections},")

    # record_list = run_pipelines.run(filename_plus_config['targets_enabled'], filename_plus_config['config'],\
    #               input_file=filename_plus_config['filename'], output_dir=filename_plus_config['output_dir'])


    # for r in record_list:
        #record_property("Wav", r['filename'])
            # ("Wakewords", r['filename'] + '.'+r['result'])
        #record_property("TimeElapsed", r['time_diff'])
            
    return True

