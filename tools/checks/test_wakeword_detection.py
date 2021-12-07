# This parses a log file with specific output to produce a ratio
# of successful wakeword recognition.
# This file may be ran with pytest. For verbose success output, add the -rP
# argument.
# Running this with python will parse the log to find the success ratio.

import re

# Examine this log file
default_log_file = "check_wakeword_detection.log"

# These are log-specific criteria for use in tallying
regex_test = "Wakeword Check: "
regex_detected = "Detected:"

# Initialize variables
test_count = 0
success_count = 0

# Iterate over the log file to tally tests and successes
with open(default_log_file, "r") as f:
    for line in f:
        for match in re.finditer(regex_test, line, re.S):
            # print line.replace(regex_test, "")
            test_count += 1
        for match in re.finditer(regex_detected, line, re.S):
            success_count += 1
    print(f"{success_count} out of {test_count} passed.")

# This is called automatically by running pytest in the same directory.
def func_report():
    return success_count


def test_success():
    # define the pass threshold (60%)
    assert func_report() >= test_count * 0.6
    print(f"{success_count} out of {test_count} passed.")
