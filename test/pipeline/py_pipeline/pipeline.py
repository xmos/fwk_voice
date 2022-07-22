import sys
import os

thisfile_path = os.path.dirname(os.path.realpath(__file__))
sys.path.append(os.path.join(thisfile_path, "../../../../lib_agc/python"))
import agc
sys.path.append(os.path.join(thisfile_path, "../../../../lib_voice_toolbox/python"))
from vtb_frame import VTBInfo, VTBFrame
from aec import aec

class pipeline(object):

    def __init__(self, rate, verbose, x_channel_count, y_channel_count, frame_advance,
                 mic_shift, mic_saturate, alt_arch, aec_conf, agc_init_config):
        self.aec = aec(**aec_conf)
        self.agc = agc.agc(**agc_init_config)
        
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
        return

    def process_frame(self, new_frame, verbose=False):
        new_y = new_frame[:self.y_channel_count]
        new_x = new_frame[self.y_channel_count:self.y_channel_count + self.x_channel_count]

        # AEC
        error, Error, Error_shad = self.aec.add(new_x, new_y)
        #Do the adaption
        self.aec.adapt(Error, True, verbose)
        self.aec.adapt_shadow(Error_shad, True, verbose)

        # AGC
        output = self.agc.process_frame(error, 0 , 0, 0)
        return output
        
        

