# Copyright 2022 XMOS LIMITED.
# This Software is subject to the terms of the XMOS Public Licence: Version 1.
import xscope_fileio, xtagctl
import shutil
import os
import re
import numpy as np
import subprocess

input_wav = os.path.abspath("../../../examples/bare-metal/vad/input.wav")
vad_profile_xe = os.path.abspath("../../../build/test/lib_vad/test_vad_profile/bin/test_vad_profile_VAD_ENABLED.xe")
vad_profile_xe_empty = os.path.abspath("../../../build/test/lib_vad/test_vad_profile/bin/test_vad_profile_VAD_DISABLED.xe")

core_clock_MHz = 600

def ticks_to_cycles(ticks):
    #Note we assume 5 of fewer threads running for this to be accurate
    thread_clock_MHz = core_clock_MHz / 5
    cycles = ticks * thread_clock_MHz / 100
    return int(cycles)

def cycles_to_mips(cycles):
    frame_time_ms = 15
    fps = 1000 / frame_time_ms
    ips = cycles * fps
    return ips / 1000000

def parse_profile_output(stdout):
    vad_init_time = 0
    vad_init_start = 0
    vad_proc_start = 0
    vad_proc_times = []
    for line in stdout:
        # print("line:", line)
        m = re.match(r'.+frame\s+(\d+).*', line)
        if m:
            frame = int(m.groups(0)[0])
        m = re.match(r".+Profile\s+(\d+),\s+(\d+).*", line)
        if m:
            prof = int(m.groups(0)[0])
            ticks = int(m.groups(0)[1])
            if prof == 0:
                vad_init_start = ticks
            if prof == 1:
                vad_init_time = ticks - vad_init_start
            if prof == 2:
                vad_proc_start = ticks
            if prof == 3:
                vad_proc_time = ticks - vad_proc_start
                vad_proc_times.append(vad_proc_time)

    vad_init_cycles = ticks_to_cycles(vad_init_time)
    vad_proc_cycles = [ticks_to_cycles(ticks) for ticks in vad_proc_times]
    mean = np.mean(vad_proc_cycles)

    report = ""
    report += f"VAD Timing report\n"
    report += f"-----------------\n\n"
    report += f"Init cycles:        {vad_init_cycles}\n"
    report += f"Proc cycles mean:   {np.mean(vad_proc_cycles)}\n"
    report += f"Proc cycles max:    {np.amax(vad_proc_cycles)} at frame {np.argmax(vad_proc_cycles)}\n"
    report += f"Proc cycles min:    {np.amin(vad_proc_cycles)} at frame {np.argmin(vad_proc_cycles)}\n"
    report += f"Proc cycles stddev: {np.std(vad_proc_cycles)}\n"
    report += f"Max MIPS:           {cycles_to_mips(np.amax(vad_proc_cycles))}\n"

    return report


def test_mem_usage():
    old_cwd = os.getcwd()

    new_directory = os.path.abspath("../../../build/")
    os.chdir(new_directory)    
    subprocess.call('cmake -S.. -DCMAKE_TOOLCHAIN_FILE=../etc/xmos_toolchain.cmake -DPython3_FIND_VIRTUALENV="ONLY" -DBUILD_TESTS=ON'.split())

    new_directory = os.path.abspath("../../../build/test/lib_vad/test_vad_profile/")
    os.chdir(new_directory)    
    try:       
        subprocess.call("make clean".split())
        test_output = subprocess.check_output("make -j".split(), text=True, stderr=subprocess.STDOUT)
    except subprocess.CalledProcessError as e:
        # Unity exits non-zero if an assertion fails
        test_output = e.output
    # print(test_output)
    os.chdir(old_cwd)

    mem_used = []
    for line in test_output.split("\n"):
        m = re.match(r".*Memory available:\s+(\d+).+used:\s+(\d+).*", line)
        print(line)
        if m:
            mem_used.append(int(m.groups(0)[1]))
    mem_used.sort(reverse=True) #The two largest will be tile[0] with and without vad
    vad_mem = mem_used[0] - mem_used[1]

    report = ""
    report += f"VAD Memory report\n"
    report += f"-----------------\n\n"
    report += f"Memory used bytes:     {vad_mem}\n"

    print(report)
    with open ("vad_memory_report.log", "w") as rf:
        rf.write(report)

def test_vad_profile():
    try:
        shutil.copy2(input_wav, "input.wav")
    except shutil.SameFileError as e:
        pass
    except IOError as e:
         print('Error: %s' % e.strerror)
         assert False, f"Cannot find input wav {input_wav}"

    with xtagctl.acquire("XCORE-AI-EXPLORER") as adapter_id:
        with open("vad.txt", "w+") as ff:
            xscope_fileio.run_on_target(adapter_id, vad_profile_xe, stdout=ff)
            ff.seek(0)
            stdout = ff.readlines()

        xcore_stdo = []
        #ignore lines that don't contain [DEVICE]. Remove everything till and including [DEVICE] if [DEVICE] is present
        for line in stdout:
            m = re.search(r'^\s*\[DEVICE\]', line)
            if m is not None:
                xcore_stdo.append(re.sub(r'\[DEVICE\]\s*', '', line))

        report = parse_profile_output(stdout)
        print(report)
        with open ("vad_profile_report.log", "w") as rf:
            rf.write(report)

if __name__ == "__main__":
    test_mem_usage()
    test_vad_profile()