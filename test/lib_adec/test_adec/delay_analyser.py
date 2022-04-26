# Copyright 2022 XMOS LIMITED.
# This Software is subject to the terms of the XMOS Public Licence: Version 1.
import numpy as np
import matplotlib.pyplot as plt

FRAME_ADVANCE = 240

voice_sample_rate = 16000
frame_rate = float(voice_sample_rate) / FRAME_ADVANCE

#Tolerable delay estimation error in terms of MIC delay for the test
min_acceptable_delay_s = 0
max_acceptable_delay_s = 0.03 #Two phases

#Size of GT changes that will be accepted while NOT triggerng a DE event
tolerated_mic_delay_increase = 0.0225
tolerated_mic_delay_decrease = -0.0075 #This *should* be handled by our mic delay safety margin


#This class takes the ground truth and actual delay estimate outputs and analyses them
#and classifies the delay change events into different categories.
class delay_analyser:

  def __init__(self, frame_advance, ground_truth_file, delay_estimate_file):
    self.frame_advance = frame_advance

    self.ground_truth = np.fromfile(ground_truth_file, dtype=np.float, sep="\n")
    self.delay_estimate = np.fromfile(delay_estimate_file, dtype=np.float, sep="\n")

    self.good_estimates = []
    self.no_action_required = []
    self.inaccurate_estimates = []
    self.false_positives = []
    self.false_negatives = []

    gt_len = self.ground_truth.shape[0]
    de_len = self.delay_estimate.shape[0]
    if gt_len > de_len:
      print("Truncating ground_truth by {} samples".format(de_len-gt_len))
      self.ground_truth = self.ground_truth[:de_len]
    elif de_len > gt_len:
      print("Truncating delay_estimate by {} samples".format(de_len-gt_len))
      self.delay_estimate = self.delay_estimate[:gt_len]
    else:
      pass

    self.delays_to_events()

  def delays_to_events(self):
    self.gt_events = []
    self.measured_events = []
    
    #Trigger delay change event if new delay comes in different
    gt_current_delay_s = 0.0
    measured_current_delay_s = 0.0

    #Build list of events
    for frame_idx in range(self.ground_truth.shape[0]):
      gt = self.ground_truth[frame_idx]
      measured = self.delay_estimate[frame_idx]
      time = frame_idx / frame_rate
      #print time, gt, measured

      if gt != gt_current_delay_s:
        self.gt_events.append((time, gt))
        gt_current_delay_s = gt

      if measured != measured_current_delay_s:
        self.measured_events.append((time, measured))
        measured_current_delay_s = measured

      frame_idx += 1

    print ('gt_events =', self.gt_events)
    print ('measured_events = ', self.measured_events)

  def find_next_event_indicies(self, event_array, current_time):
    next_idx = 0
    next_found = False
    for event in event_array:
      if event[0] > current_time:
        next_found = True
        break
      next_idx += 1

    next_event_time = event_array[next_idx][0] if next_found else None
    next_event_amount = event_array[next_idx][1] if next_found else None

    return next_event_time, next_event_amount

  def check_range(self, estimate, ground_truth):
    in_range = True
    if ground_truth is None:
      return False

    diff =  estimate - ground_truth
    #print "diff: ", diff

    if diff > max_acceptable_delay_s or diff < min_acceptable_delay_s:
      in_range = False
    return in_range

  def analyse_events(self):
    current_time = 0.0

    last_event_was_gt = False
    last_de_time = 0.0
    last_gt_time = 0.0

    last_de_amount = 0.0
    last_gt_amount = 0.0

    while True:
      next_gt_time, next_gt_amount = self.find_next_event_indicies(self.gt_events, current_time)
      #print "next_gt_time: ", next_gt_time, ", amount: ", next_gt_amount
      next_de_time, next_de_amount = self.find_next_event_indicies(self.measured_events, current_time)
      #print "next_de_time: ", next_de_time, ", amount: ", next_de_amount

      #We have finished
      if next_gt_time is None and next_de_time is None:
        last_val_idx = -33 #half a second back because sometimes we get a glitch at the end of the run
        #Check last value
        last_value_inrange = self.check_range(self.delay_estimate[last_val_idx], self.ground_truth[last_val_idx])
        print ("*** last delay esimate and ground truth:", self.delay_estimate[last_val_idx], self.ground_truth[last_val_idx])
        
        #print "Done"
        result_summary = dict()
        result_summary['good_estimates'] = len(self.good_estimates)
        result_summary['no_action_required'] = len(self.no_action_required)
        result_summary['inaccurate_estimates'] = len(self.inaccurate_estimates)
        result_summary['false_positives'] = len(self.false_positives)
        result_summary['false_negatives'] = len(self.false_negatives)
        result_summary['last_value_inrange'] = last_value_inrange
        return result_summary
        break 

      #Next event is GT
      if next_de_time is None or (next_gt_time is not None and next_gt_time < next_de_time):

        #We had a GT change last time and still no DE change
        if last_event_was_gt:
          #First check delay change is big enough to register
          diff = last_de_amount - last_gt_amount
          is_significant_change = diff > tolerated_mic_delay_increase or diff < tolerated_mic_delay_decrease #+1.5 phase - -0.5 phase

          if is_significant_change:
            #print 
            self.false_negatives.append((next_gt_time, next_gt_amount))
            print ("FALSE NEGATIVE -", "last_de_amount", last_de_amount, "last_gt_amount", last_gt_amount)
          else:
            #There was a small GT change but DE correctly didn't respond to it
            self.no_action_required.append((next_de_time, next_de_amount))
        else:
          #Nothing - we have logged a new GT change and now waiting for DE change
          pass

        current_time = next_gt_time
        last_gt_time = next_gt_time
        last_event_was_gt = True
        last_gt_amount = next_gt_amount
        continue

      #Next event is DE
      if next_gt_time is None or (next_de_time is not None and next_de_time < next_gt_time):
        #print "DE @ {}".format(next_de_time)

        if last_event_was_gt:
          #print "Expected delay estimate @ time: {}".format(next_de_time - last_gt_time)
          inrange = self.check_range(next_de_amount, last_gt_amount)
          if not inrange:
            print ("NOT IN RANGE @ {} - last_gt_amount: {} next_de_amount: {}".format(next_de_time, last_gt_amount, next_de_amount))
            self.inaccurate_estimates.append((next_de_time, next_de_amount))
          else:
            self.good_estimates.append((next_de_time, next_de_amount))
        else:
          print("FALSE POSITIVE @ {}".format(next_de_time))
          self.false_positives.append((next_de_time, next_de_amount))
          #Is it accurate at least?
          inrange = self.check_range(last_de_amount, next_gt_amount)
          #print "In range: ", inrange
          if not inrange:
            print ("NOT IN RANGE @ {} - last_gt_amount: {} next_de_amount: {}".format(next_de_time, last_gt_amount, next_de_amount))
            self.inaccurate_estimates.append((next_de_time, next_de_amount))

        last_de_time = next_de_time
        current_time = next_de_time
        last_event_was_gt = False
        last_de_amount = next_de_amount
        continue

      #Un-reachable. This should never happen
      assert(0)

  def graph_delays(self, file_name = None):
    plt.clf() #We need to clear otherwise each gets overwritten into a mess 
    plt.cla()

    #trim one if longer than other (typicall one or two)
    min_length = len(self.ground_truth) if len(self.ground_truth) < len(self.delay_estimate) else len(self.delay_estimate)

    x_values = np.array([float(frame) / (voice_sample_rate/float(FRAME_ADVANCE)) for frame in range(min_length) ])
    delay_estimate = self.delay_estimate[:min_length]
    plt.plot(x_values, delay_estimate, label = "delay estimate")
    plt.axhline(y=0, color='k')
    
    plt.plot(x_values, self.ground_truth[:min_length], label = "ground truth", color='red', linewidth=2)
    plt.legend(loc='upper left', fontsize='x-small')

    if file_name:
      plt.savefig(file_name, bbox_inches = 'tight', dpi=300)
    else:
      plt.show()
