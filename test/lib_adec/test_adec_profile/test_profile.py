# Copyright 2022 XMOS LIMITED.
# This Software is subject to the terms of the XMOS Public Licence: Version 1.
import numpy as np
import os
import tempfile
import shutil
import subprocess
import scipy.io.wavfile
import scipy.signal as spsig
import xscope_fileio
import xtagctl
import io
import glob
import re
import argparse
import pytest
import glob

src_folder = os.path.join(os.path.dirname(os.path.abspath(__file__)),
                              '..', 'test_wav_adec', 'src')

hydra_audio_path = os.environ.get('hydra_audio_PATH', '~/hydra_audio')
wav_input_file = glob.glob(f'{hydra_audio_path}/adec_profile_test_stream/*.wav', recursive=True)[0]

def run_proc_with_output(cmd):
    proc = subprocess.Popen(cmd, stdout=subprocess.PIPE)
    stdout = proc.stdout.readlines()
    return [str(line, 'utf-8') for line in stdout]

def extract_memory_stats(stdout):
    memory_used = None
    for line in stdout:
        rs = re.search("Memory\savailable:\s+(\d+),\s+used:\s+(\d+).+", line)
        if rs:
            memory_used = int(rs.group(2))
    return memory_used

xc_in_file_name = "input.wav"
xc_out_file_name = "output.wav"
runtime_args_file = "args.bin"
def run_pipeline_xe(pipeline_xe, run_config, threads, audio_in, audio_out, profile_dump_file=None):
    #threads argument is only for logging the number of threads aec was built with into a file
    with open(runtime_args_file, "wb") as fargs:
        fargs.write(f"main_filter_phases {run_config.num_main_filt_phases}\n".encode('utf-8'))
        fargs.write(f"shadow_filter_phases {run_config.num_shadow_filt_phases}\n".encode('utf-8'))
        fargs.write(f"y_channels {run_config.num_y_channels}\n".encode('utf-8'))
        fargs.write(f"x_channels {run_config.num_x_channels}\n".encode('utf-8'))
    
    tmp_folder = tempfile.mkdtemp(prefix='tmp_', dir='.')
    shutil.copy2(runtime_args_file, os.path.join(tmp_folder, runtime_args_file))
    shutil.copy2(audio_in, os.path.join(tmp_folder, xc_in_file_name))
    shutil.copy2(runtime_args_file, os.path.join(tmp_folder, runtime_args_file))
    
    prev_path = os.getcwd()
    os.chdir(tmp_folder)    
        
    with xtagctl.acquire("XCORE-AI-EXPLORER") as adapter_id:
        print(f"Running on {adapter_id}")
        with open("ic_prof.txt", "w+") as ff:
            xscope_fileio.run_on_target(adapter_id, pipeline_xe, stdout=ff)
            ff.seek(0)
            stdout = ff.readlines()

        xcore_stdo = []
        #ignore lines that don't contain [DEVICE]. Remove everything till and including [DEVICE] if [DEVICE] is present
        for line in stdout:
            m = re.search(r'^\s*\[DEVICE\]', line)
            if m is not None:
                xcore_stdo.append(re.sub(r'\[DEVICE\]\s*', '', line))
        
    os.chdir(prev_path)

    with open(profile_dump_file, 'w') as fp:
        for line in xcore_stdo:
            fp.write(f"{line}\n")
    parse_profile_log(run_config, threads, xcore_stdo, worst_case_file=f"adec_prof_{run_config.config_str()}_{threads}threads.log")

    shutil.rmtree(tmp_folder, ignore_errors=True)    

'''
output: profile_file contains profiling info for all frames.
output: worst_case_file contains profiling info for worst case frame
output: mapping_file contains the profiling index to tag string mapping. This is useful when adding a new prof() call to look-up indexes that are already used
        in order to avoid duplicating indexes
'''
def parse_profile_log(run_config, threads, prof_stdo, profile_file="parsed_profile.log", worst_case_file="worst_case.log", mapping_file="profile_index_to_tag_mapping.log"):
    profile_strings = {}
    profile_regex = re.compile(r'\s*prof\s*\(\s*(\d+)\s*,\s*"(.*)"\s*\)\s*;')
    #find all source files that might have a prof() function call
    src_files = glob.glob(f'{src_folder}/**/*.xc', recursive=True)
    src_files = src_files + glob.glob(f'{src_folder}/**/*.c', recursive=True)
    for file in src_files:
        with open(file, 'r') as fd:
            lines = fd.readlines()
        for line in lines:
            #look for prof(profiling_index, tag_string) type of calls
            m = profile_regex.match(line)
            if m:
                if m.group(1) in profile_strings:
                    print(f"Profiling index {m.group(1)} used more than once with tags '{profile_strings[m.group(1)]}' and '{m.group(2)}'.")
                    assert(False)
                #add to a dict[profile_index] = tag_string structure to create a integer index -> tag string mapping
                profile_strings[m.group(1)] = m.group(2)

    #log profile_strings in a file so it's easy for a user adding a new prof calls to look up already used indexes
    with open(mapping_file, 'w') as fp:
        for index in profile_strings:
            fp.write(f'{index:<4} {profile_strings[index]}\n')
    
    #parse stdo output and for every frame, generate a dictionary that stores dict[tag_string] = timer_snapshot 
    all_frames = []
    tags = {} #dictionary that stores dict[tag_string] = timer_snapshot information
    profile_regex = re.compile(r'Profile\s*(\d+)\s*,\s*(\d+)')
    #look for start of frame
    frame_regex = re.compile(r'frame\s*(\d+)')
    frame_num = 0
    for line in prof_stdo:
        m = frame_regex.match(line)
        if m:
            if frame_num:
                #append previous frames profiling info to all_frames
                all_frames.append(tags)
                tags = {} #reset tags
            frame_num += 1
        m = profile_regex.match(line)
        if m:
            prof_index = m.group(1)
            prof_str = profile_strings[prof_index]
            tags[profile_strings[m.group(1)]] = int(m.group(2))
    
    frame_num = 0
    worst_case_frame = ()
    with open(profile_file, 'w') as fp:
        fp.write(f'{"Tag":<44} {"Cycles":<12} {"% of total cycles":<10}\n')
        for tags in all_frames: #look at framewise profiling information
            fp.write(f"Frame {frame_num}\n")
            total_cycles = 0
            #convert from (start_ tag_string, timer_snapshot), (end_ tag_string, timer_snapshot) type information to (tag_string without start_ or end_ prefix, timer cycles between start_ and end_ tag_string) 
            this_frame_tags = {} #structure to store this frame's dict[tag_string] = cycles_between_start_and_end info so that we can use it later to print cycles as well as % of overall cycles
            for tag in tags:
                if tag.startswith('start_'):
                    end_tag = 'end_' + tag[6:]
                    cycles = tags[end_tag] - tags[tag]
                    this_frame_tags[tag[6:]] = cycles
                    total_cycles += cycles
            #this_frame is a tuple of (dictionary dict[tag_string] = cycles_between_start_and_end, total cycle count, frame_num)
            this_frame = (this_frame_tags, total_cycles, frame_num)

            #now write this frame's info in file
            for key, value in this_frame[0].items():
                fp.write(f'{key:<44} {value:<12} {round((value/float(this_frame[1]))*100,2):>10}% \n')
            fp.write(f'{"TOTAL_CYCLES":<32} {this_frame[1]}\n')
            if frame_num == 0:
                worst_case_frame = this_frame
            else:
                if worst_case_frame[1] < this_frame[1]:
                    worst_case_frame = this_frame
            frame_num += 1

        with open(worst_case_file, 'w') as fp:
            fp.write(f"Config: Threads ({threads}), Y_channels ({run_config.num_y_channels}), X_channels ({run_config.num_x_channels}), Main filter phases ({run_config.num_main_filt_phases}), Shadow filter phases ({run_config.num_shadow_filt_phases})\n")            
            fp.write(f"Worst case frame = {worst_case_frame[2]}\n")
            #in the end, print the worst case frame
            for key, value in worst_case_frame[0].items():
                fp.write(f'{key:<44} {value:<12} {round((value/float(worst_case_frame[1]))*100,2):>10}% \n')
            worst_case_timer_cycles = np.float64(worst_case_frame[1])
            fp.write(f'{"Worst_case_frame_timer(100MHz)_cycles":<32} {worst_case_timer_cycles}\n')
            worst_case_processor_cycles = (worst_case_timer_cycles/100) * 120
            fp.write(f'{"Worst_case_frame_processor(120MHz)_cycles":<32} {worst_case_processor_cycles}\n')
            #0.015 is seconds_per_frame. 1/0.015 is the frames_per_second.
            #processor_cycles_per_frame * frames_per_sec = processor_cycles_per_sec. processor_cycles_per_sec/1000000 => MCPS
            mcps = "{:.2f}".format((worst_case_processor_cycles / 0.015) / 1000000)
            fp.write(f'{"MCPS":<10} {mcps} MIPS\n')


class aec_config:
    def __init__(self, config_str):
        config = config_str.split()
        assert len(config) == 4, "Incorrect length config specified!"
        self.num_y_channels = config[0]
        self.num_x_channels = config[1]
        self.num_main_filt_phases = config[2]
        self.num_shadow_filt_phases = config[3]
    def print_config(self):
        print("Config = ", self.num_y_channels,  self.num_x_channels, self.num_main_filt_phases, self.num_shadow_filt_phases)
    def config_str(self):
        return f"{self.num_y_channels}ych_{self.num_x_channels}xch_{self.num_main_filt_phases}mainph_{self.num_shadow_filt_phases}shadph"


xe_files = glob.glob('../../../build/test/lib_adec/test_adec_profile/bin/*.xe')
#create wav input
@pytest.fixture(scope="session", params=xe_files)
def setup(request):
    xe = os.path.abspath(request.param) #get .xe filename including path
    #extract stem part of filename
    name = os.path.splitext(os.path.basename(xe))[0] #This should give a string of the form test_aec_profile_<threads>_<ychannels>_<xchannels>_<mainphases>_<shadowphases>
    config = (f"{name}".split('_'))[-5:] #Split by _ and pick up the last 5 values to get the config
    threads = config[0]
    rest_of_config = ' '.join(config[1:]) #remaining build config in "<ych> <xch> <mainph> <shadowph>" form
    return xe, aec_config(rest_of_config), threads 

#For every build_config, test with all specified run time configs
@pytest.mark.parametrize("run_config", ['', '1 2 15 5'])
def test_profile(setup, run_config):
    #run_config is the aec runtime configuration specified in '<num_y_channels> <num_x_channels> <num_main_filter_phases> <num_shadow_filter_phases>' format
    #if run_config is an empty string, run the configuration that was built
    print(f"config {run_config}")
    pipeline_xe, build_config, threads = setup
    if run_config == '':
        #test the configuration that was built
        print(f'test build_config')
        run_pipeline_xe(pipeline_xe, build_config, threads, wav_input_file, "output.wav", "profile.log") #threads is passed in only for logging purposes
    else:
        #test the specified run time configuration
        run_config = aec_config(run_config)
        run_pipeline_xe(pipeline_xe, run_config, threads, wav_input_file, "output.wav", "profile.log")
        print('test run_config')

