# Copyright 2018-2021 XMOS LIMITED.
# This Software is subject to the terms of the XMOS Public Licence: Version 1.

import pytest
import numpy as np
import os, shutil, tempfile
import xscope_fileio, xtagctl
import re

input_wav = os.path.abspath("../../../examples/bare-metal/vad/input.wav")
old_vad_xe = os.path.abspath("../../../build/test/lib_vad/compare_xc_c/run_old_vad.xe")
new_vad_xe = os.path.abspath("../../../build/examples/bare-metal/vad/bin/vad_example.xe")

def process_xe(xe_file, input_file):
    input_file = os.path.abspath(input_file)

    tmp_folder = tempfile.mkdtemp(suffix=os.path.basename(__file__))
    prev_path = os.getcwd()
    os.chdir(tmp_folder)

    shutil.copyfile(input_file, "input.wav")

    with xtagctl.acquire("XCORE-AI-EXPLORER") as adapter_id:
        with open("vad.txt", "w") as ff:
            xscope_fileio.run_on_target(adapter_id, xe_file, stdout=ff)

    with open("vad.txt") as ff:
        stdout = ff.readlines()

    os.chdir(prev_path)
    shutil.rmtree(tmp_folder)
    return stdout

def compare_vads(old, new):
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
    
        print(f"frame {new_frame}: {old_vad} {new_vad}")



def test_xc_c_comparison():
    stdo_old_vad = process_xe(old_vad_xe, input_wav)
    stdo_new_vad = process_xe(new_vad_xe, input_wav)

    compare_vads(stdo_old_vad, stdo_new_vad)
