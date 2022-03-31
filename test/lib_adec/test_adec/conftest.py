# Copyright 2022 XMOS LIMITED.
# This Software is subject to the terms of the XMOS Public Licence: Version 1.
# content of conftest.py
import os
import numpy as np

source_wav_file_rate = 48000

# ADEC tests only pass with AEC configured in alt arch mode :(
pipeline_configs = [{"num_y_channels":1, "num_x_channels":2, "num_main_filter_phases":15, "num_shadow_filter_phases":5}]

def generate_random_delay_changes(number, spacing, min_s, max_s):
  delays = []
  for change in range(number):
    this_delay = np.random.uniform(min_s, max_s, 1)[0] * source_wav_file_rate
    this_time_samps = spacing * (1 + change) * source_wav_file_rate
    delays.append((int(this_time_samps), int(this_delay)))
  return delays

def scale_delay_changes(delays):
  scaled_delays = []
  for delay_change in delays:
    scaled_delays.append(([delay_change[0] * source_wav_file_rate, delay_change[1] * source_wav_file_rate]))
  return scaled_delays

def scale_vol_changes(vols):
  scaled_vols = []
  for vol in vols:
    scaled_vols.append([vol[0] * source_wav_file_rate, vol[1]])
  return scaled_vols


def add_test(test_info, test_list, path_to_regression_files, input_audio_files, far_end_delay_changes, limit_test_length_s, gt_changes, \
             allowable_false_negatives, allowable_false_positives, run_target=False, volume_changes=False):
  #if test_info != "small_mic_increase":
  #    return

  #if test_info != "limits":
  #    return

  #if test_info != "aec_delay_change":
  #    return
  test_conf = {}
  test_conf['info'] = test_info
  test_conf['path_to_regression_files'] = path_to_regression_files
  test_conf['input_audio_files'] = input_audio_files
  test_conf['far_end_delay_changes'] = far_end_delay_changes
  test_conf['volume_changes'] = volume_changes
  test_conf['test_length_s'] = limit_test_length_s
  test_conf['run_target'] = run_target
  test_conf['gt_changes'] = gt_changes
  test_conf['allowable_false_negatives'] = allowable_false_negatives
  test_conf['allowable_false_positives'] = allowable_false_positives

  for pipeline_config in pipeline_configs:
    test_conf['info'] = test_info + "_" + "_".join([str(v) for k,v in pipeline_config.items()])
    test_conf['pipeline_config'] = pipeline_config
    test_list.append(test_conf)

def pytest_generate_tests(metafunc):
  hydra_audio_path = os.environ.get('hydra_audio_PATH', '~/hydra_audio')
  #To allow Ed to work offline, first we check to see if we have the regression files locally and if so use them. If not, go to the network.
  path_to_regression_files = 'adec_regression/' if os.path.isdir('adec_regression') else os.path.join(hydra_audio_path, 'adec_regression')
  path_to_regression_files = os.path.expanduser(path_to_regression_files)
  print("...Expecting regression files in: {}".format(path_to_regression_files))


  test_wav_dir = os.path.join(path_to_regression_files, 'test_wavs')
  fixed_test_files = [f for f in os.listdir(test_wav_dir) if f.endswith('wav')]
  # fixed_test_files = ['vestel_fixed_40ms_ref_delay.wav']
  test_list = []
  target =  "xcore"

  do_random_tests = False #These use random delay values so are more useful for dev rather than regression 

  #Run through stock files
  for test_wav in fixed_test_files:
    input_audio_files = os.path.abspath(os.path.join(test_wav_dir, test_wav))
    allowable_false_positives = 0
    if test_wav == 'loud_near_end_15s_16k.wav' or test_wav == 'tv_60_60_3500.wav':
      allowable_false_positives = 1 #This is a really tricky and likely unrealistic case. tv_60_60_3500.wav appears to have time drift on it so allow a FP. ()
    add_test("stock_"+test_wav, test_list, path_to_regression_files, input_audio_files, None, None, 0, 0, allowable_false_positives, run_target=target)
    #TODO re-enable when all these pass (automatic detection)

  #Test specific points
  limit_test_length_s = 40
  gt_changes = 2
  far_end_delay_changes = scale_delay_changes([(10, 0.04), (20, 0.05), (30, 0.06)])
  input_audio_files = ['happy.wav', 'busy_bar_5min_48k.wav']
  add_test("test_headrooms_gt", test_list, path_to_regression_files, input_audio_files, far_end_delay_changes, limit_test_length_s, gt_changes, 0, 0, run_target=target)

  #Test specific points ensuring that we find a point where the direct path is spread equally between two phases. It goes back and forth, decrementing
  #by 40 samples (1/6 phase or 2.5ms) at a time 
  limit_test_length_s = 120
  far_end_delay_changes = scale_delay_changes([(20, 0.10), (40, -0.10), (60, 0.0975), (80, -0.10), (100, 0.095)])
  input_audio_files = ["e3-whats-cloud-gaming-bbc-news_squashed.wav", "busy_bar_5min_48k.wav"]
  add_test("phase_energy_spreading_1", test_list, path_to_regression_files, input_audio_files, far_end_delay_changes, limit_test_length_s, len(far_end_delay_changes), 0, 1, run_target=target)

  far_end_delay_changes = scale_delay_changes([(20, 0.0925), (40, -0.10), (60, 0.090), (80, -0.10), (100, 0.0875)])
  input_audio_files = ["e3-whats-cloud-gaming-bbc-news_squashed.wav", "busy_bar_5min_48k.wav"]
  add_test("phase_energy_spreading_2", test_list, path_to_regression_files, input_audio_files, far_end_delay_changes, limit_test_length_s, len(far_end_delay_changes), 0, 2, run_target=target)

  #Test volume changes
  limit_test_length_s = 50
  far_end_delay_changes = [(0,0)]
  input_audio_files = ["e3-whats-cloud-gaming-bbc-news.wav", "busy_bar_5min_48k.wav"]
  volume_changes = scale_vol_changes([(8, -6.0), (16, 0), (24, -9.0), (32, 0), (40, -12.0), (48, 0)]) #6,9,12 dB. May see up to three false positives 
  add_test("volume_changes", test_list, path_to_regression_files, input_audio_files, far_end_delay_changes, limit_test_length_s, len(far_end_delay_changes), 0, 4, run_target=target, volume_changes=volume_changes)

  #AEC normal mode delay change detect
  far_end_delay_changes = scale_delay_changes([(10, -0.03), (20, -0.06), (30, -0.09), (40, -0.12), (50, -0.15)])
  input_audio_files = ["e3-whats-cloud-gaming-bbc-news_squashed.wav", "busy_bar_5min_48k.wav"]
  add_test("aec_delay_change", test_list, path_to_regression_files, input_audio_files, far_end_delay_changes, 60, len(far_end_delay_changes), 1, 0, run_target=target)

  #Small +ve change (should ignore)
  far_end_delay_changes = scale_delay_changes([(10, -0.0075)])
  input_audio_files = ["e3-whats-cloud-gaming-bbc-news.wav", "busy_bar_5min_48k.wav"]
  add_test("small_mic_increase", test_list, path_to_regression_files, input_audio_files, far_end_delay_changes, 20, len(far_end_delay_changes), 0, 0, run_target=target)

  #Small -ve change (should change)
  input_audio_files = ['happy.wav', 'busy_bar_5min_48k.wav']
  far_end_delay_changes = scale_delay_changes([(10, 0.0075)])
  add_test("small_ref_increase", test_list, path_to_regression_files, input_audio_files, far_end_delay_changes, 20, len(far_end_delay_changes), 0, 0, run_target=target)

  #Big step change up to limits
  far_end_delay_changes = scale_delay_changes ([(10, 0.15), (20, -0.15)])
  add_test("limits", test_list, path_to_regression_files, input_audio_files, far_end_delay_changes, 30, len(far_end_delay_changes), 1, 0, run_target=target)

  #Test for delay change before convergence. One GT event at beginning which puts delay outside of AEC window (ref delay)
  input_audio_files = ['e3-whats-cloud-gaming-bbc-news.wav', 'busy_bar_5min_48k.wav']
  far_end_delay_changes = scale_delay_changes([(0.1, 0.1)])
  add_test("delay_at_start", test_list, path_to_regression_files, input_audio_files, far_end_delay_changes, 20, len(far_end_delay_changes), 0, 0, run_target=target)

  #Check if we can recover from rapid fire delay changes which will lead to a wrong setting with no convergence. Tests the watchdog feature..
  input_audio_files = ['e3-whats-cloud-gaming-bbc-news.wav', 'busy_bar_5min_48k.wav']
  far_end_delay_changes = scale_delay_changes([(3, 0.04), (5, -0.08), (7, 0.12)])
  add_test("rapid_changes", test_list, path_to_regression_files, input_audio_files, far_end_delay_changes, 42, len(far_end_delay_changes), 2, 2, run_target=target)

  # Vivaldi always was tricky (doesn't converge so well). Allow missing one change - main thing is we do get there eventually. 
  far_end_delay_changes = scale_delay_changes([(15, 0.110)])
  input_audio_files = ['Vivaldi_48kHz_2Ch_UCA202_SmallSpeakers_Short.wav', 'busy_bar_5min_48k.wav']
  add_test("tricky", test_list, path_to_regression_files, input_audio_files, far_end_delay_changes, 35, len(far_end_delay_changes), 1, 1, run_target=target)

  #Test the ADEC getting stuck after rapid pk:ave ratio increase after 3s 
  input_audio_files = ['sudden_white_noise_after_3s_silence.wav', 'silence_5m_1ch.wav']
  far_end_delay_changes = scale_delay_changes([(10, 0.1)])
  add_test("sudden_noise_stuck_adec", test_list, path_to_regression_files, input_audio_files, far_end_delay_changes, 28, len(far_end_delay_changes), 0, 0, run_target=target)


  if do_random_tests:
    # Now generate some tests using the room simulator and random delay insertion utility and test those. This will generate ground truth also
    limit_test_length_s = 70
    gt_changes = 6
    far_end_delay_changes = generate_random_delay_changes(gt_changes, 10, -0.15, +0.15)
    input_audio_files = ['happy.wav', 'busy_bar_5min_48k.wav']
    add_test("random_gt", test_list, path_to_regression_file, input_audio_files, far_end_delay_changes, limit_test_length_s, gt_changes, 1, 0, run_target=target)


    #Do same with talk/news audio
    input_audio_files = ["e3-whats-cloud-gaming-bbc-news.wav", "busy_bar_5min_48k.wav"]
    add_test("random_gt", test_list, path_to_regression_file, input_audio_files, far_end_delay_changes, limit_test_length_s, gt_changes, 1, 0, run_target=target)


  #Create list of names so test jobs are human readable
  test_name_list = [test["info"]+("_xcore" if test["run_target"] else "_model") for test in test_list]
  metafunc.parametrize("test_config", test_list, ids=test_name_list)
