import sys
import os

thisfile_path = os.path.dirname(os.path.realpath(__file__))
sys.path.append(os.path.join(thisfile_path, "../../../../lib_agc/python"))
import agc
sys.path.append(os.path.join(thisfile_path, "../../../../lib_noise_suppression/python"))
import sup_sequential
sys.path.append(os.path.join(thisfile_path, "../../../../lib_voice_toolbox/python"))
from vtb_frame import VTBInfo, VTBFrame
from aec import aec
import IC
import numpy as np

class pipeline(object):

    def __init__(self, rate, verbose, disable_aec, disable_ic, disable_ns, disable_agc, x_channel_count, y_channel_count, frame_advance,
                 mic_shift, mic_saturate, alt_arch, aec_conf, agc_init_config, ic_conf, suppression_conf):

        # Fix path to VNR model
        self.aec = aec(**aec_conf)
        self.agc = agc.agc(**agc_init_config)
        ic_conf['vnr_model'] = os.path.join(thisfile_path, "config", ic_conf['vnr_model'])
        self.ifc = IC.adaptive_interference_canceller(**ic_conf) 
        self.ns = sup_sequential.sup_sequential(**suppression_conf)
        
        self.x_channel_count = x_channel_count
        self.y_channel_count = y_channel_count
        self.frame_advance = frame_advance

        # Store pipeline data before/during/after processing
        self.input_frame_before_shift = None
        self.input_frame = None
        self.post_stage_a_frame = None
        self.post_stage_b_frame = None
        self.vad_result = None
        self.output_frame = None

        self.frame_count = 0
        self.time_elapsed = 0
        self.verbose = verbose
        self.vnr_pred_log = np.empty(0, dtype=np.float64)
        self.mu_log = np.empty(0, dtype=np.float64)
        self.agc_vnr_threshold = 0.5
        self.disable_aec = disable_aec
        self.disable_ic = disable_ic 
        self.disable_ns = disable_ns
        self.disable_agc = disable_agc 
        return

    def process_frame(self, new_frame, verbose=False, vnr_input_override=None, mu_override=None):
        new_y = new_frame[:self.y_channel_count]
        new_x = new_frame[self.y_channel_count:self.y_channel_count + self.x_channel_count]

        # AEC
        if self.disable_aec:
            error_aec = new_y
        else:
            error_aec, Error, Error_shad = self.aec.add(new_x, new_y)
            #Do the adaption
            self.aec.adapt(Error, True, verbose)
            self.aec.adapt_shadow(Error_shad, True, verbose)

        # IC
        if self.disable_ic:
            error_ic_2ch = error_aec
            self.agc.ch_state[0].adapt = 0 # Since IC is disabled, run AGC with fixed gain since we'll not be able to provide VNR input
        else:
            error_ic, Error = self.ifc.process_frame(error_aec)
            error_ic_2ch = np.vstack((error_ic, error_ic)) # Duplicate across the 2nd channel
            if self.ifc.vnr_obj != None:
                py_vnr_in, py_vnr_out = self.ifc.calc_vnr_pred(Error)            
                if vnr_input_override != None:
                    self.ifc.input_vnr_pred = vnr_input_override
                    py_vnr_in = vnr_input_override
                self.vnr_pred_log = np.append(self.vnr_pred_log, self.ifc.input_vnr_pred)

            mu, control_flag = self.ifc.mu_control_system()
            if mu_override != None:
                self.ifc.mu = mu_override
                mu = mu_override
            self.mu_log = np.append(self.mu_log, mu)

            self.ifc.adapt(Error, mu)

        # NS
        if self.disable_ns:
            ns_output = error_ic_2ch
        else:
            x = np.zeros((0,240))
            ns_output = self.ns.process_frame(x, error_ic_2ch)

        # AGC
        if self.disable_agc:
            self.agc.ch_state[0].adapt = 0 # When AGC disabled, run it with fixed gain
        vnr_flag = (py_vnr_out > self.agc_vnr_threshold)
        pipeline_output = self.agc.process_frame(ns_output, 0 , vnr_flag, 0)

        return pipeline_output
        
        

