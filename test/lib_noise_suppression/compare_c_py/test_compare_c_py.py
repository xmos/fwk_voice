# Copyright 2018-2021 XMOS LIMITED.
# This Software is subject to the terms of the XMOS Public Licence: Version 1.

from __future__ import print_function
#from future import standard_library
#standard_library.install_aliases()
from builtins import zip
import pytest
from compare_c_py import get_attenuation_c_py
import configparser


def read_config(testname, filename):
    parser = configparser.ConfigParser()
    parser.read(filename)
    cfg = {}
    cfg['noise_db'] = [int(x) for x in parser.get(testname, "noise_db").split(',')]
    cfg['noise_band'] = [x.strip() for x in parser.get(testname, "noise_band").split(',')]
    cfg['py_atten'] = [x.strip() for x in parser.get(testname, "py_atten").split(',')]
    return cfg


def get_test_instances(testname='gen_audio', filename='config.cfg'):
    config = read_config(testname, filename)
    tests = []
    i = 0
    for db in config['noise_db']:
        for noise in config['noise_band']:
            test_id = "{}_{}Hz_{}dB".format(testname, noise, db)

            arr = [0, 0, 0, 0, 0, 0, 0, 0, 0, 0]
            for t in range (i, i + 10):
                arr[t - i] = config['py_atten'][t]

            test_dict = {'id' : test_id,
                             'noise_band' : noise,
                             'noise_db' : db,
                             'attenuation_py' : arr}
            tests.append(test_dict)
            i = i + 10
    return tests


TESTS = get_test_instances('gen_audio', 'config.cfg')

@pytest.mark.parametrize('test', TESTS, ids = [test['id'] for test in TESTS])
def test_sup_attenuation(test):
    print("--- {} ---" .format(test['id']))

    attenuation_c = get_attenuation_c_py(test['id'], int(test['noise_band']), int(test['noise_db']))

    print("PYTHON SUP: {}".format(["%.2f"%item for item in test['attenuation_py']]))
    for (atten_bin_xc, atten_bin_py) in zip(attenuation_c, test['attenuation_py']):
        assert abs(atten_bin_xc - atten_bin_py) < 1
        assert atten_bin_py > 0