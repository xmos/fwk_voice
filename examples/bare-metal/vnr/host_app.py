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

        if self.output_offset < input_file_length:
            #print('[HOST]Record callback has been triggered value = {}'.format(dataval))
            self.output_data_array = np.append(self.output_data_array, dataval)
            self.output_offset += 1
            #print("[HOST]Values recieved: ", self.output_offset, " Values expected: ", input_file_length)

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

        padded_array = np.pad(array, (0, FRAME_LENGTH - len(array) % FRAME_LENGTH), 'constant')
        self.num_frames = int(len(padded_array)/FRAME_LENGTH)
        print("total frames = ",self.num_frames)
        for i in range(0, len(padded_array), FRAME_LENGTH):
            #print(padded_array[i : i + FRAME_LENGTH])
            self.publish_frame(padded_array[i : i + FRAME_LENGTH].tobytes())

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

    ep.output_data_array.tofile(output_file)
    print('[HOST]END!')
    return ep.output_data_array

def run_with_xscope_fileio(input_file, output_file):
    run_xcoreai.run("../../../build/examples/bare-metal/vnr/bin/avona_example_bare_metal_vnr_fileio.xe", input_file)    
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

