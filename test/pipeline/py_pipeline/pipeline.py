import sys
import os

thisfile_path = os.path.dirname(os.path.realpath(__file__))
sys.path.append(os.path.join(thisfile_path, "../../../../lib_agc/python"))
import agc
sys.path.append(os.path.join(thisfile_path, "../../../../lib_voice_toolbox/python"))
from vtb_frame import VTBInfo, VTBFrame
from aec import aec
import IC
import numpy as np

class pipeline(object):

    def __init__(self, rate, verbose, x_channel_count, y_channel_count, frame_advance,
                 mic_shift, mic_saturate, alt_arch, aec_conf, agc_init_config, ic_conf):

        # Fix path to VNR model
        self.aec = aec(**aec_conf)
        self.agc = agc.agc(**agc_init_config)
        ic_conf['vnr_model'] = os.path.join(thisfile_path, "config", ic_conf['vnr_model'])
        self.ifc = IC.adaptive_interference_canceller(**ic_conf) 
        
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
        return

    def process_frame(self, new_frame, verbose=False, vnr_input_override=None, mu_override=None):
        new_y = new_frame[:self.y_channel_count]
        new_x = new_frame[self.y_channel_count:self.y_channel_count + self.x_channel_count]

        # AEC
        error_aec, Error, Error_shad = self.aec.add(new_x, new_y)
        #Do the adaption
        self.aec.adapt(Error, True, verbose)
        self.aec.adapt_shadow(Error_shad, True, verbose)

        # IC
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

        # AGC
        output = self.agc.process_frame(error_ic_2ch, 0 , 0, 0)

        return output
        
        

