# Copyright 2022 XMOS LIMITED.
# This Software is subject to the terms of the XMOS Public Licence: Version 1.


# CFFI ffibuilder.cdef function doesn't support #includes, so use the compiler pre-processor
# and a bit of scripting to extract the ic state and xs3 math types from the source

import subprocess

xs3_math_types_api_dir = "../../build/avona_deps/lib_xs3_math/lib_xs3_math/api"
lib_ic_api_dir = "../../modules/lib_ic/api/"
lib_ic_src_dir = "../../modules/lib_ic/src/"
lib_vad_api_dir = "../../modules/lib_vad/api/"
lib_vad_src_dir = "../../modules/lib_vad/src/"
lib_vnr_api_dir = "../../modules/lib_vnr/api/features/"
lib_vnr_inference_api_dir = "../../modules/lib_vnr/api/inference/"
lib_vnr_inference_model_dir = "../../modules/lib_vnr/src/inference/model"
lib_vnr_inference_src_dir = "../../modules/lib_vnr/src/inference/"
state = []

def extract_section(line, pp, filenames):
    log_state = False
    for filename in filenames:
        if filename in line:
            log_state = True
    while True:
        line = pp.readline()
        if line.startswith("#") or line == "":
            return line
        if line == "\n":
            continue
        if log_state:
            state.append(line)

def extract_xs3_math():
    #This is a really quick and dirty way to get things to play nicely. CFFI should do this for us but doesn't
    #Note enums do not go down well so we replace that with int late
    skip_lines_containing = ["#", "C_TYPE", "enum", "BFP_FLAG_DYNAMIC", "bfp_flags_e;"]
    with open(xs3_math_types_api_dir+"/xs3_math_types.h") as xs3m:
        lines = xs3m.readlines()
        for line in lines:
            line_ok = True
            for skip in skip_lines_containing:
                if skip in line:
                    line_ok = False
            if line_ok and not line == "\n":
                state.append(line)


def extract_pre_defs():
    #Grab xs3_math types
    extract_xs3_math()

    #Grab just ic_state related lines from the C pre-processed 
    subprocess.call(f"gcc -E ic_vad_test.c -o ic_vad_test.i -I {lib_ic_api_dir} -I {lib_ic_src_dir} -I {lib_vad_api_dir} -I {lib_vad_src_dir} -I {xs3_math_types_api_dir} -I {lib_vnr_api_dir} -I {lib_vnr_inference_api_dir} -I {lib_vnr_inference_model_dir} -I {lib_vnr_inference_src_dir}".split())

    with open("ic_vad_test.i") as pp:
        end_of_file = False
        line = pp.readline()
        line_number = 1
        while not end_of_file:
            if line == "":
                end_of_file = True
                break 
            if line.startswith("#"):
                line = extract_section(line, pp, ["ic_state.h", "vnr_features_state.h", "vnr_inference_state.h"])
                continue
            line = pp.readline()

    return "".join(state)

if __name__ == "__main__":
    extract_pre_defs()
    for line in state:
        print(line, end="")
