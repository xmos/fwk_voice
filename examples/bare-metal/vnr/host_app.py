# Copyright 2022 XMOS LIMITED.
# This Software is subject to the terms of the XMOS Public Licence: Version 1.

import sys
import os
import platform
import ctypes
import numpy as np
import time
from scipy.io import wavfile
import matplotlib.pyplot as plt
import argparse
import shutil
sys.path.append(os.path.join(os.getcwd(), "../shared_src/python"))
import run_xcoreai
import re
import glob
from collections import defaultdict

def parse_arguments():
    parser = argparse.ArgumentParser()
    parser.add_argument("input_wav", nargs='?', help="input wav file")
    parser.add_argument("output_bin", nargs='?', help="vnr output bin file")
    parser.add_argument("--run_with_xscope_fileio", type=int, default=0, help="run with xscope_fileio")
    args = parser.parse_args()
    return args


FRAME_LENGTH = 240

CHUNK_SIZE = 256

PRINT_CALLBACK = ctypes.CFUNCTYPE(
    None, ctypes.c_ulonglong, ctypes.c_uint, ctypes.c_char_p
)

RECORD_CALLBACK = ctypes.CFUNCTYPE(
    None, ctypes.c_uint, ctypes.c_ulonglong, ctypes.c_uint,
    ctypes.c_long, ctypes.c_char_p
)

REGISTER_CALLBACK = ctypes.CFUNCTYPE(
    None, ctypes.c_uint, ctypes.c_uint, ctypes.c_uint, ctypes.c_uint, ctypes.c_uint,
    ctypes.c_char_p, ctypes.c_char_p, ctypes.c_uint, ctypes.c_char_p
)

class Endpoint(object):
    def __init__(self):
        tool_path = os.environ.get("XMOS_TOOL_PATH")
        ps = platform.system()
        if ps == 'Windows':
            lib_path = os.path.join(tool_path, 'lib', 'xscope_endpoint.dll')
        else:  # Darwin (aka MacOS) or Linux
            lib_path = os.path.join(tool_path, 'lib', 'xscope_endpoint.so')

        self._probe_info = {}

        self.lib_xscope = ctypes.CDLL(lib_path)

        self.output_offset = 0
        self.num_output_ints_per_frame = 2 #One float_s32_t output value per frame from vnr
        self.output_data_array = np.empty(0, dtype=np.int32)

        self._print_cb = self._print_callback_func()
        self._error = self.lib_xscope.xscope_ep_set_print_cb(self._print_cb)
        self._assert("[ERROR]Error while setting the print callback")

        self._record_cb = self._record_callback_func()
        self._error = self.lib_xscope.xscope_ep_set_record_cb(self._record_cb)
        self._assert("[ERROR]Error while setting the record callback")

        self._register_cb = self._register_callback_func()
        self._error = self.lib_xscope.xscope_ep_set_register_cb(self._register_cb)
        self._assert("[ERROR]Error while setting the register callback")

    def _assert(self, msg):
        if self._error:
            print(msg)
            assert(0)

    def _print_callback_func(self):
        def func(timestamp, length, data):
            self.on_print(timestamp, data[0:length])

        return PRINT_CALLBACK(func)

    def _record_callback_func(self):
        def func(id, timestamp, length, dataval, databytes):
            self.on_record(id, length, dataval)

        return RECORD_CALLBACK(func)

    def _register_callback_func(self):
        def func(id, type, r, g, b, name, uint, data_type, data_name):
            self._probe_info[id] = {
                'type' : type,
                'name' : name,
                'uint' : uint,
                'data_type' : data_type
            }
            self.on_register(id, type, name, uint, data_type)

        return REGISTER_CALLBACK(func)

    def on_print(self, timestamp, data):
        msg = data.decode().rstrip()

        print(msg)

    def on_record(self, id, length, dataval):
        self._error = id
        self._assert("[ERROR]Wrong channel is being used, see xscope.config for the channel info")
        self._error = length
        self._assert("[ERROR]You can't use xscope_bytes() in this application")

        #print('[HOST]Record callback has been triggered value = {}'.format(dataval))
        self.output_data_array = np.append(self.output_data_array, dataval)
        self.output_offset += 1
        #print("[HOST]Values recieved: ", self.output_offset")

    def on_register(self, id, type, name, uint, data_type):
        print ('[HOST]Probe registered: id = {}, type = {}, name = {}, uint = {}, data type = {}'.format(id, type, name, uint, data_type))

    def is_done(self):
        if self.output_offset == (self.num_output_ints_per_frame * self.num_frames):
            return 1
        else:
            return 0

    def connect(self, hostname="localhost", port="10234"):
        return self.lib_xscope.xscope_ep_connect(hostname.encode(), port.encode())

    def disconnect(self):
        self.lib_xscope.xscope_ep_disconnect()
        print("[HOST]Disconnected")

    def publish(self, data):
        #print(len(data))
        return self.lib_xscope.xscope_ep_request_upload(
            ctypes.c_uint(len(data)), ctypes.c_char_p(data)
        )

    def publish_frame(self, frame):
        SLEEP_DURATION = 0.025
        for i in range(0, len(frame), CHUNK_SIZE):
            self._error = self.publish(frame[i : i + CHUNK_SIZE])
            self._assert('[ERROR]Failed to send the data to XCORE')
            time.sleep(SLEEP_DURATION)

    def publish_wav(self, array):
        self.num_frames = int(len(array)/FRAME_LENGTH)
        print("total frames = ",self.num_frames)
        for i in range(0, len(array), FRAME_LENGTH):
            self.publish_frame(array[i : i + FRAME_LENGTH].tobytes())

def run_with_python_xscope_host(input_file, output_file):
    sr, input_data_array = wavfile.read(input_file)

    if (sr != 16000):
        print("[ERROR]Only 16kHz sample rate is supported")
        assert 0

    if len(np.shape(input_data_array)) != 1:
        print("[ERROR]Only single channel inputs are supported in this application")
        assert 0

    input_file_length = len(input_data_array)

    ep = Endpoint()
    if ep.connect():
        print("[ERROR]Failed to connect the endpoint")
    else:
        print("[HOST]Endpoint connected")
        time.sleep(1)

        ep.publish_wav(input_data_array)
         
        print("output_offset = ",ep.output_offset)
        while not ep.is_done():
            pass

    done_msg = "DONE"
    #err = ep.lib_xscope.xscope_ep_request_upload(ctypes.c_uint(len(done_msg)), ctypes.c_char_p(done_msg))
    #print(err)
    time.sleep(1)
    ep.disconnect()
    
    print(len(ep.output_data_array), ep.output_data_array.dtype)
    ep.output_data_array.astype(np.int32).tofile(output_file)
    print('[HOST]END!')
    return ep.output_data_array

'''
output: profile_file contains profiling info for all frames.
output: worst_case_file contains profiling info for worst case frame
output: mapping_file contains the profiling index to tag string mapping. This is useful when adding a new prof() call to look-up indexes that are already used
        in order to avoid duplicating indexes
'''
def parse_profile_log(prof_stdo, profile_file="parsed_profile.log", worst_case_file="worst_case.log"):
    src_folder = os.path.join(os.path.dirname(os.path.abspath(__file__)), 'src')
    profile_strings = {}
    profile_regex = re.compile(r'\s*prof\s*\(\s*(\d+)\s*,\s*"(.*)"\s*\)\s*;')
    #find all source files that might have a prof() function call
    src_files = glob.glob(f'{src_folder}/**/*.c', recursive=True)
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
    worst_case_dict = defaultdict(lambda : {'cycles': 0, 'frame_num': -1}) #Worst case cycles and the frame at which they happen for every tag string
    with open(profile_file, 'w') as fp:
        fp.write(f'{"Tag":<44} {"Cycles":<12} {"% of total cycles":<10}\n')
        for tags in all_frames: #look at framewise profiling information
            fp.write(f"Frame {frame_num}\n")
            total_cycles = 0
            #convert from (start_ tag_string, timer_snapshot), (end_ tag_string, timer_snapshot) type information to (tag_string without start_ or end_ prefix, timer cycles between start_ and end_ tag_string) 
            this_frame_tags = {} #structure to store this frame's dict[tag_string] = cycles_between_start_and_end info so that we can use it later to print cycles as well as % of overall cycles
            worst_case_cycles = {} #Store (worst case cycles for every tag string, framenum for worst case cycles for that tag string)
            for tag in tags:
                if tag.startswith('start_'):
                    end_tag = 'end_' + tag[6:]
                    cycles = tags[end_tag] - tags[tag]
                    this_frame_tags[tag[6:]] = cycles
                    if worst_case_dict[tag[6:]]['cycles'] < cycles:
                        worst_case_dict[tag[6:]] = {'cycles': cycles, 'frame_num': frame_num} 
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
        
        total_cycles = 0
        for key, val in worst_case_dict.items():
            total_cycles = total_cycles + val['cycles']
        
        name = "Function"
        worst_case ="worst_case_frame"
        cycles = "100MHz_timer_ticks"
        mcps = "120MHz_processor_MCPS"
        percent = "%_total"
        with open(worst_case_file, 'w') as fp:
            fp.write(f'{name:<24} {worst_case:<24} {cycles:<24} {mcps:<26} {percent}\n')
            for key, val in worst_case_dict.items():
                worst_case_frame = val['frame_num']
                worst_case_cycles_100MHz_timer = val['cycles']
                percentage_total = (float(worst_case_cycles_100MHz_timer)/total_cycles)*100
                processor_cycles_120MHz = (float(worst_case_cycles_100MHz_timer)/100)*120
                #0.015 is seconds_per_frame. 1/0.015 is the frames_per_second.
                #processor_cycles_per_frame * frames_per_sec = processor_cycles_per_sec.
                #processor_cycles_per_sec/1000000 => MCPS
                mcps = (processor_cycles_120MHz/0.015)/1000000 
                fp.write(f'{key:<24}: {worst_case_frame:<26} {round(worst_case_cycles_100MHz_timer,2):<24} {round(mcps,3):<24} {round(percentage_total, 3)}%\n')

def parse_profiling_info(stdo):
    xcore_stdo = []
    #ignore lines that don't contain [DEVICE]. Remove everything till and including [DEVICE] if [DEVICE] is present
    for line in stdo:
        m = re.search(r'^\s*\[DEVICE\]', line)
        if m is not None:
            xcore_stdo.append(re.sub(r'\[DEVICE\]\s*', '', line))

    #print(xcore_stdo)
    parse_profile_log(xcore_stdo, worst_case_file=f"vnr_prof.log")
    
def run_with_xscope_fileio(input_file, output_file):
    stdo = run_xcoreai.run("../../../build/examples/bare-metal/vnr/bin/avona_example_bare_metal_vnr_fileio.xe", input_file, return_stdout=True)
    parse_profiling_info(stdo)
    shutil.copyfile("vnr_out.bin", output_file)
    return np.fromfile(output_file, dtype=np.int32)

def plot_result(vnr_out, out_file):
    #Plot VNR output
    mant = np.array(vnr_out[0::2], dtype=np.float64)
    exp = np.array(vnr_out[1::2], dtype=np.int32)
    vnr_out = mant * (float(2)**exp)
    plt.plot(vnr_out)
    plt.xlabel('frames')
    plt.ylabel('vnr estimate')
    fig_instance = plt.gcf()
    plt.show()
    plotfile = f"plot_{os.path.splitext(os.path.basename(out_file))[0]}.png"
    fig_instance.savefig(plotfile)

    
if __name__ == "__main__":
    args = parse_arguments()
    print(f"input_file: {args.input_wav}, output_file: {args.output_bin}")
    if args.run_with_xscope_fileio:
        vnr_out = run_with_xscope_fileio(args.input_wav, args.output_bin)
    else:
        vnr_out = run_with_python_xscope_host(args.input_wav, args.output_bin)

    plot_result(vnr_out, args.input_wav)

