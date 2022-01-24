# Copyright 2022 XMOS LIMITED.
# This Software is subject to the terms of the XMOS Public Licence: Version 1.
import subprocess
import sys
import argparse
from pathlib import Path

def parse_arguments():
    parser = argparse.ArgumentParser()
    parser.add_argument("--project-root", nargs='?', help="Project root directory")
    parser.add_argument("--source-file", nargs='?', help="source file.")
    parser.add_argument("--runner-file", nargs='?', help="runner file.")
    args = parser.parse_args()
    return args

if __name__ == "__main__":
    args = parse_arguments()

    print(f"in python: root {args.project_root}, source {args.source_file}, runner {args.runner_file}")

    runner_generator = Path(args.project_root) / 'Unity' / 'auto' / 'generate_test_runner.rb'

    try:
        subprocess.check_call(['ruby', runner_generator, args.source_file, args.runner_file])
    except OSError as e:
        print("Ruby generator failed\n\t{}".format(e), file=sys.stderr)
        exit(1)
