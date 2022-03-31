# Copyright 2022 XMOS LIMITED.
# This Software is subject to the terms of the XMOS Public Licence: Version 1.
from builtins import str
from builtins import range
import sys
import os.path
import configparser
import numpy as np
import scipy.io.wavfile
import scipy.signal.windows
# export PYTHONPATH=$PYTHONPATH:audio_test_tools/python
from audio_generation import (get_filenames, get_magnitude,
                              get_suppressed_magnitude, db)


def files_exist(*args):
    for filename in args:
        if not os.path.isfile(filename):
            return False
    return True


def read_config(testname, filename='test_config.cfg'):
    parser = configparser.ConfigParser()
    parser.read(filename)
    cfg = {}
    cfg['settle_time'] = parser.getint(testname, "settle_time")
    cfg['start_fft'] = parser.getint(testname, "start_fft")
    cfg['end_fft'] = parser.getint(testname, "end_fft")
    cfg['ignore_exclusion'] = parser.getboolean(testname, "ignore_exclusion")
    cfg['headroom'] = [int(x) for x in parser.get(testname, "headroom").split(',')]
    cfg['echo'] = [x.strip() for x in parser.get(testname, "echo").split(',')]
    cfg['reference'] = [x.strip() for x in parser.get(testname, "reference").split(',')]
    try:
        frequencies = [int(x) for x in parser.get(testname, "frequencies").split(',')]
        cfg['frequencies'] = frequencies
    except configparser.NoOptionError:
        pass
    return cfg


def read_wav(filename):
    rate, data = scipy.io.wavfile.read(filename)
    return data.astype(float) / np.iinfo(data.dtype).max


def get_excluded_tests():
    excluded_tests = []
    with open('excluded_tests.txt', 'r') as f:
        for line in f.readlines():
            line = line.strip()
            excluded_tests.append(line)
    return list(set(excluded_tests))


def get_test_instances(testname, in_dir, out_dir):
    """ Gets all generated tests by checking test_config.cfg and the files
        present in the in_dir.

        Any tests in excluded_tests.txt will not be included."""
    tests = []
    excluded_tests = get_excluded_tests()
    cfg = read_config(testname)
    for headroom in cfg['headroom']:
        for echo_type in cfg['echo']:
            for ref_type in cfg['reference']:
                test_id = ",".join([testname, echo_type, ref_type,
                                    str(headroom)])
                in_filename, ref_filename, out_filename\
                    = get_filenames(testname, echo_type, ref_type, headroom)
                in_filename = os.path.join(in_dir, in_filename + ".wav")
                ref_filename = os.path.join(in_dir, ref_filename + ".wav")
                out_filename = os.path.join(out_dir,
                                               out_filename + ".wav")
                test_dict = {'id' : test_id,
                             'test_type' : testname,
                             'settle_time' : cfg['settle_time'],
                             'headroom' : headroom,
                             'echo' : echo_type,
                             'reference' : ref_type,
                             'in_filename' : in_filename,
                             'ref_filename' : ref_filename,
                             'out_filename' : out_filename}
                if files_exist(in_filename, ref_filename):
                    tests.append(test_dict)
    return tests


def get_section(testid, sections):
    best_section = 'DEFAULT'
    best_precision = np.iinfo(np.int32).min
    testid = testid.split(',')
    for section in sections:
        section = section.split(',')
        precision = -len([x for x in section if x == '*'])
        match = True
        for i in range(len(section)):
            if section[i] == testid[i] or section[i] == '*':
                continue
            match = False
        if match and precision > best_precision:
            best_precision = precision
            best_section = ','.join(section)
    return best_section


def get_criteria(testid, filename="criteria.cfg"):
    parser = configparser.ConfigParser()
    parser.read(filename)
    criteria = {}
    section = get_section(testid, parser.sections())
    for key, val in parser.items(section):
        criteria[key] = val
    return criteria


def get_h_hat_impulse_response(h_hat, y_channel, x_channel):
    """Gets the impulse response of h_hat.

    h_hat is an array internal to the aec with a shape as follows:
    (y_channel_count, x_channel_count, max_phase_count, f_bin_count)

    Args:
        h_hat: h_hat array
        y_channel: y_channel to plot
        x_channel: x_channel to plot

    Returns:
        Impulse response of h_hat for channel pair (y_channel, x_channel)
    """

    y_channel_count = h_hat.shape[0]
    x_channel_count = h_hat.shape[1]
    max_phase_count = h_hat.shape[2]
    f_bin_count     = h_hat.shape[3]
    frame_advance = 240
    h_hat_ir = np.zeros((max_phase_count * frame_advance,))

    for phase in range(max_phase_count):
        phase_ir = np.fft.irfft(h_hat[y_channel][x_channel][phase])
        start   = frame_advance *  phase
        end     = frame_advance * (phase + 1)
        h_hat_ir[start:end] = phase_ir[:frame_advance]

    return h_hat_ir


def check_aec_output(audio_in, audio_ref, audio_out, start_s, end_s, criteria,
                     frequencies, Fs=16000, debug=True):
    success = True
    start = Fs * start_s
    end = Fs * end_s
    window = scipy.signal.windows.hann(end - start, sym=True)
    In = np.abs(np.fft.rfft(audio_in[start:end] * window))
    Ref = np.abs(np.fft.rfft(audio_ref[start:end] * window))
    Out = np.abs(np.fft.rfft(audio_out[start:end] * window))
    # Check for near-end frequencies
    for f in frequencies:
        in_mag = get_magnitude(f, In, Fs, 10, normalise=True)
        out_mag = get_magnitude(f, Out, Fs, 10, normalise=True)
        db_out = db(out_mag, in_mag)
        suppression_max = int(criteria['near_end_max_suppression'])
        if db_out < suppression_max:
            print("Check failed! Near-end Frequency: %d, db: %f (< %ddB)"\
                  % (f, db_out, suppression_max))
            success = False
        if debug:
            print("freq: %d, in: %f, out: %f, db: %f"\
                  % (f, in_mag, out_mag, db_out))
    # Check magnitude of suppressed frequencies
    band_min = int(criteria['suppression_band_min'])
    band_max = int(criteria['suppression_band_max'])
    max_suppressed_magnitude, freq = get_suppressed_magnitude(frequencies, Out,
                                                              Fs, 10,
                                                              band_min=band_min,
                                                              band_max=band_max)
    suppression_min = int(criteria['far_end_min_suppression'])
    db_suppressed = db(max_suppressed_magnitude, np.max(In))
    if db_suppressed > suppression_min:
        print("Check failed! Suppression at freq %d is %fdB (> %ddB)"\
                % (freq, db_suppressed, suppression_min))
        success = False
    else:
        print("Suppression check passed.")
    if success:
        print("Check passed!")
    return success
