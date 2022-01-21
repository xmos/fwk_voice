# Copyright 2018-2021 XMOS LIMITED.
# This Software is subject to the terms of the XMOS Public Licence: Version 1.
import xscope_fileio
import argparse
import io
import sh
import os

def get_adapter_id():
    xrun_bin = "xrun" 
    xrun_timeout = 20
    try:
        xrun_out_buf = io.StringIO()
        xrun_out_proc = sh.Command(xrun_bin)(
            "-l", _bg=True, _bg_exc=False, _out=xrun_out_buf, _err_to_out=True
        )

        try:
            xrun_out_proc.wait(timeout=xrun_timeout)
        except sh.TimeoutException:
            # Kill the process group
            try:
                xrun_out_proc.kill_group()
                xrun_out_proc.wait()
            except sh.SignalException:
                # Killed
                pass
            # Raise an exception
            raise RuntimeError("Error: Call to xrun timed out")            

        xrun_out = xrun_out_buf.getvalue().split("\n")
    except sh.CommandNotFound:
        raise RuntimeError(f"Error: xrun command not found: {xrun_bin}")        
    
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
    args = parser.parse_args()
    return args

if __name__ == "__main__":
    args = parse_arguments()
    assert args.xe is not None, "Specify vaild .xe file"
    adapter_id = get_adapter_id()
    print("Running on adapter_id ",adapter_id)

    xscope_fileio.run_on_target(adapter_id, args.xe)



