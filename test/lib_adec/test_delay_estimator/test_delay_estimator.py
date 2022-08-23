# Copyright 2022 XMOS LIMITED.
# This Software is subject to the terms of the XMOS Public Licence: Version 1.
from builtins import range
from builtins import object
import tempfile
import sys
import os
import warnings

from scipy.signal import convolve
import scipy.io.wavfile
import audio_generation
import audio_wav_utils as awu
import pytest
import subprocess
import numpy as np

import filters
import xscope_fileio
import xtagctl
import io
from contextlib import redirect_stdout
import re
import glob

input_folder = os.path.abspath("input_wavs")
output_folder = os.path.abspath("output_files")
hydra_audio_base_dir = os.path.expanduser("~/hydra_audio/")

delay_calc_output_file_name = "measured_delay_samples.bin"

xe_path = os.path.abspath(glob.glob('../../../build/test/lib_adec/test_delay_estimator/bin/*.xe')[0])

sample_rate = 16000
proc_frame_length = 2**9 # = 512
frame_advance = 240


try:
    hydra_audio_base_dir = os.environ['hydra_audio_PATH']
except:
    print(f'Warning: hydra_audio_PATH environment variable not set. Using local path {hydra_audio_base_dir}')

class TestCase(object):
    def __init__(self, name, h_x, h_y, aud_x=None, aud_y=None, dont_check=[], invert_check=[]):
        self.name = name
        self.h_x = h_x
        self.h_y = h_y
        self.aud_x = aud_x
        self.aud_y = aud_y
        self._dont_check = dont_check
        self._invert_check = invert_check

        if aud_x is None:
            self.aud_x = audio_generation.get_noise(duration=10, db=-20)
        if aud_y is None:
            self.aud_y = audio_generation.get_noise(duration=10, db=-20)

        # Memoization array
        self._delay_calculated = np.zeros(self._get_num_frames())
        self._delay = np.zeros(self._get_num_frames())


    def get_delay(self, frame_num=0):
        """ Get the delay between the mic and reference channels

        Delay will be positive if mics arrive after reference

        Delay will be negative if mics arrive before reference
        """
        if frame_num > self._get_num_frames():
            raise ValueError

        if self._delay_calculated[frame_num]:
            return self._delay[frame_num]

        h_x = self.h_x.get_filter(frame_num)
        h_y = self.h_y.get_filter(frame_num)
        h_x_prev = self.h_x.get_filter(frame_num - 1)
        h_y_prev = self.h_y.get_filter(frame_num - 1)

        if frame_num > 0 and self._delay_calculated[frame_num - 1]:
            if np.array_equal(h_x, h_x_prev) and np.array_equal(h_y, h_y_prev):
                self._delay_calculated[frame_num] = 1
                self._delay[frame_num] = self._delay[frame_num - 1]
                return self._delay[frame_num]

        length = max(len(h_x), len(h_y)) * 2
        h_x_pad = np.pad(h_x, (0, length - len(h_x)), 'constant')
        h_y_pad = np.pad(h_y, (0, length - len(h_y)), 'constant')
        corr = scipy.signal.correlate(h_y_pad, h_x_pad, mode='same')

        delay = np.argmax(corr) - (length // 2)
        self._delay_calculated[frame_num] = 1
        self._delay[frame_num] = delay

        return delay


    def _get_num_frames(self):
        input_len = min(len(self.aud_x), len(self.aud_y))
        return int(input_len // frame_advance)


    def get_test_name(self):
        return self.name.lower().replace(' ', '-')


    def __getattr__(self, name):
        if "do_check" == name[:len("do_check")]:
            check = name[len("do_check_"):]
            return not check in self._dont_check
        if "invert_check" == name[:len("invert_check")]:
            check = name[len("invert_check_"):]
            return check in self._invert_check
        raise AttributeError


class DelaySpec(object):
    # Time in seconds to reach the correct delay
    convergence_time = 2.0


data, jazz = scipy.io.wavfile.read(hydra_audio_base_dir+'/fwk_voice_tests/test_delay_estimator/jazz_4ch_record_10s.wav')
jazz = jazz.T.astype(float) / np.iinfo(np.int32).max
#jazz_y = np.sum(jazz[:2], axis=0)[:jazz_length*16000]
#jazz_x = np.sum(jazz[2:], axis=0)[:jazz_length*16000]

jazz_y = jazz[0, :]
jazz_x = jazz[2, :]

print(jazz_x.shape)
print(jazz_y.shape)

test_vectors = [
    TestCase('Identical Mics', filters.Identity(), filters.Identity()),
    TestCase('Impulse at minus 20 samples', filters.OneImpulse(20), filters.Identity(),
             dont_check=['convergence', 'stability', 'correct']),
    TestCase('Jazz 1000 sample delay', filters.Identity(), filters.OneImpulse(1000),
             aud_x=jazz_x, aud_y=jazz_y, dont_check=['stability']),
    TestCase('Impulse at 500 samples', filters.Identity(), filters.OneImpulse(500)),
    TestCase('Impulse at 1000 samples', filters.Identity(), filters.OneImpulse(1000)),
    TestCase('Impulse at 7000 samples', filters.Identity(), filters.OneImpulse(7000)),
    TestCase('Jazz', filters.Identity(), filters.Identity(),
             aud_x=jazz_x, aud_y=jazz_y, dont_check=['stability']),
    #TestCase('Impulse at 9000 samples', filters.Identity(), filters.OneImpulse(9000)),
]


def write_input(test_name, input_data):
    input_32bit = awu.convert_to_32_bit(input_data)
    input_filename = os.path.abspath(os.path.join(
        input_folder, test_name + "-input.wav"))
    scipy.io.wavfile.write(input_filename, sample_rate, input_32bit.T)


def write_output(test_name, output, xc_or_py):
    output_filename = os.path.abspath(os.path.join(
        output_folder, test_name + "-output-{}.txt".format(xc_or_py)))
    np.savetxt(output_filename, output)


def process_audio(input_data, test_name):
    tmp_folder = tempfile.mkdtemp(suffix=os.path.basename(test_name))
    prev_path = os.getcwd()
    os.chdir(tmp_folder)
    #write runtime arguments into args.bin
    with open("args.bin", "wb") as fargs:
        fargs.write(f"y_channels 1\n".encode('utf-8'))
        fargs.write(f"x_channels 1\n".encode('utf-8'))
        fargs.write(f"main_filter_phases 30\n".encode('utf-8'))
        fargs.write(f"shadow_filter_phases 0\n".encode('utf-8'))
        fargs.write(f"adaption_mode 1\n".encode('utf-8'))
        #force_mu = int(0.4 * (1<<30))
        #fargs.write(f"force_adaption_mu {force_mu}\n".encode('utf-8'))
    # Write input data to file
    input_32bit = awu.convert_to_32_bit(input_data)
    scipy.io.wavfile.write('input.wav', sample_rate, input_32bit.T)
    with xtagctl.acquire("XCORE-AI-EXPLORER") as adapter_id:
        xscope_fileio.run_on_target(adapter_id, xe_path)
        with open(delay_calc_output_file_name, 'r') as f:
            output = np.array([int(l) for l in f.readlines()], dtype=float)
        write_output(test_name, output, 'xc')
        os.chdir(prev_path)
        os.system("rm -r {}".format(tmp_folder))

    return output.T


@pytest.fixture
def test_input(request):
    test_case = request.param
    test_name = test_case.get_test_name()
    # Generate Audio
    noise = audio_generation.get_noise(duration=10, db=-20)
    audio_x, audio_y = filters.convolve(test_case.aud_x, test_case.aud_y,
                                        test_case.h_x, test_case.h_y)
    combined_data = np.vstack((audio_y, audio_y, audio_x, audio_x))
    if np.max(np.abs(audio_x)) > 1:
        warnings.warn("{}: max(abs(Mic 1)) == {}".format(test_name, np.max(np.abs(audio_x))))
    if np.max(np.abs(audio_y)) > 1:
        warnings.warn("{}: max(abs(Mic 0)) == {}".format(test_name, np.max(np.abs(audio_y))))
    # Write the input audio to file
    input_32bit = awu.convert_to_32_bit(combined_data)
    write_input(test_name, input_32bit)
    return (test_case, combined_data)


def get_delay_arr(test_case, num_frames):
    """ Get real delay on input.

    Could be optimised if slow...
    """

    delay_arr = np.zeros(num_frames)
    for i in range(num_frames):
        delay_arr[i] = test_case.get_delay(i)
    return delay_arr


def get_contiguous_regions(data):
    regions = {}
    last_val = data[0]
    last_i = 0
    for i in range(1, len(data)):
        if data[i] != last_val:
            regions[last_i] = i - last_i
            last_val = data[i]
            last_i = i
    regions[last_i] = len(data) - last_i
    return regions


def check_convergence(record_property, test_case, delay_arr, output):
    """ Checks the convergence time is less than the spec

    Convergence time == max time the output takes to converge when the delay is
    constant.
    """

    #delay_arr_rounded = delay_arr - (delay_arr % frame_advance)
    #regions = get_contiguous_regions(delay_arr_rounded == output)

    #worst_convergence = np.argmin((delay_arr_rounded == output) == 1).flatten()
    #if len(worst_convergence) == 0:
    #    worst_convergence = -1
    #else:
    #    worst_convergence = worst_convergence[0]

    worst_convergence = -1
    num_frames = len(output)
    cur_delay = delay_arr[0] - (delay_arr[0] % frame_advance)
    cur_index = 0
    for i in range(1, num_frames):
        next_delay = delay_arr[i] - (delay_arr[i] % frame_advance)
        if cur_delay != next_delay or i == num_frames-1:
            try:
                frames_taken = np.min(np.argwhere(output[cur_index:i] == cur_delay))
            except ValueError:
                # Did not converge
                frames_taken = i - num_frames
            if frames_taken > worst_convergence:
                worst_convergence = frames_taken

            cur_delay = next_delay
            cur_index = i

    convergence_spec_frames = int((DelaySpec.convergence_time * sample_rate)\
                                   // frame_advance)
    check = (worst_convergence <= convergence_spec_frames)\
            and worst_convergence >= 0

    record_property("Worst convergence (frames)", str(worst_convergence))
    worst_convergence_secs = worst_convergence * frame_advance / float(sample_rate)
    record_property("Worst convergence (seconds)", str(worst_convergence_secs))
    record_property("Converged", str(check))

    # Invert the check if the test vector shouldn't converge
    if not test_case.do_check_convergence:
        return True
    if test_case.invert_check_convergence:
        check = not check
    return check


def check_stability(record_property, test_case, delay_arr, output):
    """ Checks that the estimated delay stays constant when the delay isn't
    changing.
    """

    delay_arr_rounded = delay_arr - (delay_arr % frame_advance)
    output_regions = get_contiguous_regions(output)
    delay_regions = get_contiguous_regions(delay_arr)

    num_frames = len(output)

    delay_keys = list(delay_regions.keys())
    delay_keys.sort()
    delay_keys.append(num_frames)
    delay_keys = np.array(delay_keys)

    output_keys = list(output_regions.keys())
    output_keys.sort()
    output_keys.append(num_frames)
    output_keys = np.array(output_keys)

    check = True
    max_changes = 0
    for i, key in enumerate(delay_keys):
        if i == len(delay_keys) - 1:
            break
        next_key = delay_keys[i+1]

        # Maximum of 2 output regions in each delay change region
        num_regions = len(np.where((output_keys > key) & (output_keys < next_key))[0])
        if num_regions > 2:
            check = False
        if num_regions > max_changes:
            max_changes = num_regions

    record_property("Max. estimate changes", max_changes)
    record_property("Stable", check)

    # Invert the check if the test vector shouldn't check stability
    if not test_case.do_check_stability:
        return True
    if test_case.invert_check_stability:
        check = not check
    return check


def check_correct(record_property, test_case, delay_arr, output):
    """ Checks that the 3 largest correct contiguous regions take up >90%
    of the (len(output) - time till first correct region)
    """

    delay_arr_rounded = delay_arr - (delay_arr % frame_advance)
    regions = get_contiguous_regions(delay_arr_rounded == output)

    correct_regions = {}
    for key in regions:
        if delay_arr_rounded[key] == output[key]:
            correct_regions[key] = regions[key]

    check = False
    num_correct_frames = 0
    if len(correct_regions) != 0:
        first_correct_frame = list(correct_regions.items())[0][0]
        # Get size of 3 largest contiguous regions
        region_sizes = list(correct_regions.values())
        region_sizes.sort()
        num_correct_frames = sum(region_sizes[:3])
        if num_correct_frames > 0.9 * len(output) - first_correct_frame:
            check = True

    record_property('Num correct frames', num_correct_frames)
    record_property('Correct', check)

    # Invert the check if the test vector shouldn't check stability
    if not test_case.do_check_correct:
        return True
    if test_case.invert_check_correct:
        check = not check
    return check


@pytest.mark.parametrize('test_input', test_vectors, indirect=True)
def test_all(test_input, record_property):
    test_case, input_audio = test_input

    output = process_audio(input_audio, test_case.get_test_name())
    delay_arr = get_delay_arr(test_case, len(output))

    record_property('Test name', test_case.get_test_name())
    record_property('delay_arr', np.array_repr(delay_arr))
    record_property('output', np.array_repr(output))

    # Run checks
    converged = check_convergence(record_property, test_case, delay_arr, output)
    stable = check_stability(record_property, test_case, delay_arr, output)
    correct = check_correct(record_property, test_case, delay_arr, output)

    print("{}".format(test_case.name))
    # Assert checks
    criteria = [converged, stable, correct]
    assert np.all(criteria), " and ".join([str(c) for c in criteria])

