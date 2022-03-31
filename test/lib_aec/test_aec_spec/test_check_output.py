# Copyright 2022 XMOS LIMITED.
# This Software is subject to the terms of the XMOS Public Licence: Version 1.
import configparser
import pytest
from aec_test_utils import read_wav, check_aec_output, get_test_instances,\
                           get_criteria, read_config


parser = configparser.ConfigParser()
parser.read("parameters.cfg")

in_dir = parser.get("Folders", "in_dir")
out_dir = parser.get("Folders", "out_dir")


@pytest.fixture
def test_type(request):
    test_name = request.node.name
    test_type = test_name[len("test_"):test_name.index('[')]
    return test_type


@pytest.fixture
def test_config(test_type):
    return read_config(test_type)


@pytest.mark.parametrize("test", get_test_instances('simple', in_dir, out_dir))
def test_simple(test, test_config, record_property):
    audio_in = read_wav(test['in_filename'])
    audio_ref = read_wav(test['ref_filename'])
    audio_out = read_wav(test['out_filename'])[:, 0]
    criteria = get_criteria(test['id'])

    for key in list(test.keys()):
        record_property(key, test[key])

    assert check_aec_output(audio_in, audio_ref, audio_out,
                            test_config['start_fft'], test_config['end_fft'],
                            criteria,
                            frequencies=test_config['frequencies'])


@pytest.mark.parametrize("test", get_test_instances('multitone', in_dir, out_dir))
def test_multitone(test, test_config, record_property):
    audio_in = read_wav(test['in_filename'])
    audio_ref = read_wav(test['ref_filename'])
    audio_out = read_wav(test['out_filename'])[:, 0]
    criteria = get_criteria(test['id'])

    for key in list(test.keys()):
        record_property(key, test[key])

    assert check_aec_output(audio_in, audio_ref, audio_out,
                            test_config['start_fft'], test_config['end_fft'],
                            criteria,
                            frequencies=test_config['frequencies'])


@pytest.mark.parametrize("test", get_test_instances('excessive', in_dir, out_dir))
def test_excessive(test, test_config, record_property):
    audio_in = read_wav(test['in_filename'])
    audio_ref = read_wav(test['ref_filename'])
    audio_out = read_wav(test['out_filename'])[:, 0]
    criteria = get_criteria(test['id'])

    for key in list(test.keys()):
        record_property(key, test[key])

    assert not check_aec_output(audio_in, audio_ref, audio_out,
                              test_config['start_fft'], test_config['end_fft'],
                              criteria,
                              frequencies=test_config['frequencies'])


@pytest.mark.parametrize("test", get_test_instances('impulseresponse', in_dir, out_dir))
def test_impulseresponse(test, test_config, record_property):
    audio_in = read_wav(test['in_filename'])
    audio_ref = read_wav(test['ref_filename'])
    audio_out = read_wav(test['out_filename'])[:, 0]
    criteria = get_criteria(test['id'])

    for key in list(test.keys()):
        record_property(key, test[key])

    assert True # TODO
    #assert not check_aec_output(audio_in, audio_ref, audio_out,
    #                          test_config['start_fft'], test_config['end_fft'],
    #                          criteria,
    #                          frequencies=test_config['frequencies'])


@pytest.mark.parametrize("test", get_test_instances('bandlimited', in_dir, out_dir))
def test_bandlimited(test, test_config, record_property):
    audio_in = read_wav(test['in_filename'])
    audio_ref = read_wav(test['ref_filename'])
    audio_out = read_wav(test['out_filename'])[:, 0]
    criteria = get_criteria(test['id'])

    for key in list(test.keys()):
        record_property(key, test[key])

    assert check_aec_output(audio_in, audio_ref, audio_out,
                            test_config['start_fft'], test_config['end_fft'],
                            criteria,
                            frequencies=test_config['frequencies'])


@pytest.mark.parametrize("test", get_test_instances('smallimpulseresponse', in_dir, out_dir))
def test_smallimpulseresponse(test, test_config, record_property):
    audio_in = read_wav(test['in_filename'])
    audio_ref = read_wav(test['ref_filename'])
    audio_out = read_wav(test['out_filename'])[:, 0]
    criteria = get_criteria(test['id'])

    for key in list(test.keys()):
        record_property(key, test[key])

    assert True # TODO
