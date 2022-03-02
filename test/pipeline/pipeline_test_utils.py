import numpy as np
import os, shutil, tempfile
import xscope_fileio, xtagctl
import subprocess
import re

def process_xcore(xe_file, input_file, output_file):
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

def process_x86(bin_file, input_file, output_file):
    cmd = (bin_file, input_file, output_file)
    # print(" ".join(cmd))
    test_output = subprocess.check_output(cmd, text=True, stderr=subprocess.STDOUT)
    return stdout

def get_wav_info(input_file):
    chans = int(subprocess.check_output(("soxi", "-c", input_file)))
    rate = int(subprocess.check_output(("soxi", "-r", input_file)))
    samps = int(subprocess.check_output(("soxi", "-s", input_file)))
    bits = int(subprocess.check_output(("soxi", "-b", input_file)))

    return chans, rate, samps, bits

def convert_input_wav(input_file, output_file):
    chans, rate, samps, bits = get_wav_info(input_file)
    if not os.path.isfile(output_file): #optimisation. Only convert once if running test a lot locally
        if chans == 6:
            #for 6 channel wav file, first 4 channels are the mic input and last 2 channels are far-end audio
            subprocess.run(f"sox {input_file} -r 16000 -b 32 {output_file} remix 1 4 5 6".split()) #read mic in from channel spaced ~ 100mm, hence channel index 1 and 4(assuming 33mm spacing between mics)
        elif chans == 8:
            # for 8 channel wav file, first 2 channels are comms and asr outputs, followed by 4 channels of mic input
            # and last 2 channels are far-end audio
            subprocess.run(f"sox {input_file} -r 16000 -b 32 {output_file} remix 3 6 7 8".split())
        elif chans == 4:
            # for 4 channel wav file, first 2 channels are mic, followed by 2 channels of reference input
            # and last 2 channels are far-end audio
            subprocess.run(f"sox {input_file} -r 16000 -b 32 {output_file}".split())
        else:
            assert False, f"Error: input wav format not supported - chans:{chans}"
    return output_file

def convert_keyword_wav(input_file, output_file):
    #Strip off comms channel leaving just ASR
    subprocess.run(f"sox {input_file} -b 32 {output_file} remix 1".split())
    return output_file
