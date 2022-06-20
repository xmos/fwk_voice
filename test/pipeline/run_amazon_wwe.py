# Copyright 2022 XMOS LIMITED.
# This Software is subject to the terms of the XMOS Public Licence: Version 1.

import os
import subprocess
import argparse
import sys
import time
import tempfile
import shutil
import stat
from pathlib import Path


if sys.platform == "darwin":
    assert(False), "amazon_wwe filesim executable runs only on x86 Linux"
else:
    WW_FILESIM_EXE = "amazon_ww_filesim"

WW_MODEL = "WR_250k.en-US.alexa.bin"

def parse_arguments():
    parser = argparse.ArgumentParser()
    parser.add_argument("input", type=str, nargs='?', help="input wav file")
    args = parser.parse_args()
    return args

def run_file(input_filename, model):
    try:
        wwe_path = os.environ['AMAZON_WWE_PATH']
        print("wwe_path = %s"%(wwe_path))
    except:
        wwe_path = os.path.join(os.path.dirname(os.path.abspath(__file__)), '../../../amazon_wwe/')
        print('env variable AMAZON_WWE_PATH not set. looking for Amazon WWE in the default path ', wwe_path)

    filesim_exe = os.path.expanduser(os.path.join(wwe_path, "x86/", WW_FILESIM_EXE))
    ww_model = os.path.expanduser(os.path.join(wwe_path, "models/common", model))
    if not os.path.isfile(filesim_exe):
        print('filesim executable not present in %s ', filesim_exe)
        assert(False)
    if not os.path.isfile(ww_model):
        print('model not present in %s ',ww_model)
        assert(False)

    #There is an issue when lots of instances running the same kw bin, so make a copy and run own version
    tmp_folder = tempfile.mkdtemp(suffix=os.path.basename(input_filename), dir=".")
    prev_path = os.getcwd()
    os.chdir(tmp_folder)
    shutil.copyfile(filesim_exe, "kw_bin")
    os.chmod("kw_bin", stat.S_IXUSR)
    shutil.copyfile(ww_model, "kw_model")
    # There's this really srtange error where if the input stream path starts with a /, amazon_ww_filesim issues a warning, Warning: Can't open file and detects 0 keywords
    shutil.copy2(input_filename, "./")
    os.system(f"echo {os.path.basename(input_filename)} > list.txt")
    
    run_cmd = '%s list.txt -t 500 -m %s' %("./kw_bin", "kw_model")
    print("run_cmd = ", run_cmd)
    output = subprocess.check_output(run_cmd, shell=True)

    os.chdir(prev_path)
    shutil.rmtree(tmp_folder)

    # Compute the number of occurrences of 'alexa' to get the number of detection
    filename = os.path.splitext(input_filename)[0]
    filename = os.path.basename(Path(filename))
    detections = len(output.decode().split(f"{filename}: 'ALEXA'")) - 1
    return detections

def run_amazon_wwe(input_filename):
    detections = run_file(os.path.abspath(input_filename), WW_MODEL)
    return detections

if __name__ == "__main__":
    args = parse_arguments()
    assert(args.input != None), "Specify Input wav file"
    detections = run_amazon_wwe(os.path.abspath(args.input))
    print("detections = %d"%(detections))
