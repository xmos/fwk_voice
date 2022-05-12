# Copyright 2022 XMOS LIMITED.
# This Software is subject to the terms of the XMOS Public Licence: Version 1.


# CFFI ffibuilder.cdef function doesn't support #includes, so use the compiler pre-processor
# and a bit of scripting to extract the vnr state and xs3 math types from the source

import subprocess

xs3_math_types_api_dir = "../../../build/avona_deps/lib_xs3_math/lib_xs3_math/api"
lib_vnr_api_dir = "../../../modules/lib_vnr/api/features/"
vnr_state = []

def extract_section_vnr(line, pp):
    log_state = True if "vnr_features_state.h" in line else False
    while True:
        line = pp.readline()
        if line.startswith("#") or line == "":
            return line
        if line == "\n":
            continue
        if log_state:
            vnr_state.append(line)

def extract_xs3_math_vnr():
    with open(xs3_math_types_api_dir+"/xs3_math_types.h") as xs3m:
        lines = xs3m.readlines()
        for line in lines:
            if not "#" in line and "C_TYPE" not in line:
                if line == "\n":
                    continue
                vnr_state.append(line)

def extract_pre_defs_vnr():
    #Grab xs3_math types
    extract_xs3_math_vnr()

    #Grab just vnr_feature_state related lines from the C pre-processed 
    subprocess.call(f"gcc -E vnr_test.c -o vnr_test.i -I {lib_vnr_api_dir} -I {xs3_math_types_api_dir}".split())

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
        
