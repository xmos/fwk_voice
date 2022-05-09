# Copyright 2022 XMOS LIMITED.
# This Software is subject to the terms of the XMOS Public Licence: Version 1.
import xscope_fileio
import argparse
import shutil
import subprocess

def get_adapter_id():
    try:
        xrun_out = subprocess.check_output(['xrun', '-l'], text=True, stderr=subprocess.STDOUT)
    except subprocess.CalledProcessError as e:
        print('Error: %s' % e.output)
        assert False

    xrun_out = xrun_out.split('\n')
    # Check that the first 4 lines of xrun_out match the expected lines
    expected_header = ["", "Available XMOS Devices", "----------------------", ""]
    if len(xrun_out) < len(expected_header):
        raise RuntimeError(
            f"Error: xrun output:\n{xrun_out}\n"
            f"does not contain expected header:\n{expected_header}"
        )

    header_match = True
    for i, expected_line in enumerate(expected_header):
        if xrun_out[i] != expected_line:
            header_match = False

    if not header_match:
        raise RuntimeError(
            f"Error: xrun output header:\n{xrun_out[:4]}\n"
            f"does not match expected header:\n{expected_header}"
        )

    try:
        if "No Available Devices Found" in xrun_out[4]:
            raise RuntimeError(f"Error: No available devices found\n")
            return
    except IndexError:
        raise RuntimeError(f"Error: xrun output is too short:\n{xrun_out}\n")

    for line in xrun_out[6:]:
        if line.strip():
            adapterID = line[26:34].strip()
            status = line[34:].strip()
        else:
            continue
    print("adapter_id = ",adapterID)
    return adapterID


def parse_arguments():
    parser = argparse.ArgumentParser()
    parser.add_argument("xe", nargs='?',
                        help=".xe file to run")
    parser.add_argument('--input', type=str, default="input.wav",
                        help="input wav file. Default: input.wav")
    args = parser.parse_args()
    return args

def run(xe, input_file, return_stdout=False):
    try:
        shutil.copy2(input_file, "input.wav")
    except shutil.SameFileError as e:
        pass
    except IOError as e:
         print('Error: %s' % e.strerror)
         assert False, "Invalid input file"
    adapter_id = get_adapter_id()
    print("Running on adapter_id ",adapter_id)
    if return_stdout == False:
        xscope_fileio.run_on_target(adapter_id, xe)
    else:
        with open("prof.txt", "w+") as ff:
            xscope_fileio.run_on_target(adapter_id, xe, stdout=ff)
            ff.seek(0)
            stdout = ff.readlines()
        return stdout

if __name__ == "__main__":
    args = parse_arguments()
    assert args.xe is not None, "Specify vaild .xe file"
    print(f"args.input = {args.input}")
         
    run(args.xe, args.input)

