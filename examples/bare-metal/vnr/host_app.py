# Copyright 2022 XMOS LIMITED.
# This Software is subject to the terms of the XMOS Public Licence: Version 1.

###### help
#### Running with python xscope host (Doesn't work on M1 Mac)
# python host_app.py test_stream_1.wav vnr_out1.bin

#### Running with lib xscope_filio implemented xscope host
# python host_app.py test_stream_1.wav vnr_out2.bin --run-with-xscope-fileio

### To see the plot
# python host_app.py test_stream_1.wav vnr_out1.bin --show-plot
# python host_app.py test_stream_1.wav vnr_out2.bin --run-with-xscope-fileio --show-plot
# python host_app.py test_stream_1.wav vnr_out2.bin --run-with-xscope-fileio --show-plot --run-x86 (x86 only supported in the xscope_fileio version)

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
import socket
import subprocess
import signal
from profile import parse_profiling_info, parse_profile_log

# How long in seconds we would expect xrun to open a port for the host app
# The firmware will have already been loaded so 5s is more than enough
# as long as the host CPU is not too busy. This can be quite long (10s+)
# for a busy CPU
XRUN_TIMEOUT = 20

FRAME_LENGTH = 240
CHUNK_SIZE = 256

def parse_arguments():
    parser = argparse.ArgumentParser()
    parser.add_argument("input_wav", nargs='?', help="input wav file")
    parser.add_argument("output_bin", nargs='?', help="vnr output bin file")
    parser.add_argument("--run-with-xscope-fileio", action='store_true', help="run with lib xscope_fileio instead of python xscope host")
    parser.add_argument("--show-plot", action='store_true', help="show the VNR output plot")
    parser.add_argument("--run-x86", action='store_true', help="Run x86 exe. Only use with --run-with-xscope-fileio")
    args = parser.parse_args()
    return args


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
        self.stdo = []

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
        print(f"[DEVICE] {msg}")
        self.stdo.append(f"[DEVICE] {msg}")
        

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
        for i in range(0, self.num_frames*FRAME_LENGTH, FRAME_LENGTH): 
            self.publish_frame(array[i : i + FRAME_LENGTH].tobytes())

def _get_open_port():
    s = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    s.bind(("localhost", 0))
    s.listen(1)
    port = s.getsockname()[1]
    s.close()
    return port

def _test_port_is_open(port):
    port_open = True
    s = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    try:
        s.bind(("localhost", port))
    except OSError:
        port_open = False
    s.close()
    return port_open

def start_xrun(firmware_xe):
    adapter_id = run_xcoreai.get_adapter_id()
    port = _get_open_port()
    xrun_cmd = (
        f"--xscope-port localhost:{port} --adapter-id {adapter_id} {firmware_xe}"
    )
    xrun_proc = subprocess.Popen(["xrun"]+xrun_cmd.split())
    print("Waiting for xrun", end="")
    timeout = time.time() + XRUN_TIMEOUT
    while _test_port_is_open(port):
        print(".", end="", flush=True)
        time.sleep(0.1)
        if time.time() > timeout:
            xrun_proc.kill()
            assert 0, f"xrun timed out - took more than {XRUN_TIMEOUT} seconds to start"
    return xrun_proc, port

def stop_xrun(xrun_proc):
    xrun_proc.send_signal(signal.SIGINT)
    time.sleep(2)
    xrun_proc.send_signal(signal.SIGINT)
    time.sleep(1)
    xrun_proc.send_signal(signal.SIGINT)
    time.sleep(2)
    xrun_proc.kill()
    time.sleep(1)

   
def plot_result(vnr_out, out_file, show_plot=False):
    #Plot VNR output
    mant = np.array(vnr_out[0::2], dtype=np.float64)
    exp = np.array(vnr_out[1::2], dtype=np.int32)
    vnr_out = mant * (float(2)**exp)
    plt.plot(vnr_out)
    plt.xlabel('frames')
    plt.ylabel('vnr estimate')
    fig_instance = plt.gcf()
    if show_plot:
        plt.show()
    plotfile = f"vnr_example_plot_{os.path.splitext(os.path.basename(out_file))[0]}.png"
    fig_instance.savefig(plotfile)


def run_with_python_xscope_host(input_file, output_file, parse_profile=False):
    # Get input data
    sr, input_data_array = wavfile.read(input_file)
    assert(sr == 16000), "[ERROR]Only 16kHz sample rate is supported"
    assert(len(np.shape(input_data_array)) == 1), "[ERROR]Only single channel inputs are supported in this application" 
    if input_data_array.dtype == np.int16:
        input_data_array = np.array(input_data_array << 16, dtype=np.int32)
    assert(input_data_array.dtype == np.int32), "[ERROR] Only 16bit or 32bit wav files supported"
    
    # Start device
    firmware_xe = "../../../build/examples/bare-metal/vnr/bin/fwk_voice_example_bare_metal_vnr.xe"
    xrun_proc, port = start_xrun(firmware_xe)
    
    # Start host app
    print("Starting host app...", end="\n")
    ep = Endpoint()
    if ep.connect(port=str(port)):
        print("[ERROR]Failed to connect the endpoint")
    else:
        print("[HOST]Endpoint connected")
        time.sleep(1)
        ep.publish_wav(input_data_array)
        print("output_offset = ",ep.output_offset)
        while not ep.is_done():
            pass

    done_msg = "DONE"
    time.sleep(1)
    ep.disconnect()
    ep.output_data_array.astype(np.int32).tofile(output_file)
    print('[HOST]END!')

    # Stop xrun process
    stop_xrun(xrun_proc)
    
    # Parse and log profiling info
    if parse_profile:
        src_folder = os.path.join(os.path.dirname(os.path.abspath(__file__)), 'src')
        parse_profiling_info(ep.stdo, src_folder)
    return ep.output_data_array


def run_with_xscope_fileio(input_file, output_file, run_x86, parse_profile=False):
    if run_x86:
        subprocess.run(["../../../build/examples/bare-metal/vnr/bin/fwk_voice_example_bare_metal_vnr_fileio", input_file, output_file], check=True)
    else:
        stdo = run_xcoreai.run("../../../build/examples/bare-metal/vnr/bin/fwk_voice_example_bare_metal_vnr_fileio.xe", input_file, return_stdout=True)
        if parse_profile:
            src_folder = os.path.join(os.path.dirname(os.path.abspath(__file__)), 'src/with_fileio')
            parse_profiling_info(stdo, src_folder)
        try:
            shutil.copy2("vnr_out.bin", output_file) # fwk_voice_example_bare_metal_vnr_fileio.xe writes vnr output in vnr_out.bin file
        except shutil.SameFileError as e:
            pass
        except IOError as e:
             print('Error: %s' % e.strerror)
    return np.fromfile(output_file, dtype=np.int32)

    
if __name__ == "__main__":
    args = parse_arguments()
    print(f"input_file: {args.input_wav}, output_file: {args.output_bin}, with_xscope_fileio={args.run_with_xscope_fileio}, show_plot={args.show_plot}, run_x86={args.run_x86}")
    if args.run_with_xscope_fileio:
        vnr_out = run_with_xscope_fileio(args.input_wav, args.output_bin, args.run_x86, parse_profile=True)
    else:
        assert(args.run_x86 == False), "x86 build not supported when using python xscope host"
        vnr_out = run_with_python_xscope_host(args.input_wav, args.output_bin, parse_profile=True)

    plot_result(vnr_out, args.input_wav, show_plot=args.show_plot)

