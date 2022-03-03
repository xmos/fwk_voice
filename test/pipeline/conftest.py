# Copyright 2022 XMOS LIMITED.
# This Software is subject to the terms of the XMOS Public Licence: Version 1.

import pytest
import os, re, sys

hydra_audio_base_dir = os.path.expanduser("~/hydra_audio/")
pipeline_input_dir = os.path.abspath("./pipeline_input/")
pipeline_output_base_dir = os.path.abspath("./pipeline_output/")
keyword_input_base_dir = os.path.abspath("./keyword_input/")
pipeline_x86_bin = os.path.abspath("../../build/examples/bare-metal/pipeline_single_threaded/bin/avona_example_bare_metal_pipeline_single_thread")
pipeline_xe_bin = os.path.abspath("../../build/examples/bare-metal/pipeline_single_threaded/bin/avona_example_bare_metal_pipeline_single_thread.xe")
results_log_file = os.path.abspath("pipeline_results.txt")

all_tests_list = []
# targets = ("xcore", "x86")
targets = ["x86"]


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


    global all_tests_list
    for target in targets:
        all_tests_list = [(os.path.join(hydra_audio_path, filename), target) for 
            filename in os.listdir(hydra_audio_path) if filename.endswith(".wav")]
       
    #create pipeline input and sensory input directories
    os.makedirs(pipeline_input_dir, exist_ok=True)
    for target in targets:
        os.makedirs(os.path.join(pipeline_output_base_dir+"_"+target), exist_ok=True)
        os.makedirs(os.path.join(keyword_input_base_dir+"_"+target), exist_ok=True)

    #Start with empty logfile
    open(results_log_file, "w").close()

def pytest_sessionfinish(session):
    with open(results_log_file, "r+") as lf:
        log = lf.readlines()
        log = sorted(log)
        lf.seek(0)
        lf.writelines(log)


def pytest_generate_tests(metafunc):
    ids = [os.path.basename(item[0]) + ", " + item[1] for item in all_tests_list]
    metafunc.parametrize("test", all_tests_list, ids=ids)
    # n_procs = 8
    # metafunc.parametrize("test_audio", all_files_list[0:n_procs], ids=all_files_list[0:n_procs]) #just a few
