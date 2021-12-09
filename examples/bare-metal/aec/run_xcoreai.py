# Copyright 2018-2021 XMOS LIMITED.
# This Software is subject to the terms of the XMOS Public Licence: Version 1.
import xtagctl
import xscope_fileio
import argparse

def parse_arguments():
    parser = argparse.ArgumentParser()
    parser.add_argument("xe", nargs='?',
                        help=".xe file to run")
    args = parser.parse_args()
    return args


args = parse_arguments()
assert args.xe is not None, "Specify vaild .xe file"

with xtagctl.acquire("XCORE-AI-EXPLORER") as adapter_id:
    xscope_fileio.run_on_target(adapter_id, args.xe)



