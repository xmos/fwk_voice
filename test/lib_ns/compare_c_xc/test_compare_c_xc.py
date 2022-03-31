# Copyright 2022 XMOS LIMITED.
# This Software is subject to the terms of the XMOS Public Licence: Version 1.

from __future__ import print_function
#from future import standard_library
#standard_library.install_aliases()
from builtins import zip
import pytest
from compare_c_xc import get_attenuation_c_xc
import configparser
import numpy as np


def read_config(testname, filename):
    parser = configparser.ConfigParser()
    parser.read(filename)
    cfg = {}
    cfg['noise_db'] = [int(x) for x in parser.get(testname, "noise_db").split(',')]
    cfg['noise_band'] = [x.strip() for x in parser.get(testname, "noise_band").split(',')]
    return cfg


def get_test_instances(testname='gen_audio', filename='config.cfg'):
    config = read_config(testname, filename)
    tests = []
    for db in config['noise_db']:
        for noise in config['noise_band']:
            test_id = "{}_{}Hz_{}dB".format(testname, noise, db)

            test_dict = {'id' : test_id,
                             'noise_band' : noise,
                             'noise_db' : db}
            tests.append(test_dict)
    return tests


TESTS = get_test_instances('gen_audio', 'config.cfg')

@pytest.mark.parametrize('test', TESTS, ids = [test['id'] for test in TESTS])
def test_ns_attenuation(test):
    print("--- {} ---" .format(test['id']))

    attenuation_c, attenuation_xc = get_attenuation_c_xc(test['id'], int(test['noise_band']), int(test['noise_db']))

    c_res = np.array(attenuation_c)
    xc_res = np.array(attenuation_xc)
    for (atten_bin_xc, atten_bin_c) in zip(xc_res, c_res):
        assert atten_bin_xc - 1 < atten_bin_c
        assert atten_bin_c > 0