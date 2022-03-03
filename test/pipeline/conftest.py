# Copyright 2022 XMOS LIMITED.
# This Software is subject to the terms of the XMOS Public Licence: Version 1.

import pytest
import os, re, sys

hydra_audio_base_dir = os.path.expanduser("~/hydra_audio/")
pipeline_input_dir = os.path.abspath("./pipeline_input/")
pipeline_output_dir = os.path.abspath("./pipeline_output/")
keyword_input_dir = os.path.abspath("./keyword_input/")
pipeline_x86_bin = os.path.abspath("../../build/examples/bare-metal/pipeline_single_threaded/bin/avona_example_bare_metal_pipeline_single_thread")
pipeline_xe_bin = os.path.abspath("../../build/examples/bare-metal/pipeline_single_threaded/bin/avona_example_bare_metal_pipeline_single_thread.xe")
results_log_file = os.path.abspath("pipeline_results.txt")

all_files_list = []

def pytest_sessionstart(session):
    """ before session.main() is called. """
    try:
        quick_test_setting = int(os.environ['RUN_QUICK_TEST'])
    except:
        print('Warning: RUN_QUICK_TEST environment variable not set. Running quick tests by default')
        quick_test_setting = 1

    if quick_test_setting == 1:
        hydra_audio_path = os.path.join(hydra_audio_base_dir, "xvf3510_no_processing_xmos_test_suite")
    else:
        hydra_audio_path = os.path.join(hydra_audio_base_dir, "xvf3510_no_processing_xmos_test_suite_subset")


    global all_files_list 
    all_files_list = [os.path.join(hydra_audio_path, filename) for filename in os.listdir(hydra_audio_path) if filename.endswith(".wav")]
    

    #create pipeline input and sensory input directories
    os.makedirs(pipeline_input_dir, exist_ok=True)
    os.makedirs(pipeline_output_dir, exist_ok=True)
    os.makedirs(keyword_input_dir, exist_ok=True)

    #Start with empty logfile
    if os.path.exists(results_log_file):
        os.remove(results_log_file)

def pytest_sessionfinish(session):
    with open(results_log_file, "r+") as lf:
        log = lf.readlines()
        log = sorted(log)
        lf.seek(0)
        lf.writelines(log)


def pytest_generate_tests(metafunc):
    metafunc.parametrize("test_audio", all_files_list, ids=all_files_list)
    # n_procs = 8
    # metafunc.parametrize("test_audio", all_files_list[0:n_procs], ids=all_files_list[0:n_procs]) #just a few
