# Copyright 2022 XMOS LIMITED.
# This Software is subject to the terms of the XMOS Public Licence: Version 1.

import os
import subprocess
import argparse
import sys
import time
import tempfile

if sys.platform == "darwin":
    SPOT_EVAL_EXE = "spot-eval_x86_64-apple-darwin"
else:
    SPOT_EVAL_EXE = "spot-eval_x86_64-pc-linux-gnu"

SPOT_EVAL_OLD_MODEL = "spot-alexa-rpi-31000.snsr"
SPOT_EVAL_NEW_MODEL = "thfft_alexa_enus_v6_1mb.snsr"

def parse_arguments():
    parser = argparse.ArgumentParser()
    parser.add_argument("--input", type=str, nargs='?', help="input wav file", default='')
    args = parser.parse_args()
    return args

def run_file(input_filename, sensory_model):
    try:
        sensory_path = os.environ['SENSORY_PATH']
        print("sensory_path = %s"%(sensory_path))
    except:
        sensory_path = os.path.join(os.path.dirname(os.path.abspath(__file__)), '../../../sensory_sdk/')
        print('env variable SENSORY_PATH not set. looking for sensory in the default path ',sensory_path)

    spot_eval_exe = os.path.expanduser(os.path.join(sensory_path, "spot_eval_exe", SPOT_EVAL_EXE))
    spot_model = os.path.expanduser(os.path.join(sensory_path, "model", sensory_model))
    if not os.path.isfile(spot_eval_exe):
        print('spot-eval not present in %s ',spot_eval_exe)
        # assert(False)
    if not os.path.isfile(spot_model):
        print('model not present in %s ',spot_model)
        # assert(False)

    max_retries = 10
    retry = 0
    output = None
    while retry < max_retries:
        try:
            output = subprocess.check_output('%s -t %s -s operating-point=5 -v %s' %(spot_eval_exe, spot_model, input_filename), shell=True)
            break
        except subprocess.CalledProcessError as e:
            print(f"RETRY: {retry}", e.output,file=sys.stderr)
            time.sleep(1)
            # assert(False)
        retry += 1
    assert output, f"Unable to run {spot_eval_exe} after {max_retries} attempts"


    # Compute the number of occurrences of 'alexa' to get the number of detection
    detections = len(output.decode().split('alexa')) - 1
    return detections

def run_sensory(input_filename, old_model=True):
    if old_model:
        detections = run_file(input_filename, SPOT_EVAL_OLD_MODEL)
    else:
        detections = run_file(input_filename, SPOT_EVAL_NEW_MODEL)
    return detections

if __name__ == "__main__":

    args = parse_arguments()
    detections = run_sensory(args.input)
    print("detections = %d"%(detections))