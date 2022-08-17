# Copyright 2022 XMOS LIMITED.
# This Software is subject to the terms of the XMOS Public Licence: Version 1.

import pytest
import os, re, sys
import subprocess

hydra_audio_base_dir = os.path.expanduser("~/hydra_audio/")
pipeline_input_dir = os.path.abspath("./pipeline_input/")
pipeline_output_base_dir = os.path.abspath("./pipeline_output/")
keyword_input_base_dir = os.path.abspath("./keyword_input/")
pipeline_bins = {
                "prev_arch" :    {"x86" : os.path.abspath("../../build/examples/bare-metal/pipeline_single_threaded/bin/fwk_voice_example_bare_metal_pipeline_single_thread"),
                                "xcore" : os.path.abspath("../../build/examples/bare-metal/pipeline_multi_threaded/bin/fwk_voice_example_bare_metal_pipeline_multi_thread.xe")},
                "alt_arch"  :    {"x86" : os.path.abspath("../../build/examples/bare-metal/pipeline_alt_arch/bin/fwk_voice_example_bare_metal_pipeline_alt_arch_st"),
                                "xcore" : os.path.abspath("../../build/examples/bare-metal/pipeline_alt_arch/bin/fwk_voice_example_bare_metal_pipeline_alt_arch_mt.xe")},
                "aec_ic_prev_arch" : {"xcore" : os.path.abspath("../../build/examples/bare-metal/pipeline_multi_threaded/bin/fwk_voice_example_pipeline_aec_ic.xe"),
                                      "x86" : os.path.abspath("../../build/examples/bare-metal/pipeline_single_threaded/bin/fwk_voice_example_st_pipeline_aec_ic.xe")},
                "aec_ic_ns_prev_arch" : {"xcore" : os.path.abspath("../../build/examples/bare-metal/pipeline_multi_threaded/bin/fwk_voice_example_pipeline_aec_ic_ns_agc.xe")},
                "aec_ic_agc_prev_arch" : {"xcore" : os.path.abspath("../../build/examples/bare-metal/pipeline_multi_threaded/bin/fwk_voice_example_pipeline_aec_ic_agc.xe")},
                "aec_ic_ns_agc_prev_arch" : {"xcore" : os.path.abspath("../../build/examples/bare-metal/pipeline_multi_threaded/bin/fwk_voice_example_pipeline_aec_ic_ns_agc.xe")}
                }
results_log_file = os.path.abspath("results.csv")
xtag_aquire_timeout_s = int(8.5 * 60 * 1.2 * 2) # Add a generous timeout for xtag acquisition here. Max input wav is 8m21s so double & add 20%
                                                # The time to run the multithreaded example on xcore is approximately the wav length
                                                # we run 2 xcore targets so, even if we queue 4 xcore jobs, they should never timeout
#These thresholds were arbitraily chosen but are designed to flag when a catastrophic drop off in pipeline performance occurs (regression test)
#By default we run alt-arch for quicktest
quick_test_pass_thresholds = {
"InHouse_XVF3510v080_v1.2_20190423_Loc1_Clean_XMOS_DUT1_80dB_Take1.wav" : 24, #24 max score. AEC test mainly
"InHouse_XVF3510v080_v1.2_20190423_Loc1_Noise2_70dB__Take1.wav" : 22, #25 max score. IC test mainly
"InHouse_XVF3510v080_v1.2_20190423_Loc1_Noise1_65dB_XMOS_DUT1_80dB_Take1.wav" : 24, #24 max score. AEC and IC test
"InHouse_XVF3510v080_v1.2_20190423_Loc2_Noise1_65dB__Take1.wav" : 24, #25 max score. IC test mainly
}

def get_wav_info(input_file):
    chans = int(subprocess.check_output(("soxi", "-c", input_file)))
    rate = int(subprocess.check_output(("soxi", "-r", input_file)))
    samps = int(subprocess.check_output(("soxi", "-s", input_file)))
    bits = int(subprocess.check_output(("soxi", "-b", input_file)))
    return chans, rate, samps, bits


def convert_input_wav(input_file, output_file):
    chans, rate, samps, bits = get_wav_info(input_file)
    extra_args = "" #"trim 0 5" #to test with short wavs
    if chans == 6:
        #for 6 channel wav file, first 2 channels are the mic input, followed by 2 channels of far-end audio, followed by 2 channels of pipeline output
        subprocess.run(f"sox {input_file} -r 16000 -b 32 {output_file} remix 1 2 3 4 {extra_args}".split())
    elif chans == 8:
        # for 8 channel wav file, first 2 channels are comms and asr outputs, followed by 4 channels of mic input
        # and last 2 channels are far-end audio
        subprocess.run(f"sox {input_file} -r 16000 -b 32 {output_file} remix 3 6 7 8 {extra_args}".split())
    elif chans == 4:
        # for 4 channel wav file, first 2 channels are mic, followed by 2 channels of reference input
        # and last 2 channels are far-end audio
        subprocess.run(f"sox {input_file} -r 16000 -b 32 {output_file} {extra_args}".split())
    else:
        assert False, f"Error: input wav format not supported - chans:{chans}"
    return output_file

# This is a list of tuples we will build consisting of test_wav and target
all_tests_list = []
# Select whether we run previous or al pipeline architecture. Default = alt hence first in list
full_pipeline_run = 1
# Select whether we run each test on xcore or using the x86 compiled example app or using python. Only AEC+IC pipeline exists for python right now.
targets = ["xcore", "python"]
architectures = []# These are populated below depending on full_pipeline_run

""" before session.main() is called. """
def pytest_sessionstart(session):
    global hydra_audio_base_dir, full_pipeline_run, architectures, all_tests_list
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
        hydra_audio_path = os.path.join(hydra_audio_base_dir, "xvf3510_no_processing_xmos_test_suite_subset_avona")
    
    # prev-arch: Standard config, full pipeline
    # alt-arch: Alt-arch config, full pipeline
    # aec_ic_prev_arch: Standard config, AEC+IC pipeline
    if full_pipeline_run:
        architectures = ["prev_arch", "alt_arch", "aec_ic_prev_arch", "aec_ic_ns_agc_prev_arch"]
    else:
        architectures = ["alt_arch", "aec_ic_prev_arch"]

    input_wav_files = [os.path.join(hydra_audio_path, filename) for filename in os.listdir(hydra_audio_path) if (filename.endswith(".wav"))]
    #input_wav_files = [os.path.join(hydra_audio_path, "InHouse_XVF3510v080_v1.2_20190423_Loc3_Noise2_80dB__Take1.wav")]
    #input_wav_files = [os.path.join(hydra_audio_path, "InHouse_XVF3510v080_v1.2_20190423_Loc1_Noise1_65dB_XMOS_DUT1_80dB_Take1.wav")]

    for input_wav_file in input_wav_files:
        #We sometimes get weird files appearing in dir starting with "._InHouse_X..." so ignore
        if '._InHouse' in input_wav_file:
            continue
        for target in targets:
            for arch in architectures:
                all_tests_list.append([input_wav_file, arch, target])
       
    #create pipeline input and sensory input directories
    os.makedirs(pipeline_input_dir, exist_ok=True)
    for target in targets:
        for arch in architectures:
            os.makedirs(os.path.join(pipeline_output_base_dir+"_"+arch+"_"+target), exist_ok=True)
            os.makedirs(os.path.join(keyword_input_base_dir+"_"+arch+"_"+target), exist_ok=True)
    
    for test in all_tests_list:
        wav_file = test[0]
        wav_name = os.path.basename(wav_file)
        input_file = os.path.join(pipeline_input_dir, wav_name)
        convert_input_wav(wav_file, input_file)

    #Start with empty logfile
    open(results_log_file, "w").close()

def pytest_sessionfinish(session):
    #read log file
    with open(results_log_file, "r") as lf:
        log = lf.readlines()
        #split into two, target specific, sorted files
        for arch in architectures:
            for target in targets:
                target_log = []
                for line in log:
                    fields = line.split(",")
                    if target == fields[2] and arch == fields[1]:
                        target_stripped_line = line.replace(target+",", "").replace(arch+",", "")
                        target_log.append(target_stripped_line) 
                target_log = sorted(target_log)
                target_specific_log_file = results_log_file.replace(".csv", "_Avona"+"_"+arch+"_"+target+".csv")
                with open(target_specific_log_file, "w") as tlf:
                    tlf.write("Input,Sensory_rpi-31000,Sensory_v6_1mb,Amazon_WR_250k.en-US\n")
                    tlf.writelines(target_log)


def pytest_generate_tests(metafunc):
    ids = [os.path.basename(item[0]) + ", " + item[1] + ", " + item[2] for item in all_tests_list]
    metafunc.parametrize("test", all_tests_list, ids=ids)
