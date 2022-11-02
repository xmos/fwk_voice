# Copyright 2022 XMOS LIMITED.
# This Software is subject to the terms of the XMOS Public Licence: Version 1.


# CFFI ffibuilder.cdef function doesn't support #includes, so use the compiler pre-processor
# and a bit of scripting to extract the ic state and xs3 math types from the source

import subprocess

xcore_math_types_api_dir = "../../build/fwk_voice_deps/lib_xcore_math/lib_xcore_math/api"
lib_ic_api_dir = "../../modules/lib_ic/api/"
lib_ic_src_dir = "../../modules/lib_ic/src/"
lib_vnr_api_common_dir = "../../modules/lib_vnr/api/common/"
lib_vnr_api_dir = "../../modules/lib_vnr/api/features/"
lib_vnr_inference_api_dir = "../../modules/lib_vnr/api/inference/"
lib_vnr_inference_model_dir = "../../modules/lib_vnr/src/inference/model"
lib_vnr_inference_src_dir = "../../modules/lib_vnr/src/inference/"
calc_vnr_pred_dir = "../../examples/bare-metal/shared_src/calc_vnr_pred/src"
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

def extract_xcore_math():
    #This is a really quick and dirty way to get things to play nicely. CFFI should do this for us but doesn't
    #Note enums do not go down well so we replace that with int late
    skip_lines_containing = ["#", "C_TYPE", "enum", "BFP_FLAG_DYNAMIC", "bfp_flags_e;"]
    with open(xcore_math_types_api_dir+"/xmath/types.h") as xs3m:
        lines = xs3m.readlines()
        for line in lines:
            line_ok = True
            for skip in skip_lines_containing:
                if skip in line:
                    line_ok = False
            if line_ok and not line == "\n":
                state.append(line)

    # really hacky way to work-around CFFI's lack of support for `extern "C"`
    #  this is fragile because it assumes the extern "C" is on line #2.  And, that the 
    #  closing bracket is the last line.  However, this may not make the parsing of 
    #  the lib_xcore_math types.h file any more fragile.  The parsing can be broken by 
    #  subtle changes to the header.  
    EXTERN_C_LINE_NUM=2
    if state[EXTERN_C_LINE_NUM] == 'extern "C" { \n':
        del state[EXTERN_C_LINE_NUM]
        del state[-1]

def extract_pre_defs():
    #Grab xcore_math types
    extract_xcore_math()

    #Grab just ic_state related lines from the C pre-processed 
    subprocess.call(f"gcc -E ic_vnr_test.c -o ic_vnr_test.i -I {lib_ic_api_dir} -I {lib_ic_src_dir} -I {xcore_math_types_api_dir} -I {lib_vnr_api_common_dir} -I {lib_vnr_api_dir} -I {lib_vnr_inference_api_dir} -I {lib_vnr_inference_model_dir} -I {lib_vnr_inference_src_dir} -I {calc_vnr_pred_dir}".split())

    with open("ic_vnr_test.i") as pp:
        end_of_file = False
        line = pp.readline()
        line_number = 1
        while not end_of_file:
            if line == "":
                end_of_file = True
                break 
            if line.startswith("#"):
                line = extract_section(line, pp, ["ic_state.h", "vnr_features_state.h", "vnr_inference_state.h", "calc_vnr_pred.h"])
                continue
            line = pp.readline()

    return "".join(state)

if __name__ == "__main__":
    extract_pre_defs()
    for line in state:
        print(line, end="")
