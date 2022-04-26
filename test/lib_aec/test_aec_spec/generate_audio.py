# Copyright 2022 XMOS LIMITED.
# This Software is subject to the terms of the XMOS Public Licence: Version 1.
from builtins import str
import configparser
import argparse
import numpy as np
from scipy.signal import convolve

from aec_test_utils import read_config, get_excluded_tests
from audio_generation import get_ref, get_noise, get_near_end,\
                             get_h, DEFAULT_SAMPLE_RATE, get_headroom_divisor
from audio_generation import write_audio as write_audio_ag
from timeit import default_timer as timer

excluded_tests = get_excluded_tests()


def is_excluded(testname, headroom, echo_type, ref_type):
    test_id = ','.join([testname, echo_type, ref_type, str(headroom)])
    return test_id in excluded_tests


def write_audio(*args, **kwargs):
    start_time = timer()
    write_audio_ag(*args, **kwargs)
    end_time = timer()
    print("Time to write audio: %f" % (end_time - start_time))


def generate_simple_tests(audio_dir='spec_audio', settings=None):
    cfg = read_config('simple')
    settle_time = cfg['settle_time']
    headrooms = cfg['headroom']
    echos = cfg['echo']
    references = cfg['reference']
    if settings:
        echos = [settings[0]]
        references = [settings[1]]
        headrooms = [int(settings[2])]
    total_time = 2*settle_time
    for headroom in headrooms:
        for echo_type in echos:
            for ref_type in references:
                if  not cfg['ignore_exclusion']\
                    and is_excluded('simple', headroom, echo_type, ref_type):
                    continue
                timer_start = timer()
                ref = get_ref(total_time, ref_type)
                background_noise = get_noise(samples=len(ref), db=-150)
                near_end = get_near_end(settle_time,
                                        frequencies=cfg['frequencies'])
                near_end = np.concatenate(
                    (background_noise[:-len(near_end)],
                     background_noise[-len(near_end):] + near_end))
                ref_room = convolve(ref, get_h(echo_type))[:len(ref)]
                AudioIn = ref_room + near_end
                AudioRef = ref
                write_audio('simple', echo_type, ref_type, headroom, AudioIn,
                            AudioRef, audio_dir=audio_dir)
                timer_end = timer()
                print("Total time to generate: %f" % (timer_end - timer_start))



def generate_multitone_tests(audio_dir='spec_audio', settings=None):
    cfg = read_config('multitone')
    settle_time = cfg['settle_time']
    headrooms = cfg['headroom']
    echos = cfg['echo']
    references = cfg['reference']
    if settings:
        echos = [settings[0]]
        references = [settings[1]]
        headrooms = [int(settings[2])]
    total_time = 2*settle_time
    for headroom in headrooms:
        for echo_type in echos:
            for ref_type in references:
                if  not cfg['ignore_exclusion']\
                    and is_excluded('multitone', headroom, echo_type, ref_type):
                    continue
                timer_start = timer()
                ref = get_ref(total_time, ref_type)
                background_noise = get_noise(samples=len(ref), db=-150)
                near_end = get_near_end(settle_time,
                                        frequencies=cfg['frequencies'])
                near_end = np.concatenate(
                    (background_noise[:-len(near_end)],
                     background_noise[-len(near_end):] + near_end))
                ref_room = convolve(ref, get_h(echo_type))[:len(ref)]
                AudioIn = ref_room + near_end
                AudioRef = ref
                write_audio('multitone', echo_type, ref_type, headroom, AudioIn,
                            AudioRef, audio_dir=audio_dir)
                timer_end = timer()
                print("Total time to generate: %f" % (timer_end - timer_start))


def generate_impulseresponse_tests(audio_dir='spec_audio', settings=None):
    cfg = read_config('impulseresponse')
    settle_time = cfg['settle_time']
    headrooms = cfg['headroom']
    echos = cfg['echo']
    references = cfg['reference']
    if settings:
        echos = [settings[0]]
        references = [settings[1]]
        headrooms = [int(settings[2])]
    total_time = 2*settle_time
    for headroom in headrooms:
        for echo_type in echos:
            for ref_type in references:
                if  not cfg['ignore_exclusion']\
                    and is_excluded('impulseresponse', headroom, echo_type,
                                    ref_type):
                    continue
                ref = get_ref(total_time, ref_type)
                background_noise = get_noise(samples=len(ref), db=-90)
                near_end = background_noise
                ref[settle_time * DEFAULT_SAMPLE_RATE:] = \
                        background_noise[:-settle_time * DEFAULT_SAMPLE_RATE]
                transfer_function = get_h(echo_type, normalise=False)
                ref_room = convolve(ref, transfer_function)[:len(ref)]
                AudioIn = ref_room + near_end
                AudioRef = ref
                divisor = get_headroom_divisor(AudioIn, headroom)
                AudioIn = AudioIn / divisor
                AudioRef = AudioRef / divisor
                AudioRef[(settle_time + 1) * DEFAULT_SAMPLE_RATE] = -0.99
                write_audio('impulseresponse', echo_type, ref_type, headroom,
                            AudioIn, AudioRef, audio_dir=audio_dir,
                            adjust_headroom=False)


def generate_smallimpulseresponse_tests(audio_dir='spec_audio', settings=None):
    cfg = read_config('smallimpulseresponse')
    settle_time = cfg['settle_time']
    headrooms = cfg['headroom']
    echos = cfg['echo']
    references = cfg['reference']
    if settings:
        echos = [settings[0]]
        references = [settings[1]]
        headrooms = [int(settings[2])]
    total_time = 2*settle_time
    for headroom in headrooms:
        for echo_type in echos:
            for ref_type in references:
                if  not cfg['ignore_exclusion']\
                    and is_excluded('smallimpulseresponse', headroom, echo_type,
                                    ref_type):
                    continue
                ref = get_ref(total_time, ref_type)
                background_noise = get_noise(samples=len(ref), db=-90)
                near_end = background_noise
                ref[settle_time * DEFAULT_SAMPLE_RATE:] = \
                        background_noise[:-settle_time * DEFAULT_SAMPLE_RATE]
                transfer_function = get_h(echo_type, normalise=False)
                ref_room = convolve(ref, transfer_function)[:len(ref)]
                AudioIn = ref_room + near_end
                AudioRef = ref
                divisor = get_headroom_divisor(AudioIn, headroom)
                AudioIn = AudioIn / divisor
                AudioRef = AudioRef / divisor
                AudioRef[(settle_time + 1) * DEFAULT_SAMPLE_RATE] = -0.99 / (1<<10)
                write_audio('smallimpulseresponse', echo_type, ref_type, headroom,
                            AudioIn, AudioRef, audio_dir=audio_dir,
                            adjust_headroom=False)


def generate_excessive_tests(audio_dir='spec_audio', settings=None):
    cfg = read_config('excessive')
    settle_time = cfg['settle_time']
    headrooms = cfg['headroom']
    echos = cfg['echo']
    references = cfg['reference']
    if settings:
        echos = [settings[0]]
        references = [settings[1]]
        headrooms = [int(settings[2])]
    total_time = 2*settle_time
    for headroom in headrooms:
        for echo_type in echos:
            for ref_type in references:
                if  not cfg['ignore_exclusion']\
                    and is_excluded('excessive', headroom, echo_type, ref_type):
                    continue
                ref = get_ref(total_time, ref_type)
                background_noise = get_noise(samples=len(ref), db=-150)
                near_end = get_near_end(settle_time,
                                        frequencies=cfg['frequencies'])
                near_end = np.concatenate((background_noise[:-len(near_end)],
                                           background_noise[-len(near_end):] + near_end))
                ref_room = convolve(ref, get_h(echo_type))[:len(ref)]
                AudioIn = ref_room + near_end
                AudioRef = ref
                write_audio('excessive', echo_type, ref_type, headroom,
                            AudioIn, AudioRef, audio_dir=audio_dir)


def generate_bandlimited_tests(audio_dir='spec_audio', settings=None):
    cfg = read_config('bandlimited')
    settle_time = cfg['settle_time']
    headrooms = cfg['headroom']
    echos = cfg['echo']
    references = cfg['reference']
    if settings:
        echos = [settings[0]]
        references = [settings[1]]
        headrooms = [int(settings[2])]
    total_time = 2*settle_time
    for headroom in headrooms:
        for echo_type in echos:
            for ref_type in references:
                if  not cfg['ignore_exclusion']\
                    and is_excluded('bandlimited', headroom, echo_type, ref_type):
                    continue
                ref = get_ref(total_time, ref_type)
                background_noise = get_noise(samples=len(ref), db=-150)
                near_end = get_near_end(settle_time,
                                        frequencies=cfg['frequencies'])
                near_end = np.concatenate((background_noise[:-len(near_end)],
                                           background_noise[-len(near_end):] + near_end))
                ref_room = convolve(ref, get_h(echo_type))[:len(ref)]
                AudioIn = ref_room + near_end
                AudioRef = ref
                write_audio('bandlimited', echo_type, ref_type, headroom,
                            AudioIn, AudioRef, audio_dir=audio_dir)


def main():
    global excluded_tests
    config_parser = configparser.ConfigParser()
    config_parser.read("parameters.cfg")
    in_dir = config_parser.get("Folders", "in_dir")

    parser = argparse.ArgumentParser(description='Generate AEC test audio files.')
    parser.add_argument('--audio-dir', type=str, help='Directory for wav outputs',
                        default=in_dir)
    parser.add_argument('--sub-test', type=str, default=None,
                        help="""Specify a specific test to generate e.g.
                                --sub-test simple,short,discrete,2""")
    args = parser.parse_args()
    if args.sub_test:
        excluded_tests = []
        testname, echo, ref, headroom = args.sub_test.split(',')
        gen_function = globals()["generate_%s_tests" % testname]
        gen_function(audio_dir=args.audio_dir, settings=[echo, ref, headroom])
    else:
        generate_simple_tests(audio_dir=args.audio_dir)
        generate_multitone_tests(audio_dir=args.audio_dir)
        generate_impulseresponse_tests(audio_dir=args.audio_dir)
        generate_smallimpulseresponse_tests(audio_dir=args.audio_dir)
        generate_excessive_tests(audio_dir=args.audio_dir)
        generate_bandlimited_tests(audio_dir=args.audio_dir)

if __name__ == "__main__":
    main()
