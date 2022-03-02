# Copyright (c) 2022, XMOS Ltd, All rights reserved
import os, sys, shutil
import pytest
from pipeline_test_utils import process_xcore, process_x86, get_wav_info, convert_input_wav, convert_keyword_wav
from conftest import pipeline_input_dir, keyword_input_dir, pipeline_x86_bin, pipeline_xe_bin
import time


def test_pipelines(test_audio, record_property):


    input_file = os.path.join(pipeline_input_dir, os.path.basename(test_audio))
    convert_wav(test_audio, input_file)
    keyword_file = os.path.join(keyword_input_dir, os.path.basename(test_audio))

    chans, rate, samps, bits = get_wav_info(input_file)
    print(f"Processing a {samps//rate}s track")
    t0 = time.time()
    process_x86(pipeline_x86_bin, input_file, keyword_file)
    t1 = time.time()
    print(f"Processing took {t1-t0}s")


    # record_list = run_pipelines.run(filename_plus_config['targets_enabled'], filename_plus_config['config'],\
    #               input_file=filename_plus_config['filename'], output_dir=filename_plus_config['output_dir'])


    # for r in record_list:
        #record_property("Wav", r['filename'])
            # ("Wakewords", r['filename'] + '.'+r['result'])
        #record_property("TimeElapsed", r['time_diff'])
            
    return True

