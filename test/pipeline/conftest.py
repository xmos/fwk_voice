# Copyright (c) 2022, XMOS Ltd, All rights reserved
import pytest
import os, re

hydra_audio_base_dir = os.path.expanduser("~/hydra_audio/")
keyword_input_dir = os.path.abspath("./keyword_input/")
pipeline_input_dir = os.path.abspath("./pipeline_input/")
pipeline_x86_bin = os.path.abspath("../../build/examples/bare-metal/pipeline_single_threaded/bin/avona_example_bare_metal_pipeline_single_thread")
pipeline_xe_bin = os.path.abspath("../../build/examples/bare-metal/pipeline_single_threaded/bin/avona_example_bare_metal_pipeline_single_thread.xe")


all_files_list = []

def pytest_sessionstart(session):
    """ before session.main() is called. """
    quick_test_setting = 1
    try:
        quick_test_setting = int(os.environ['RUN_QUICK_TEST'])
    except:
        print('Warning: RUN_QUICK_TEST environment variable not set. Running quick tests by default')

    hydra_audio_path = os.path.join(hydra_audio_base_dir, "xvf3510_no_processing_xmos_test_suite")
    global all_files_list 
    all_files_list = [os.path.join(hydra_audio_path, filename) for filename in os.listdir(hydra_audio_path)]
    

    #create pipeline input and sensory input directories
    os.makedirs(pipeline_input_dir, exist_ok=True)
    os.makedirs(keyword_input_dir, exist_ok=True)

def pytest_generate_tests(metafunc):
    # metafunc.parametrize("test_audio", all_files_list, ids=all_files_list)
    metafunc.parametrize("test_audio", [all_files_list[0]], ids=[all_files_list[0]])
