# Copyright 2022 XMOS LIMITED.
# This Software is subject to the terms of the XMOS Public Licence: Version 1.
# Convolve room impulse responses with WAV files
#
# Dependencies:
# - Matplotlib 2.2.4
# - Numpy 1.16.2
# - Scipy 1.2.1
# - SoundFile 0.10.2

import os
import numpy as np
import scipy.signal as spsig
import matplotlib.pyplot as plt
import soundfile as sf
from shutil import copyfile
import pandas


frame_advance = 240
frame_size = 512
input_file_fs = 48000
voice_sample_rate = 16000
frame_rate = float(voice_sample_rate) / frame_advance
aec_phases = 10

#Iterate through list of changes and shift the near end signals (channels 0..1) while keeping the
#reference fixed. Since they are relative it doesn't really matter which one you shift.
#If a positive number is supplied (second value in tuple which gets loaded into this_change_amount)
#then the far end (reference) is delayed by this amount.
#So a negative number will cause the far end to be shifted forwards, or in other words we delay the mics
def apply_delay_changes(audio_file_name_input, audio_file_name_output, far_end_delay_changes):
    print_stats = False
    audio_file_stats = sf.info(audio_file_name_input, verbose=True)

    num_frames = audio_file_stats.frames #note  frames is really samples here
    data, fs = sf.read(audio_file_name_input)

    near = data[:, 0:2]
    far = data[:, 2:4]

    new_near = np.copy(near)

    fs = 48000
    frame_advance = 240
    frame_size = 512

    ground_truth = np.zeros(num_frames)

    num_changes = len(far_end_delay_changes)
    if num_changes == 0:
      return

    for change_idx, far_end_delay_change in zip(list(range(num_changes)), far_end_delay_changes):
      this_change_base = int(far_end_delay_change[0])
      this_change_amount = int(far_end_delay_change[1])
      near_copy_from_base = this_change_base + this_change_amount

      if print_stats:
        print("Change number: ", change_idx)
        print("num_frames: ", num_frames)
        print("this_change_base: ", this_change_base)
        print("this_change_amount: ", this_change_amount)
        print("near_copy_from_base: ", near_copy_from_base)

      #For the last frame..
      if change_idx == num_changes - 1:
        next_change_base = num_frames - this_change_amount #handle case where not enough far end exists at end, we pad zeros see below..
        if print_stats:
          print("next_change_base: ", next_change_base)
        if next_change_base > num_frames: #handle case where we want too much far end
          next_change_base = num_frames
        else:
          new_near[next_change_base:num_frames, :] = np.zeros((num_frames - next_change_base, 2))
      else:
        next_change_base = far_end_delay_changes[change_idx + 1][0]

      if print_stats:
        print("next_change_base: ", next_change_base)

      #Handle the case where we want near end signal before t0 - just pad with zeros 
      if near_copy_from_base < 0:
        new_near[this_change_base:-near_copy_from_base, :] = np.zeros((-near_copy_from_base, 2))
        new_near[-near_copy_from_base:next_change_base, :] = near[0:next_change_base + this_change_amount, :]    
      else:
        new_near[this_change_base:next_change_base, :] = near[near_copy_from_base:next_change_base + this_change_amount, :]
      ground_truth[this_change_base:next_change_base] = this_change_amount

    data = np.concatenate((new_near, far), axis=1)
    sf.write(audio_file_name_output, data, fs, subtype='PCM_32')

    return ground_truth

#implements a variable 2D delay with second dimension 2
class delay_line:

  def __init__(self, max_delay_samples):
    self.delay_line = np.zeros((max_delay_samples, 2))
    self.max_delay_samples = max_delay_samples

  def push(self, push_input):
    size = push_input.shape[0]
    assert (len(push_input.shape) == 2),"Only 2D arrays can be delayed"
    self.delay_line[size:, :] = self.delay_line[:self.max_delay_samples - size, :]
    self.delay_line[:size, :] = push_input

  def read(self, size, delay_samples):
    assert (size + delay_samples <= self.max_delay_samples),"required delay and size exceed max delay size"
    return self.delay_line[delay_samples:delay_samples+size, :]


class ema_filter():
  def __init__(self, alpha, neg_clip = -50, pos_clip = 50, initial_val = 0):
    self.alpha = alpha
    self.neg_clip = neg_clip
    self.pos_clip = pos_clip
    self.last_val = initial_val

  def set(self, initial_val):
    self.last_val = initial_val

  def get(self):
    return self.last_val 

  def do(self, new_val):
    new_val = self.neg_clip if new_val < self.neg_clip else new_val
    new_val = self.pos_clip if new_val > self.pos_clip else new_val

    filtered = (new_val * self.alpha) + (self.last_val * (1 - self.alpha))
    self.last_val = filtered

    return filtered

  def get_filtered_list(self, input_list):
    filtered_list = []
    for val in input_list:
      filtered = self.do(val)
      filtered_list.append(filtered)
    return filtered_list

class moving_average_filter():
  def __init__(self, length, neg_clip = -5000, pos_clip = 5000):
    self.history = np.zeros(length)
    self.length = length
    self.idx = 0
    self.neg_clip = neg_clip
    self.pos_clip = pos_clip
    self.full = False

  def get(self):
    return self.history.mean()

  def push(self, new_val):
    new_val = self.neg_clip if new_val < self.neg_clip else new_val
    new_val = self.pos_clip if new_val > self.pos_clip else new_val

    self.history[self.idx] = new_val
    self.idx = (self.idx + 1) % self.length
    if self.idx == 0:
      self.full = True

  def get_filtered_list(self, input_list):
    filtered_list = []
    for val in input_list:
      filtered = self.do(val)
      filtered_list.append(filtered)
    return filtered_list


def extract_audio_and_gt_section(input_audio_file, output_file, frames_base, frames_length, ground_truth):
  data, fs = sf.read(input_audio_file)
  extract = data[frames_base:frames_base + frames_length, :]
  sf.write(output_file, extract, fs, subtype='PCM_32')

  #Ground truth 
  gt = np.array(ground_truth)
  ground_truth = gt[frames_base:frames_base + frames_length]
  return ground_truth


#This is a modified copy from run_aec_erle.py which needed some extra calcs
def get_erle(mic_in_array, aec_out_array, aec_ref_array):
  # Calculate EWM of audio power in 1s window
  mic_in_power = np.power(mic_in_array, 2)
  aec_out_power = np.power(aec_out_array, 2)
  aec_ref_power = np.power(aec_ref_array, 2)

  mic_in_power_ewm = pandas.Series(mic_in_power).ewm(span=16000).mean()
  aec_out_power_ewm = pandas.Series(aec_out_power).ewm(span=16000).mean()
  aec_ref_power_ewm = pandas.Series(aec_ref_power).ewm(span=16000).mean()

  # Get sum of average power
  mic_in_power_sum = mic_in_power_ewm.sum()
  aec_out_power_sum = aec_out_power_ewm.sum()
  aec_ref_power_sum = aec_ref_power_ewm.sum()
  
  erle = 10 * np.log10(mic_in_power_sum/(aec_out_power_sum + np.finfo(float).eps))
  return erle, mic_in_power_sum, aec_out_power_sum, aec_ref_power_sum

def get_file_energy_stats(aec_input_file, aec_out_file):
  pre_aec_data, fs = sf.read(aec_input_file)
  post_aec_data, fs = sf.read(aec_out_file)

  erle_list = []
  mic_in_power_list = []
  aec_out_power_list = []
  aec_ref_power_list = []

  for idx in range(0, post_aec_data.shape[0] - frame_size, frame_advance):
    #We are just looking at one channel here
    mic_in_data = pre_aec_data[idx:idx + frame_size, 0]
    aec_out_data = post_aec_data[idx:idx + frame_size, 0]
    aec_ref_data = post_aec_data[idx:idx + frame_size, 2]

    erle, mic_in_power, aec_out_power, aec_ref_power = get_erle(mic_in_data, aec_out_data, aec_ref_data)
    #print "ERLE: {}dB".format(erle)
    erle_list.append(erle)
    mic_in_power_list.append(mic_in_power)
    aec_out_power_list.append(aec_out_power)
    aec_ref_power_list.append(aec_ref_power)

  return erle_list, mic_in_power_list, aec_out_power_list, aec_ref_power_list

def do_delay_estimate(input_audio_file):
  delay_file = "delays.txt"
  delay_estimate_audio_file = "delay_estimate_input_16k.wav"
  #we have to shift the delay back 20 phases (0.3s) so that we can detect +- 0.3s
  delay_estimator_offset = 0.3
  file_delay_changes = [(voice_sample_rate * 0, voice_sample_rate * -delay_estimator_offset)]
  apply_delay_changes(input_audio_file, delay_estimate_audio_file, file_delay_changes)
  test_file(delay_estimate_audio_file, delay_file, 2)
  estimate = np.fromfile(delay_file, dtype=np.float, count=-1, sep="\n")
  return (delay_estimator_offset * voice_sample_rate) - estimate

if __name__ == '__main__':
    input_audio_dir = 'output_audio_from_room_model'
    input_audio_file = 'simulated_room_output.wav'
    output_audio_dir = '.'
    output_audio_file = 'stage_a_input_48k.wav'
    input_audio = os.path.join(input_audio_dir, input_audio_file)
    output_audio = os.path.join(output_audio_dir, output_audio_file)
    copyfile(input_audio, output_audio)
    #far_end_delay_changes = [(48000*20, 48000 * -0.1), (48000*30, 48000 * 0.0), (48000*40, 48000 * 0.1), (48000*50, 48000 * 0.0)]
    #far_end_delay_changes = [(48000*0, 48000 * -0.25), (48000*10, 48000 * -0.00), (48000*20, 48000 * 0.20), (48000*30, 48000 * -0.1), (48000*40, 48000 * 0.0), (48000*5, 48000 * 0.10)]
    far_end_delay_changes = [(48000*0, 48000 * -0.25), (48000*10, 48000 * -0.00), (48000*20, 48000 * 0.20), (48000*30, 48000 * -0.1), (48000*40, 48000 * 0.0), (48000*50, 48000 * 0.10)]
    
    gt = apply_delay_changes(output_audio, output_audio, far_end_delay_changes)

    plt.plot(gt)
    plt.show()
