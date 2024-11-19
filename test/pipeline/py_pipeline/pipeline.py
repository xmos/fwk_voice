import numpy as np
from py_voice.modules import aec, agc, ic, ns

class pipeline(object):

    def __init__(self, disable_aec, disable_ic, disable_ns, disable_agc, ap_conf):

        # Fix path to VNR model
        self.aec_obj = aec.aec(ap_conf)
        self.agc_obj = agc.agc(ap_conf)
        self.ic_obj = ic.ic(ap_conf)
        self.ns_obj = ns.ns(ap_conf)
        
        self.x_channel_count = ap_conf["aec"]["x_channel_count"]
        self.y_channel_count = ap_conf["aec"]["y_channel_count"]

        # Store pipeline data before/during/after processing
        self.pp_metadata = {}

        self.vnr_pred_log = np.empty(0, dtype=np.float64)
        self.mu_log = np.empty(0, dtype=np.float64)
        self.disable_aec = disable_aec
        self.disable_ic = disable_ic 
        self.disable_ns = disable_ns
        self.disable_agc = disable_agc 
        return

    def process_frame(self, new_frame, vnr_input_override=None, mu_override=None):

        # AEC
        error_aec, self.pp_metadata = self.aec_obj.process_frame(new_frame, self.pp_metadata, self.disable_aec)

        # IC
        # doing it manually because we want to be able to overwrite vnr prediction and mu
        # also, using output_vnr prediction to feed to the agc, not the input one like in py_voice
        if self.disable_ic:
            error_ic_2ch = error_aec
            self.agc_obj.adapt = 0 # Since IC is disabled, run AGC with fixed gain since we'll not be able to provide VNR input
        else:
            error_ic, self.pp_metadata, Error = self.ic_obj.add(error_aec, self.pp_metadata, self.disable_ic)
            error_ic_2ch = np.vstack((error_ic, error_ic)) # Duplicate across the 2nd channel
            if self.ic_obj.vnr_obj != None:
                py_vnr_in, py_vnr_out = self.ic_obj.calc_vnr_pred(Error)
                #self.pp_metadata["ic"]["vnr_pred"] = py_vnr_out
                self.pp_metadata = self.ic_obj.write_metadata("ic", self.pp_metadata, {"vnr_pred" : py_vnr_out})
                if vnr_input_override != None:
                    self.ic_obj.input_vnr_pred = vnr_input_override
                self.vnr_pred_log = np.append(self.vnr_pred_log, self.ic_obj.input_vnr_pred)

            mu, control_flag = self.ic_obj.mu_control_system()
            if mu_override != None:
                self.ic_obj.mu = mu_override
                mu = mu_override
            self.mu_log = np.append(self.mu_log, mu)

            self.ic_obj.main_filter.adapt(self.ic_obj, Error, True)

        # NS
        if self.disable_ns:
            ns_output = error_ic_2ch
        else:
            ns_output, self.pp_metadata = self.ns_obj.process_frame(error_ic_2ch, self.pp_metadata)

        # AGC
        if self.disable_agc:
            self.agc_obj.adapt = 0 # When AGC disabled, run it with fixed gain
        pipeline_output, self.pp_metadata = self.agc_obj.process_frame(ns_output, self.pp_metadata)

        return pipeline_output
        
        

