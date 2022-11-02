# Copyright 2022 XMOS LIMITED.
# This Software is subject to the terms of the XMOS Public Licence: Version 1.


# CFFI ffibuilder.cdef function doesn't support #includes, so use the compiler pre-processor
# and a bit of scripting to extract the vnr state and xs3 math types from the source

import subprocess

xcore_math_types_api_dir = "../../../build/fwk_voice_deps/lib_xcore_math/lib_xcore_math/api"
lib_vnr_api_dir = "../../../modules/lib_vnr/api/features/"
lib_vnr_defines_dir = "../../../modules/lib_vnr/api/common/"
lib_vnr_inference_api_dir = "../../../modules/lib_vnr/api/inference/"
lib_vnr_inference_model_dir = "../../../modules/lib_vnr/src/inference/model"
lib_vnr_inference_src_dir = "../../../modules/lib_vnr/src/inference/"
vnr_state = []

def extract_section_vnr(line, pp):
    log_state = False
    if "vnr_features_state.h" in line:
        log_state = True
    elif "vnr_inference_state.h" in line:
        log_state = True

    while True:
        line = pp.readline()
        if line.startswith("#") or line == "":
            return line
        if line == "\n":
            continue
        if log_state:
            vnr_state.append(line)

def extract_xcore_math_vnr():
    with open(xcore_math_types_api_dir+"/xmath/types.h") as xs3m:
        lines = xs3m.readlines()
        for line in lines:
            if not "#" in line and "C_TYPE" not in line:
                if line == "\n":
                    continue
                vnr_state.append(line)

    # really hacky way to work-around CFFI's lack of support for `extern "C"`
    #  this is fragile because it assumes the extern "C" is on line #2.  And, that the 
    #  closing bracket is the last line.  However, this may not make the parsing of 
    #  the lib_xcore_math types.h file any more fragile.  The parsing can be broken by 
    #  subtle changes to the header.  
    EXTERN_C_LINE_NUM=2
    if vnr_state[EXTERN_C_LINE_NUM] == 'extern "C" { \n':
        del vnr_state[EXTERN_C_LINE_NUM]
        del vnr_state[-1]

def extract_pre_defs_vnr():
    #Grab xcore_math types
    extract_xcore_math_vnr()

    #Grab just vnr_feature_state related lines from the C pre-processed 
    subprocess.call(f"gcc -E vnr_test.c -o vnr_test.i -I {lib_vnr_api_dir} -I {lib_vnr_defines_dir} -I {lib_vnr_inference_api_dir} -I {lib_vnr_inference_model_dir} -I {lib_vnr_inference_src_dir} -I {xcore_math_types_api_dir}".split())

    with open("vnr_test.i") as pp:
        end_of_file = False
        line = pp.readline()
        line_number = 1
        while not end_of_file:
            if line == "":
                end_of_file = True
                break 
            if line.startswith("#"):
                line = extract_section_vnr(line, pp)
                continue
            line = pp.readline()

    return "".join(vnr_state)
 
if __name__ == "__main__":
    extract_pre_defs()
    for line in vnr_state:
        print(line, end="")
        
