# Copyright 2022 XMOS LIMITED.
# This Software is subject to the terms of the XMOS Public Licence: Version 1.

import numpy as np
import scipy.io.wavfile
import audio_wav_utils as awu
import ctypes
import pytest
import sys, os
import subprocess

from build import ic_vnr_test_py
from ic_vnr_test_py import ffi
import ic_vnr_test_py.lib as ic_vnr_test_lib

package_dir = os.path.dirname(os.path.abspath(__file__))
pvc_path = os.path.join(package_dir, '../shared/python')
hydra_audio_base_dir = os.path.expanduser("~/hydra_audio/")

sys.path.append(pvc_path)
import IC
import py_vs_c_utils as pvc 

ap_config_file = "../shared/config/ic_conf_no_adapt_control.json"
tflite_model = os.path.join(package_dir, "../../modules/lib_vnr/python/model/model_output/model_qaware.tflite")
input_file = "input.wav"
output_file = "output.wav"

frames_print = 15

@pytest.fixture(params=[ap_config_file])
def test_config(request):
    ap_conf = pvc.json_to_dict(request.param)
    global hydra_audio_base_dir
    try:
        hydra_audio_base_dir = os.environ['hydra_audio_PATH']
    except:
        print(f'Warning: hydra_audio_PATH environment variable not set. Using local path {hydra_audio_base_dir}')
    hydra_audio_path = os.path.join(hydra_audio_base_dir, "xvf3510_no_processing_xmos_test_suite")
    test_track = os.path.join(hydra_audio_path, "InHouse_XVF3510v080_v1.2_20190423_Loc1_Noise2_60dB__Take1.wav")
    cmd = f"sox {test_track} -c 2 {input_file} trim 120 30"
    subprocess.run(cmd.split())

    return ap_conf


class stage_b_comparison:
    def __init__(self, ic_conf):

        self.frame_advance = ic_conf["frame_advance"]
        self.proc_frame_length = ic_conf["proc_frame_length"]
        self.num_phases = ic_conf["phases"]

        self.ic = IC.adaptive_interference_canceller(**ic_conf)
        #self.vnr = vnr.Vnr(model_file=tflite_model)

        self.x_data = np.zeros(self.proc_frame_length, dtype=np.float64)

        ic_vnr_test_lib.test_init() 
        
        #Logging
        self.ic_state = None
        self.py_vnr = None
        self.c_vnr = None

    def process_frame(self, frame, index):
        #we need to delay the y for python as not done in model
        #first copy the input data for C ver before we modfiy it
        frame_int = pvc.float_to_int32(frame)

        #Run a frame through python  
        output_py, Error = self.ic.process_frame(frame)

        py_vnr_in, py_vnr_out = self.ic.calc_vnr_pred(Error)
        py_vnr = py_vnr_in
        mu, control_flag = self.ic.mu_control_system()
        self.ic.adapt(Error, mu)

        #Grab a pointer to the data storage of the numpy arrays
        y_data = ffi.cast("int32_t *", ffi.from_buffer(frame_int[0].data))
        x_data = ffi.cast("int32_t *", ffi.from_buffer(frame_int[1].data))
        output_c = np.zeros((240), dtype=np.int32)
        output_c_ptr = ffi.cast("int32_t *", ffi.from_buffer(output_c.data))
        ic_vnr_test_lib.test_filter(y_data, x_data, output_c_ptr)
        c_vnr = ic_vnr_test_lib.test_vnr()
        if (index < frames_print) and False:
            print(f"py_vnr: {py_vnr}, c_vnr: {pvc.float_s32_to_float(c_vnr)}")

        #note we override c_vnr to match py_vnr for comparison
        #c_vnr = [int(0), int(0)] # dummy
        if (index < frames_print) and False:
            print(f"py_vnr: {py_vnr}, c_vnr: {pvc.float_s32_to_float(c_vnr)}")
        ic_vnr_test_lib.test_adapt(c_vnr, output_c_ptr)

        ic_state = ic_vnr_test_lib.test_get_ic_state()
        self.ic_state = ic_state
        if (index < frames_print) and False:
            print(f"py_input_en {self.ic.input_energy0} c_input_en {pvc.float_s32_to_float(ic_state.ic_adaption_controller_state.input_energy)}")
            print(f"py_output_en {self.ic.output_energy0} c_output_en {pvc.float_s32_to_float(ic_state.ic_adaption_controller_state.output_energy)}")
        if (index < frames_print) and False:
            print(f"py_fast_ratio {self.ic.fast_ratio} c_fast_ratio {pvc.float_s32_to_float(ic_state.ic_adaption_controller_state.fast_ratio)}")
        if (index < frames_print) and False:
            print(f"py_mu: {self.ic.mu}, c_mu: {pvc.float_s32_to_float(ic_state.mu[0][0])}")

        if (index < frames_print) and False:
            print('-')
        return output_py, pvc.int32_to_float(output_c)

def test_frame_compare(test_config):

    test_config["adaption_config"] = 'IC_ADAPTION_AUTO'
    test_config["vnr_model"] = "../../modules/lib_vnr/python/model/model_output/model_qaware.tflite"
    sbc = stage_b_comparison(test_config)

    frame_advance = test_config["frame_advance"]
    proc_frame_length = test_config["proc_frame_length"]

    input_rate, input_wav_file = scipy.io.wavfile.read(input_file, 'r')
    input_wav_data, input_channel_count, file_length = awu.parse_audio(input_wav_file)

    output_wav_data = np.zeros((2, file_length))
    mu_log = np.zeros((file_length//frame_advance, 2))
    vnr_log = np.zeros((file_length//frame_advance, 2))

    #for frame_start in range(0, frame_advance * 15, frame_advance):
    for frame_start in range(0, file_length-proc_frame_length*2, frame_advance):
        input_frame = awu.get_frame(input_wav_data, frame_start, frame_advance)[0:2,:]

        if False:
            print ('# ' + str(frame_start // frame_advance))

        output_py, output_c = sbc.process_frame(input_frame, frame_start // frame_advance)
        mu_log[frame_start//frame_advance, :] = np.array([sbc.ic.mu, pvc.float_s32_to_float(sbc.ic_state.mu[0][0])])
        vnr_log[frame_start//frame_advance, :] = np.array([sbc.py_vnr, sbc.c_vnr])

        output_wav_data[0, frame_start: frame_start + frame_advance] = output_py
        output_wav_data[1, frame_start: frame_start + frame_advance] = output_c
        
    #Write a copy of the output for post analysis if needed
    scipy.io.wavfile.write(output_file, input_rate, pvc.float_to_int32(output_wav_data.T))

    pvc.basic_line_graph("py_vs_c_mu", mu_log)
    pvc.basic_line_graph("py_vs_c_vnr", vnr_log)


    arith_closeness, geo_closeness, c_delay, peak2ave = pvc.pcm_closeness_metric(output_file)

    assert c_delay == 0
    assert geo_closeness > 0.98
    assert arith_closeness > 0.87 #Still very close over a 30s piece of audio with multiple blocks (adaption controller & IC)

""" #Check equivalence of adaption controller
def test_adaption_controller(test_config):
    stage_b_conf = test_config
    #instantiate and init a stage B instance
    #sb = ap_stage_b(stage_b_conf["ic_conf"]["frame_advance"], stage_b_conf["ic_conf"],stage_b_conf["ic_conf"]["passthrough_channel_count"], mic_shift=0, mic_saturate=0)
    sbc = stage_b_comparison(test_config)
    #Init the avona instance
    ic_vnr_test_lib.test_init()

    #A few fixed sceanrios
    vnr_vects = [0, 0.1, 0.2, 0.1, 0.99, 1.0]
    in_energy_vects_slow = [0.1, 0.2, 0.3, 0.4, 0.2, 0.2]
    out_energy_vects_slow = [0.1, 0.15, 0.2, 0.3, 0.1, 0.1]
    in_energy_vects_fast = [0.1, 0.2, 0.3, 0.4, 0.2, 0.2]
    out_energy_vects_fast = [0.1, 0.3, 0.2, 0.1, 0.1, 0.1]

    #A lot of random scenarios
    for i in range(1000):
        vnr_vects.append(np.random.random(1)[0])
        in_energy_vects_slow.append(np.random.random(1)[0])
        out_energy_vects_slow.append(np.random.random(1)[0])
        in_energy_vects_fast.append(np.random.random(1)[0])
        out_energy_vects_fast.append(np.random.random(1)[0])

    for vnr, in_s, out_s, in_f, out_f in zip(vnr_vects, in_energy_vects_slow, out_energy_vects_slow,
                                            in_energy_vects_fast, out_energy_vects_fast):
        sbc.input_energy = in_s 
        sbc.output_energy = out_s 
        sbc.input_energy0 = in_f 
        sbc.output_energy0 = out_f 

        sbc.adaption_controller(vnr, 1000)
        py_mu = sbc.ic.mu
        py_svc = sbc.smoothed_voice_chance

        c_vnr = pvc.float_to_float_s32(vnr)
        ic_vnr_test_lib.test_set_ic_energies(in_s, out_s, in_f, out_f)
        ic_vnr_test_lib.test_adaption_controller(c_vnr)
        ic_state = ic_vnr_test_lib.test_get_ic_state()
        c_mu = pvc.float_s32_to_float(ic_state.mu[0][0])
        c_svc = pvc.float_s32_to_float(ic_state.ic_adaption_controller_state.smoothed_voice_chance)

        # print(f"py_mu:{py_mu}, c_mu:{c_mu}")
        # print(f"py_svc:{py_svc}, c_svc:{c_svc}")

        rtol=1-(1/256)#Because we quantise to 8b for VAD input
        atol=1/256

        assert np.isclose(py_mu, c_mu, rtol=rtol, atol=atol)
        assert np.isclose(py_svc, c_svc, rtol=rtol, atol=atol) """
