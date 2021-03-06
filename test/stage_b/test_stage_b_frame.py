# Copyright 2022 XMOS LIMITED.
# This Software is subject to the terms of the XMOS Public Licence: Version 1.

import numpy as np
import scipy.io.wavfile
import audio_wav_utils as awu
import ctypes
import pytest
import sys, os
import subprocess

from build import ic_vad_test_py
from ic_vad_test_py import ffi
import ic_vad_test_py.lib as ic_vad_test_lib

package_dir = os.path.dirname(os.path.abspath(__file__))
att_path = os.path.join(package_dir,'../../audio_test_tools/python/')
py_ic_path = os.path.join(package_dir,'../../../lib_interference_canceller/python')
py_stage_b_path = os.path.join(package_dir,'../../../lib_audio_pipelines/python')
py_vad_path = os.path.join(package_dir,'../../../lib_vad/python')
pvc_path = os.path.join(package_dir, '../shared/python')
hydra_audio_base_dir = os.path.expanduser("~/hydra_audio/")

sys.path.append(att_path)
sys.path.append(py_stage_b_path)
sys.path.append(py_ic_path)
sys.path.append(py_vad_path)
sys.path.append(pvc_path)
from ap_stage_b import ap_stage_b
import py_vs_c_utils as pvc 

ap_config_file = "../shared/config/two_mic_stereo_prev_arch.json"
input_file = "input.wav"
output_file = "output.wav"

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

    return ap_conf['ap_stage_b_conf']


class stage_b_comparison:
    def __init__(self, stage_b_conf):

        self.frame_advance = stage_b_conf["ic_conf"]["frame_advance"]
        self.proc_frame_length = stage_b_conf["ic_conf"]["proc_frame_length"]
        self.num_phases = stage_b_conf["ic_conf"]["phases"]
        self.passthrough_channel_count = stage_b_conf["ic_conf"]["passthrough_channel_count"]

        #override to match C version
        stage_b_conf["ic_conf"]["use_noise_minimisation"] = False
        print(stage_b_conf)

        self.sb = ap_stage_b(self.frame_advance, stage_b_conf["ic_conf"], self.passthrough_channel_count, mic_shift=0, mic_saturate=0)

        ic_vad_test_lib.test_init() 
        
        #Logging
        self.ic_state = None
        self.py_vad = None
        self.c_vad = None


    def process_frame(self, frame):
        #we need to delay the y for python as not done in model
        #first copy the input data for C ver before we modfiy it
        frame_int = pvc.float_to_int32(frame)
 
        class stage_a_md:
            vad = 0

        #Run a frame through python  
        output_py, metadata = self.sb.process_frame(frame, stage_a_md)
        py_vad = metadata.vad_result
        self.py_vad = py_vad

        #Grab a pointer to the data storage of the numpy arrays
        y_data = ffi.cast("int32_t *", ffi.from_buffer(frame_int[0].data))
        x_data = ffi.cast("int32_t *", ffi.from_buffer(frame_int[1].data))
        output_c = np.zeros((240), dtype=np.int32)
        output_c_ptr = ffi.cast("int32_t *", ffi.from_buffer(output_c.data))
        ic_vad_test_lib.test_filter(y_data, x_data, output_c_ptr)
        c_vad = pvc.uint8_to_float(ic_vad_test_lib.test_vad(output_c_ptr))
        self.c_vad = c_vad.copy()
        print(f"1py_vad: {py_vad:.4f}, c_vad: {c_vad:.4f}")

        #note we override c_vad to match py_vad for comparison
        c_vad = pvc.float_to_uint8(np.array(py_vad))
        print(f"2py_vad: {py_vad:.4f}, c_vad: {c_vad:.4f}")
        ic_vad_test_lib.test_adapt(c_vad, output_c_ptr)

        ic_state = ic_vad_test_lib.test_get_ic_state()
        self.ic_state = ic_state
        print(f"py_mu: {self.sb.ifc.mu}, c_mu: {pvc.float_s32_to_float(ic_state.mu[0][0])}")
        # print(pvc.float_s32_to_float(state.config_params.delta))

        cies = pvc.float_s32_to_float(ic_state.ic_adaption_controller_state.input_energy_slow)
        coes = pvc.float_s32_to_float(ic_state.ic_adaption_controller_state.output_energy_slow)
        cief = pvc.float_s32_to_float(ic_state.ic_adaption_controller_state.input_energy_fast)
        coef = pvc.float_s32_to_float(ic_state.ic_adaption_controller_state.output_energy_fast)
        print(f"c - ies: {cies} oes: {coes} ief: {cief} oef: {coef}")
        print(f"p - ies: {self.sb.input_energy} oes: {self.sb.output_energy} ief: {self.sb.input_energy0} oef: {self.sb.output_energy0}")
        return output_py[0], pvc.int32_to_float(output_c)


def test_frame_compare(test_config):
    sbc = stage_b_comparison(test_config)

    frame_advance = test_config["ic_conf"]["frame_advance"]
    proc_frame_length = test_config["ic_conf"]["proc_frame_length"]

    input_rate, input_wav_file = scipy.io.wavfile.read(input_file, 'r')
    input_wav_data, input_channel_count, file_length = awu.parse_audio(input_wav_file)
    delays = np.zeros(input_channel_count) #we do delay of y channel in process_frame above and in C rather than awu.get_frame

    output_wav_data = np.zeros((2, file_length))
    mu_log = np.zeros((file_length//frame_advance, 2))
    vad_log = np.zeros((file_length//frame_advance, 2))

    for frame_start in range(0, file_length-proc_frame_length*2, frame_advance):
        input_frame = awu.get_frame(input_wav_data, frame_start, frame_advance, delays)[0:2,:]

        if False:
            print ('# ' + str(frame_start // frame_advance))

        output_py, output_c = sbc.process_frame(input_frame)
        mu_log[frame_start//frame_advance, :] = np.array([sbc.sb.ifc.mu, pvc.float_s32_to_float(sbc.ic_state.mu[0][0])])
        vad_log[frame_start//frame_advance, :] = np.array([sbc.py_vad, sbc.c_vad])

        output_wav_data[0, frame_start: frame_start + frame_advance] = output_py
        output_wav_data[1, frame_start: frame_start + frame_advance] = output_c
        
    #Write a copy of the output for post analysis if needed
    scipy.io.wavfile.write(output_file, input_rate, pvc.float_to_int32(output_wav_data.T))

    pvc.basic_line_graph("py_vs_c_mu", mu_log)
    pvc.basic_line_graph("py_vs_c_vad", vad_log)


    arith_closeness, geo_closeness, c_delay, peak2ave = pvc.pcm_closeness_metric(output_file)

    assert c_delay == 0
    assert geo_closeness > 0.98
    assert arith_closeness > 0.88 #Still very close over a 30s piece of audio with multiple blocks (adaption controller & IC)

#Check equivalence of adaption controller
def test_adaption_controller(test_config):
    stage_b_conf = test_config
    #instantiate and init a stage B instance
    sb = ap_stage_b(stage_b_conf["ic_conf"]["frame_advance"], stage_b_conf["ic_conf"],stage_b_conf["ic_conf"]["passthrough_channel_count"], mic_shift=0, mic_saturate=0)
    #Init the avona instance
    ic_vad_test_lib.test_init()

    #A few fixed sceanrios
    vad_vects = [0, 0.1, 0.2, 0.1, 0.99, 1.0]
    in_energy_vects_slow = [0.1, 0.2, 0.3, 0.4, 0.2, 0.2]
    out_energy_vects_slow = [0.1, 0.15, 0.2, 0.3, 0.1, 0.1]
    in_energy_vects_fast = [0.1, 0.2, 0.3, 0.4, 0.2, 0.2]
    out_energy_vects_fast = [0.1, 0.3, 0.2, 0.1, 0.1, 0.1]

    #A lot of random scenarios
    for i in range(1000):
        vad_vects.append(np.random.random(1)[0])
        in_energy_vects_slow.append(np.random.random(1)[0])
        out_energy_vects_slow.append(np.random.random(1)[0])
        in_energy_vects_fast.append(np.random.random(1)[0])
        out_energy_vects_fast.append(np.random.random(1)[0])

    for vad, in_s, out_s, in_f, out_f in zip(vad_vects, in_energy_vects_slow, out_energy_vects_slow,
                                            in_energy_vects_fast, out_energy_vects_fast):
        sb.input_energy = in_s 
        sb.output_energy = out_s 
        sb.input_energy0 = in_f 
        sb.output_energy0 = out_f 

        sb.adaption_controller(vad)
        py_mu = sb.ifc.mu
        py_svc = sb.smoothed_voice_chance

        c_vad = pvc.float_to_uint8(vad)
        ic_vad_test_lib.test_set_ic_energies(in_s, out_s, in_f, out_f)
        ic_vad_test_lib.test_adaption_controller(c_vad)
        ic_state = ic_vad_test_lib.test_get_ic_state()
        c_mu = pvc.float_s32_to_float(ic_state.mu[0][0])
        c_svc = pvc.float_s32_to_float(ic_state.ic_adaption_controller_state.smoothed_voice_chance)

        # print(f"py_mu:{py_mu}, c_mu:{c_mu}")
        # print(f"py_svc:{py_svc}, c_svc:{c_svc}")

        rtol=1-(1/256)#Because we quantise to 8b for VAD input
        atol=1/256

        assert np.isclose(py_mu, c_mu, rtol=rtol, atol=atol)
        assert np.isclose(py_svc, c_svc, rtol=rtol, atol=atol)





