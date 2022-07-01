# Copyright 2022 XMOS LIMITED.
# This Software is subject to the terms of the XMOS Public Licence: Version 1.


# CFFI ffibuilder.cdef function doesn't support #includes, so use the compiler pre-processor
# and a bit of scripting to extract the ic state and xs3 math types from the source

import subprocess

xs3_math_types_api_dir = "../../../build/fwk_voice_deps/lib_xs3_math/lib_xs3_math/api"
lib_ic_api_dir = "../../../modules/lib_ic/api/"
lib_vnr_api_dir = "../../../modules/lib_vnr/api/features/"
lib_vnr_inference_api_dir = "../../../modules/lib_vnr/api/inference/"
lib_vnr_inference_model_dir = "../../../modules/lib_vnr/src/inference/model"
lib_vnr_inference_src_dir = "../../../modules/lib_vnr/src/inference/"
ic_state = []

def extract_section(line, pp):
    #log_ic_state = True if ("ic_state.h" or "vnr_features_state.h" or "vnr_inference_state.h") in line else False
    log_ic_state = False
    if  ("ic_state.h" in line) or ("vnr_features_state.h" in line) or ("vnr_inference_state.h" in line):
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

def extract_xs3_math():
    with open(xs3_math_types_api_dir+"/xs3_math_types.h") as xs3m:
        lines = xs3m.readlines()
        for line in lines:
            if not "#" in line and "C_TYPE" not in line:
                if line == "\n":
                    continue
                ic_state.append(line)

def extract_pre_defs():
    #Grab xs3_math types
    extract_xs3_math()

    #Grab just ic_state related lines from the C pre-processed 
    subprocess.call(f"gcc -E ic_test.c -o ic_test.i -I {lib_ic_api_dir} -I {xs3_math_types_api_dir} -I {lib_vnr_api_dir} -I {lib_vnr_inference_api_dir} -I {lib_vnr_inference_model_dir} -I {lib_vnr_inference_src_dir}".split())

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
        
