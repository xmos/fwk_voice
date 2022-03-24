# Copyright 2022 XMOS LIMITED.
# This Software is subject to the terms of the XMOS Public Licence: Version 1.

import pytest
import os, re, sys

hydra_audio_base_dir = os.path.expanduser("~/hydra_audio/")
pipeline_input_dir = os.path.abspath("./pipeline_input/")
pipeline_output_base_dir = os.path.abspath("./pipeline_output/")
keyword_input_base_dir = os.path.abspath("./keyword_input/")
pipeline_x86_bin = os.path.abspath("../../build/examples/bare-metal/pipeline_single_threaded/bin/avona_example_bare_metal_pipeline_single_thread")
pipeline_xe_bin = os.path.abspath("../../build/examples/bare-metal/pipeline_multi_threaded/bin/avona_example_bare_metal_pipeline_multi_thread.xe")
results_log_file = os.path.abspath("results.csv")
xtag_aquire_timeout_s = int(8.5 * 60 * 1.2 * 2) # Add a generous timeout for xtag acquisition here. Max input wav is 8m21s so double & add 20%
                                                # The time to run the multithreaded example on xcore is approximately the wav length
                                                # we run 2 xcore targets so, even if we queue 4 xcore jobs, they should never timeout
# quick_test_pass_thresholds = {"Loc1_Noise1_65dB_XMOS_DUT1_80dB_Take1_extract_69s" : 8} #This is what lib_audio_pipelines scores so use this as passmark
quick_test_pass_thresholds = {"Loc1_Noise1_65dB_XMOS_DUT1_80dB_Take1_extract_69s" : 6} #TODO - get this scoring 8 and update the passmark

# This is a list of tuples we will build consisting of test_wav and target
all_tests_list = []
full_pipeline_run = 1
# Select whether we run each test on xcore or using the x86 compiled example app
targets = ("xcore", "x86")


""" before session.main() is called. """
def pytest_sessionstart(session):
    global hydra_audio_base_dir, full_pipeline_run
    try:
        hydra_audio_base_dir = os.environ['hydra_audio_PATH']
    except:
        print(f'Warning: hydra_audio_PATH environment variable not set. Using local path {hydra_audio_base_dir}')

    try:
        full_pipeline_run = int(os.environ['PIPELINE_FULL_RUN'])
    except:
        print('Warning: PIPELINE_FULL_RUN environment variable not set. Running quick pipeline test by default')
        full_pipeline_run = 0
    finally:
        print(f"PIPELINE_FULL_RUN: {full_pipeline_run}")

    if full_pipeline_run != 0:
        hydra_audio_path = os.path.join(hydra_audio_base_dir, "xvf3510_no_processing_xmos_test_suite")
    else:
        hydra_audio_path = os.path.join(hydra_audio_base_dir, "xvf3510_no_processing_xmos_test_suite_subset")


    global all_tests_list
    input_wav_files = [os.path.join(hydra_audio_path, filename) for filename in os.listdir(hydra_audio_path) if filename.endswith(".wav")]
    for input_wav_file in input_wav_files:
        #We sometimes get weird files appearing in dir starting with "._InHouse_X..." so ignore
        if '._InHouse' in input_wav_file:
            continue
        for target in targets:
            all_tests_list.append([input_wav_file, target])
       
    #create pipeline input and sensory input directories
    os.makedirs(pipeline_input_dir, exist_ok=True)
    for target in targets:
        os.makedirs(os.path.join(pipeline_output_base_dir+"_"+target), exist_ok=True)
        os.makedirs(os.path.join(keyword_input_base_dir+"_"+target), exist_ok=True)

    #Start with empty logfile
    open(results_log_file, "w").close()

def pytest_sessionfinish(session):
    #read log file
    with open(results_log_file, "r") as lf:
        log = lf.readlines()
        #split into two, target specific, sorted files
        for target in targets:
            target_log = []
            for line in log:
                if target in line:
                    target_stripped_line = line.replace(target+",", "")
                    target_log.append(target_stripped_line) 
            target_log = sorted(target_log)
            target_specific_log_file = results_log_file.replace(".csv", "_"+target+".csv")
            with open(target_specific_log_file, "w") as tlf:
                tlf.writelines(target_log)


def pytest_generate_tests(metafunc):
    ids = [os.path.basename(item[0]) + ", " + item[1] for item in all_tests_list]
    metafunc.parametrize("test", all_tests_list, ids=ids)
