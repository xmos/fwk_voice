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
import sys

ic_src_folder = os.path.join(os.path.dirname(os.path.abspath(__file__)), *"../../../examples/bare-metal/ic/src".split("/"))
thread_speed_mhz = (600 / 5)

in_file_name = "input.wav"
out_file_name = "output.wav"
def run_ic_xe(ic_xe, audio_in, audio_out, profile_dump_file=None):
    
    tmp_folder = tempfile.mkdtemp()
    shutil.copy2(audio_in, os.path.join(tmp_folder, in_file_name))
    
    prev_path = os.getcwd()
    os.chdir(tmp_folder)    
        
    with xtagctl.acquire("XCORE-AI-EXPLORER") as adapter_id:
        print(f"Running on {adapter_id} binary {ic_xe}")
        with open("ic_prof.txt", "w+") as ff:
            xscope_fileio.run_on_target(adapter_id, ic_xe, stdout=ff)
            ff.seek(0)
            stdout = ff.readlines()

        xcore_stdo = []
        #ignore lines that don't contain [DEVICE]. Remove everything till and including [DEVICE] if [DEVICE] is present
        for line in stdout:
            m = re.search(r'^\s*\[DEVICE\]', line)
            if m is not None:
                xcore_stdo.append(re.sub(r'\[DEVICE\]\s*', '', line))

    os.chdir(prev_path)
    #Save output file
    shutil.copy2(os.path.join(tmp_folder, audio_out), audio_out)

    with open(profile_dump_file, 'w') as fp:
        for line in xcore_stdo:
            fp.write(f"{line}\n")
    parse_profile_log(xcore_stdo, worst_case_file=f"ic_prof.log")

    shutil.rmtree(tmp_folder, ignore_errors=True)    

'''
output: profile_file contains profiling info for all frames.
output: worst_case_file contains profiling info for worst case frame
output: mapping_file contains the profiling index to tag string mapping. This is useful when adding a new prof() call to look-up indexes that are already used
        in order to avoid duplicating indexes
'''
def parse_profile_log(prof_stdo, profile_file="parsed_profile.log", worst_case_file="worst_case.log", mapping_file="profile_index_to_tag_mapping.log"):
    profile_strings = {}
    profile_regex = re.compile(r'\s*prof\s*\(\s*(\d+)\s*,\s*"(.*)"\s*\)\s*;')
    #find all ic source files that might have a prof() function call
    ic_files = glob.glob(f'{ic_src_folder}/**/*.xc', recursive=True)
    ic_files = ic_files + glob.glob(f'{ic_src_folder}/**/*.c', recursive=True)
    for file in ic_files:
        with open(file, 'r') as fd:
            lines = fd.readlines()
        for line in lines:
            #look for prof(profiling_index, tag_string) type of calls
            m = profile_regex.match(line)
            if m:
                # print("---", line)
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
        # print("***", line)
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
    init_frame = ()
    with open(profile_file, 'w') as fp:
        fp.write(f'{"Tag":>44} {"Cycles":<12} {"% of total cycles":<10}\n')
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
                    if tag.endswith('init'):  #Exclude init processing
                        init_frame = cycles
                    else:
                        total_cycles += cycles #Note we exclude init as part of our analysis
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
            fp.write(f"Worst case frame = {worst_case_frame[2]}\n")
            fp.write(f"{'init':<44} {init_frame:<12}\n")

            #in the end, print the worst case frame
            for key, value in worst_case_frame[0].items():
                if not "init" in key: #Exclude init processing
                    fp.write(f'{key:<44} {value:<12} {round((value/float(worst_case_frame[1]))*100,2):>10}% \n')
            worst_case_timer_ticks = int(worst_case_frame[1])
            fp.write(f'{"Worst_case_frame_timer(100MHz)_ticks":<44} {worst_case_timer_ticks}\n')
            worst_case_processor_cycles = int((worst_case_timer_ticks/100) * thread_speed_mhz)
            fp.write(f'{f"Worst_case_frame_processor({thread_speed_mhz}MHz)_cycles":<44} {worst_case_processor_cycles}\n')
            #0.015 is seconds_per_frame. 1/0.015 is the frames_per_second.
            #processor_cycles_per_frame * frames_per_sec = processor_cycles_per_sec. processor_cycles_per_sec/1000000 => MCPS
            mips = "{:.2f}".format((worst_case_processor_cycles / 0.015) / (thread_speed_mhz * 1000000) * thread_speed_mhz)
            fp.write(f'{"MCPS":<44} {mips} MIPS\n')




def leq_smooth(x, fs, T):
    len_x = x.shape[0]
    win_len = int(fs * T)
    win_count = len_x // win_len
    len_y = win_len * win_count

    y = np.reshape(x[:len_y], (win_len, win_count), 'F')

    leq = 10 * np.log10(np.mean(y ** 2.0, axis=0))
    t = np.arange(win_count) * T

    return t, leq

def make_impulse(RT, t=None, fs=None):
    scale = 0.005
    scale_noise = 0.00005
    a = 3.0 * np.log(10.0) / RT
    if t is None:
        t = np.arange(2.0*RT*fs) / fs
    N = t.shape[0]
    h = np.zeros(N)
    e = np.exp(-a*t)
    reflections = N // 100
    reflection_index = np.random.randint(N, size=reflections)
    for n, idx in enumerate(reflection_index):
        if n % 2 == 0:
            flip = 1
        else:
            flip = -1
        h[idx] = flip * scale * t[idx] * e[idx]
    h += scale_noise * np.random.randn(t.shape[0]) * e
    return h

def create_wav_input():
    fs = 16000
    N = fs * 10
    np.random.seed(500)    

    phases = 10
    fN = phases * 240

    # build impulse response
    RT = 0.15
    h = make_impulse(RT, fs=fs)
    h = h/h.max()
    hN = len(h)

    u = np.random.randn(N)

    d = spsig.convolve(u, h, 'full')[:N]
    if fN > hN:
        d = d[hN-1:hN-fN]
    else:
        d = d[hN-1:]

    sig_level = 0.01  #20dB attenuation
    d = d * sig_level
    u = u * sig_level
    
    # ideal results
    f_ideal = h[:fN]
    y_ideal = spsig.convolve(f_ideal, u, 'full')[hN-1:N]
    _, in_leq = leq_smooth(y_ideal, fs, 0.05)

    # run IC
    in_data = np.stack((d, u[hN-1:N]), axis=0)
    in_data_32bit = (np.asarray(in_data * np.iinfo(np.int32).max, dtype=np.int32)).T
    scipy.io.wavfile.write("input.wav", 16000, in_data_32bit)
    


xe_files = glob.glob('../../../build/test/lib_ic/test_ic_profile/bin/*.xe')
assert xe_files, "xe binary not found"
create_wav_input()


@pytest.fixture(scope="session", params=xe_files)
def setup(request):
    xe = os.path.abspath(request.param) #get .xe filename including path
    #extract stem part of filename
    name = os.path.splitext(os.path.basename(xe))[0] #This should give a string of the form test_ic_profile_<threads>_<ychannels>_<xchannels>_<mainphases>_<shadowphases>
    return xe

def test_profile(setup):
    ic_xe = setup
    run_ic_xe(ic_xe, "input.wav", "output.wav", "profile.log")

