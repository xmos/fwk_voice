# Copyright 2022 XMOS LIMITED.
# This Software is subject to the terms of the XMOS Public Licence: Version 1.
import configparser
import subprocess
import os.path
import numpy as np
from aec_test_utils import get_h_hat_impulse_response
from plot_test import plot_test, plot_impulseresponse_test
import shutil

import xml.etree.ElementTree as ET

parser = configparser.ConfigParser()
parser.read("parameters.cfg")
results_dir = parser.get("Folders", "results_dir")
plot_dir_pass = os.path.join(results_dir, "plots")
plot_dir_fail = os.path.join(results_dir, "plots/fail")
log_dir_pass = os.path.join(results_dir, "logs")
log_dir_fail = os.path.join(results_dir, "logs/fail")

out_dir = parser.get("Folders", "out_dir")

def get_tests(filename):
    tree = ET.parse(filename)
    test_cases = tree.findall(".//testcase")
    tests = {}
    for result in test_cases:
        if not (result.find('skipped') is None):
            # Test not run in test_check_output
            continue
        test = {}
        for key_value in result.find("properties").findall('property'):
            test[key_value.attrib['name']] = key_value.attrib['value']
        failed = not (result.find('failure') is None)
        test['failed'] = failed
        try:
            test['log'] = result.find('system-out').text
        except AttributeError:
            test['log'] = ""
        test_type = test['test_type']
        if not test_type in list(tests.keys()):
            tests[test_type] = []
        tests[test_type].append(test)
    return tests


def parse_simple_tests(tests):
    failed_tests = []
    for test in tests:
        test_id = test['id']
        plot_dir = plot_dir_pass
        log_dir = log_dir_pass

        if test['failed']:
            failed_tests.append(test_id)
            plot_dir = plot_dir_fail
            log_dir = log_dir_fail

        plot_filename = os.path.join(plot_dir, "%s.png" % (test_id))
        plot_test(plot_filename, test['id'],
                  test['in_filename'], test['ref_filename'],
                  test['out_filename'], int(test['settle_time']))
        log_filename = os.path.join(log_dir, "%s.log" % (test_id))
        print("Log Filename: %s" % log_filename)
        with open(log_filename, 'w') as f:
            f.write(test['log'])
    return failed_tests


def get_h_hat(filename):
    """Gets H_hat from XC H_hat dump

    WARNING: This could be dangerous, the filename is assumed to be able
             to be parsed as python
    """
    shutil.copy2(filename, "temp.py")
    from temp import H_hat
    assert H_hat is not None
    return H_hat


def parse_impulseresponse_tests(tests):
    failed_tests = []
    for test in tests:
        test_id = test['id']
        plot_dir = plot_dir_pass
        log_dir = log_dir_pass

        if test['failed']:
            failed_tests.append(test_id)
            plot_dir = plot_dir_fail
            log_dir = log_dir_fail

        plot_filename = os.path.join(plot_dir, "%s.png" % (test_id))
        h_hat_filename = os.path.join(out_dir, test['id'] + "-h_hat.py")
        h_hat = get_h_hat(h_hat_filename)
        h_hat_ir = get_h_hat_impulse_response(h_hat, 0, 0)
        plot_impulseresponse_test(plot_filename,
                                  test['id'],
                                  test['echo'],
                                  h_hat_ir,
                                  test['headroom'],
                                  test['out_filename'],
                                  int(test['settle_time']))
        log_filename = os.path.join(log_dir, "%s.log" % (test_id))
        print("Log Filename: %s" % log_filename)
        with open(log_filename, 'w') as f:
            f.write(test['log'])
    return failed_tests


if __name__ == "__main__":
    tests = get_tests("results_check.xml")
    failed_tests = []
    total_tests = []
    test_types = ['simple', 'multitone', 'impulseresponse',
                  'smallimpulseresponse', 'excessive',
                  'bandlimited']
    for test_type in test_types and list(tests.keys()):
        total_tests += [test['id'] for test in tests[test_type]]
        if test_type in ['impulseresponse', 'smallimpulseresponse']:
            failures = parse_impulseresponse_tests(tests[test_type])
        else:
            failures = parse_simple_tests(tests[test_type])
        failed_tests += failures

    failed_tests_filename = os.path.join(results_dir, "failed_tests.txt")
    with open(failed_tests_filename, 'w') as f:
        tests = failed_tests
        tests.sort()
        for line in tests:
            f.write(line + '\n')

    total_tests_filename = os.path.join(results_dir, "all_tests.txt")
    total_tests = list(set(total_tests))
    total_tests.sort()
    with open(total_tests_filename, 'w') as f:
        for line in total_tests:
            f.write(line + '\n')
