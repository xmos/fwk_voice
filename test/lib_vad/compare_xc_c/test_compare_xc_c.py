# Copyright 2022 XMOS LIMITED.
# This Software is subject to the terms of the XMOS Public Licence: Version 1.

import pytest
import numpy as np
import os, shutil, tempfile
import xscope_fileio, xtagctl
import re

input_wav = os.path.abspath("../../../examples/bare-metal/vad/input.wav")
old_vad_xe = os.path.abspath("../../../build/test/lib_vad/compare_xc_c/fwk_voice_run_old_vad.xe")
new_vad_xe = os.path.abspath("../../../build/examples/bare-metal/vad/bin/fwk_voice_example_bare_metal_vad.xe")

def process_xe(xe_file, input_file):
    input_file = os.path.abspath(input_file)

    tmp_folder = tempfile.mkdtemp(suffix=os.path.basename(__file__))
    prev_path = os.getcwd()
    os.chdir(tmp_folder)

    shutil.copyfile(input_file, "input.wav")

    with xtagctl.acquire("XCORE-AI-EXPLORER") as adapter_id:
        with open("vad.txt", "w+") as ff:
            xscope_fileio.run_on_target(adapter_id, xe_file, stdout=ff)
            ff.seek(0)
            stdout = ff.readlines()

    os.chdir(prev_path)
    shutil.rmtree(tmp_folder)
    return stdout

def compare_vads(old, new):
    allclose = True
    for old_line, new_line in zip(old, new):
        re_old = re.match(".+OLD frame: (\d+) vad: (\d+)", old_line)
        if re_old:
            old_frame = int(re_old.groups(0)[0])
            old_vad = int(re_old.groups(0)[1])
        re_new = re.match(".+frame: (\d+) vad: (\d+)", new_line)
        if re_new:
            new_frame = int(re_new.groups(0)[0])
            new_vad = int(re_new.groups(0)[1])
        assert old_frame == new_frame, f"Frame mismatch from old: {old_frame} vs new: {new_frame}"
    
        isclose = lambda x,y, diff: abs(x - y) <= diff
        max_diff = 0 #Exactly the same for both results
        if not isclose(old_vad, new_vad, max_diff):
            allclose = False
        print(f"frame {new_frame}: {old_vad} {new_vad}")
    print(f"Comparison: {allclose}")

    return allclose

def dump_both(old, new):
    for old_line, new_line in zip(old, new):
        print(old_line.strip(), "\n", new_line.strip())


def how_close(a, b):
    abs_diff = abs(a-b)
    if b == 0 or a == 0:
        rel = 9999999999
    else:
        if b > a:
            rel = b/a - 1.0
        else:
            rel = a/b - 1.0
    return rel, abs_diff


def is_close(a, b):
    rel_diff, abs_diff = how_close(a, b)

    if rel_diff < 0.00001 or abs_diff <= 2:
        return True 
    else:
        return False 


def compare_lines(old, new):
    frame = 0
    for old_line, new_line in zip(old, new):
        if not "MEL" in old_line:
            continue
        old_line = old_line.replace("[DEVICE] MEL ", "")
        new_line = new_line.replace("[DEVICE] MEL ", "")
        # print("**", old_line, new_line)
        old_vals = [s for s in old_line.split(' ') if s] #collapse conseqcuitive delimiters
        new_vals = [s for s in new_line.split(' ') if s]

        binc = 0
        for old_val, new_val in zip(old_vals, new_vals):
            if old_val == "\n":
                continue
            old = float(old_val)
            new = float(new_val)
            if not is_close(old, new):
                print(f"Disparity at frame {frame} bin {binc} old {old} new {new} :", how_close(old, new))
            binc += 1
        frame += 1

def test_xc_c_comparison():
    stdo_old_vad = process_xe(old_vad_xe, input_wav)
    stdo_new_vad = process_xe(new_vad_xe, input_wav)

    assert compare_vads(stdo_old_vad, stdo_new_vad)
    # dump_both(stdo_old_vad, stdo_new_vad)
