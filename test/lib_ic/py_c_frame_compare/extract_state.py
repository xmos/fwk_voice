# Copyright 2022 XMOS LIMITED.
# This Software is subject to the terms of the XMOS Public Licence: Version 1.


# CFFI ffibuilder.cdef function doesn't support #includes, so use the compiler pre-processor
# and a bit of scripting to extract the ic state and xs3 math types from the source

import subprocess

xcore_math_types_api_dir = "../../../../lib_xcore_math/lib_xcore_math/api"
lib_ic_api_dir = "../../../lib_voice/api/ic/"
lib_vnr_api_dir = "../../../lib_voice/api/vnr/"
lib_vnr_model_dir = "../../../lib_voice/src/vnr/model"
lib_vnr_src_dir = "../../../lib_voice/src/vnr/"
ic_state = []

def extract_section(line, pp):
    log_ic_state = False
    if  ("ic_state.h" in line) or ("vnr_state.h" in line):
        log_ic_state = True

    if log_ic_state:
        print("log_ic_state True for line = ",line)
    else:
        print("log_ic_state False for line = ",line)
    while True:
        line = pp.readline()
        if line.startswith("#") or line == "":
            return line
        if line == "\n":
            continue
        if log_ic_state:
            ic_state.append(line)

def extract_xcore_math():
    with open(xcore_math_types_api_dir+"/xmath/types.h") as xs3m:
        lines = xs3m.readlines()
        for line in lines:
            if not "#" in line and "C_TYPE" not in line:
                if line == "\n":
                    continue
                ic_state.append(line)

    # really hacky way to work-around CFFI's lack of support for `extern "C"`
    #  this is fragile because it assumes the extern "C" is on line #2.  And, that the
    #  closing bracket is the last line.  However, this may not make the parsing of
    #  the lib_xcore_math types.h file any more fragile.  The parsing can be broken by
    #  subtle changes to the header.
    EXTERN_C_LINE_NUM=2
    if 'extern "C"' in ic_state[EXTERN_C_LINE_NUM]:
        del ic_state[EXTERN_C_LINE_NUM]
        del ic_state[-1]

def extract_pre_defs():
    #Grab xcore_math types
    extract_xcore_math()

    #Grab just ic_state related lines from the C pre-processed
    subprocess.call(f"gcc -E ic_test.c -o ic_test.i -I {lib_ic_api_dir} -I {xcore_math_types_api_dir} -I {lib_vnr_api_dir} -I {lib_vnr_src_dir} -I {lib_vnr_model_dir}".split())

    with open("ic_test.i") as pp:
        end_of_file = False
        line = pp.readline()
        line_number = 1
        while not end_of_file:
            if line == "":
                end_of_file = True
                break
            if line.startswith("#"):
                line = extract_section(line, pp)
                continue
            line = pp.readline()

    return "".join(ic_state)

if __name__ == "__main__":
    extract_pre_defs()
    for line in ic_state:
        print(line, end="")

