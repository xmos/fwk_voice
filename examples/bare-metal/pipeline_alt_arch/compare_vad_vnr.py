import numpy as np
import sys
import os
import re
sys.path.append(os.path.join(os.getcwd(), "../shared_src/python"))
import run_xcoreai
import matplotlib.pyplot as plt
import argparse
this_filepath = os.path.dirname(os.path.realpath(__file__))
sys.path.append(os.path.join(this_filepath, "../../../test/pipeline"))
import run_sensory
import subprocess
import fcntl
import pytest

test_streams_path = "/Users/shuchitak/hydra_audio/vnr_agc_test_files"
input_wav_files = [os.path.join(test_streams_path, filename) for filename in os.listdir(test_streams_path) if filename.endswith(".wav")]
print(input_wav_files)
xe_path = os.path.join(this_filepath, "xe")
executables = [os.path.join(xe_path, xe) for xe in  os.listdir(xe_path) if xe.endswith(".xe")]
print(executables)

results_log_file = f"results.csv"
open(results_log_file, "w").close()

def parse_arguments():
    parser = argparse.ArgumentParser()
    parser.add_argument("--input", nargs='?', type=str, default=None,
                        help="input wav file")
    parser.add_argument("--stdo", nargs='?', type=str, default=None,
                        help="stdo from pipeline run if already generated")
    args = parser.parse_args()
    return args

def run_compute_plot(stdo, name, show_plot=False):
    xcore_stdo = []
    #ignore lines that don't contain [DEVICE]. Remove everything till and including [DEVICE] if [DEVICE] is present
    for line in stdo:
        m = re.search(r'^\s*\[DEVICE\]', line)
        if m is not None:
            xcore_stdo.append(re.sub(r'\[DEVICE\]\s*', '', line))

    #print(xcore_stdo)
    vad_output = np.empty(0, dtype=np.float64)
    vnr_output_pred = np.empty(0, dtype=np.float64)
    vnr_input_pred = np.empty(0, dtype=np.float64)
    vad_flag = np.empty(0, dtype=np.float64)
    vnr_pred_flag = np.empty(0, dtype=np.float64)
    vnr_agc_gain = np.empty(0, dtype=np.float64) 
    vad_agc_gain = np.empty(0, dtype=np.float64) 
    vnr_agc_debug_state = np.empty(0, dtype=np.float64) 
    vad_agc_debug_state = np.empty(0, dtype=np.float64) 
    
    for line in xcore_stdo:
        if "VAD:" in line:
            vad = float(line.split(" ")[-1])/256.0
            vad_output = np.append(vad_output, vad)
        elif "VNR OUTPUT PRED:" in line:
            vnr_mant = float(line.split(" ")[-2])
            vnr_exp = float(line.split(" ")[-1])
            vnr = vnr_mant * (2.0 ** vnr_exp)
            vnr_output_pred = np.append(vnr_output_pred, vnr)
        elif "VNR INPUT PRED:" in line:
            vnr_mant = float(line.split(" ")[-2])
            vnr_exp = float(line.split(" ")[-1])
            vnr = vnr_mant * (2.0 ** vnr_exp)
            vnr_input_pred = np.append(vnr_input_pred, vnr)
        elif "VAD FLAG:" in line:
            v = float(line.split(" ")[-1])
            vad_flag = np.append(vad_flag, v)
        elif "VNR PRED FLAG:" in line:
            v = float(line.split(" ")[-1])
            vnr_pred_flag = np.append(vnr_pred_flag, v)
        elif "VNR GAIN:" in line:
            v = float(line.split(" ")[-1])
            vnr_agc_gain = np.append(vnr_agc_gain, v)
        elif "VAD GAIN:" in line:
            v = float(line.split(" ")[-1])
            vad_agc_gain = np.append(vad_agc_gain, v)
        elif "VNR DEBUG STATE:" in line:
            v = float(line.split(" ")[-1])
            vnr_agc_debug_state = np.append(vnr_agc_debug_state, v)
        elif "VAD DEBUG STATE:" in line:
            v = float(line.split(" ")[-1])
            vad_agc_debug_state = np.append(vad_agc_debug_state, v)
    
    time = np.arange(0,len(vnr_output_pred))*0.015
    kwd_instances = np.arange(23/0.015, len(vnr_output_pred), 5/0.015)*0.015 #kwd instances starting from first kwd at 3sec and then keywords every 5 seconds 
    fig,ax = plt.subplots(4, sharex=True)
    fig.set_size_inches(20,10)
    ax[0].plot(time, vad_output, label="vad output")
    ax[0].plot(time, vnr_output_pred, label="vnr output pred")
    ax[0].plot(time, vnr_input_pred, label="vnr input pred")
    ax[0].vlines((kwd_instances), 0, 1, linestyle="dashed", colors="r", label="kwd instances")
    ax[0].legend(loc="upper right")
    ax[0].set_xlabel("time (seconds)")

    ax[1].plot(time, vad_flag, label="vad flag")
    ax[1].plot(time, vnr_pred_flag, label="vnr pred flag")
    ax[1].set_xlabel("time (seconds)")
    ax[1].vlines((kwd_instances), 0, 1, linestyle="dashed", colors="r", label="kwd instances")
    ax[1].legend(loc="upper right")

    ax[2].plot(time, vad_agc_gain, label="agc gain vad")
    ax[2].plot(time, vnr_agc_gain, label="agc gain vnr")
    ax[2].set_xlabel("time (seconds)")
    ax[2].legend(loc="upper right")
    #ax[2].vlines((kwd_instances), 0, 1, linestyle="dashed", colors="r", label="kwd instances")

    ax[3].plot(time, vad_agc_debug_state, label="vad_agc_debug_state")
    ax[3].plot(time, vnr_agc_debug_state, label="vnr_agc_debug_state")
    ax[3].set_xlabel("time (seconds)")
    ax[3].legend(loc="upper right")
    fig_instance = plt.gcf()
    if show_plot:
        plt.show()
    fig.savefig(f"plot_vad_vnr_{name}.png")



def run_test(input_file, exe, compute_plot=False, show_plot=False):
    stdo = run_xcoreai.run(exe, input_file, return_stdout=True)
    subprocess.run(f"sox output.wav -b 16 output_for_kwd.wav remix 1".split())
    kwd_detections = run_sensory.run_sensory(os.path.abspath("output_for_kwd.wav"), old_model=True)
    print(f"kwd_detections = {kwd_detections}")
    with open(f"pipeline_stdo_{os.path.basename(input_file)}.txt", "w") as fp:
        for l in stdo:
            fp.write(l)

    if compute_plot:
        run_compute_plot(stdo, os.path.basename(input_file), show_plot)

    with open(results_log_file, "a") as log:
        fcntl.flock(log, fcntl.LOCK_EX)
        log.write(f"{os.path.basename(input_file)},{os.path.basename(exe)},{kwd_detections},\n") 
        fcntl.flock(log, fcntl.LOCK_UN)

    return kwd_detections


@pytest.mark.parametrize("xe", executables) 
@pytest.mark.parametrize("input_stream", input_wav_files)
def test(input_stream, xe, record_property):
    kwd_detections = run_test(input_stream, xe)
    record_property("Testcase", os.path.basename(input_stream))
    record_property("Wakewords", kwd_detections)
    


if __name__ == "__main__":
    args = parse_arguments()
    if args.stdo != None:
        with open(args.stdo, "r") as fp:
            stdo = fp.readlines()
            run_compute_plot(stdo, "test_plot", show_plot=True)
    elif args.input != None:
        run_test(args.input, "../../../build/examples/bare-metal/pipeline_multi_threaded/bin/avona_example_bare_metal_pipeline_multi_thread.xe", compute_plot=True, show_plot=True)
        #run_test(args.input, "../../../build/examples/bare-metal/pipeline_alt_arch/bin/avona_example_bare_metal_pipeline_alt_arch_mt.xe", compute_plot=True, show_plot=True)
    else:
        assert(False), "Provide either --input or --stdo"
