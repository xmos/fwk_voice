
import os
import re
import glob
from collections import defaultdict
'''
output: profile_file contains profiling info for all frames.
output: worst_case_file contains profiling info for worst case frame
output: mapping_file contains the profiling index to tag string mapping. This is useful when adding a new prof() call to look-up indexes that are already used
        in order to avoid duplicating indexes
'''
def parse_profile_log(prof_stdo, src_folder, profile_file="parsed_profile.log", worst_case_file="worst_case.log"):
    profile_strings = {}
    profile_regex = re.compile(r'\s*prof\s*\(\s*(\d+)\s*,\s*"(.*)"\s*\)\s*;')
    #find all source files that might have a prof() function call
    src_files = glob.glob(f'{src_folder}/*.c', recursive=False)
    print(f"src_folder = {src_folder}, src_files = {src_files}")
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
                    tag_string = tag[6:] #Extract the string id after the start_. Eg. start_vnr_init has tag_string vnr_init.
                    end_tag = 'end_' + tag_string
                    cycles = tags[end_tag] - tags[tag]
                    this_frame_tags[tag_string] = cycles
                    if worst_case_dict[tag_string]['cycles'] < cycles: # Update worst case cycles and framenum for that string id.
                        worst_case_dict[tag_string] = {'cycles': cycles, 'frame_num': frame_num} 
                    total_cycles += cycles
            #this_frame is a tuple of (dictionary dict[tag_string] = cycles_between_start_and_end, total cycle count, frame_num)
            this_frame = (this_frame_tags, total_cycles, frame_num)

            #now write this frame's info in file
            for key, value in this_frame[0].items():
                fp.write(f'{key:<44} {value:<12} {round((value/float(this_frame[1]))*100,2):>10}% \n')
            fp.write(f'{"TOTAL_CYCLES":<32} {this_frame[1]}\n')
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

def parse_profiling_info(stdo, src_folder):
    xcore_stdo = []
    #ignore lines that don't contain [DEVICE]. Remove everything till and including [DEVICE] if [DEVICE] is present
    for line in stdo:
        m = re.search(r'^\s*\[DEVICE\]', line)
        if m is not None:
            xcore_stdo.append(re.sub(r'\[DEVICE\]\s*', '', line))

    #print(xcore_stdo)
    parse_profile_log(xcore_stdo, src_folder, worst_case_file=f"vnr_prof.log")
