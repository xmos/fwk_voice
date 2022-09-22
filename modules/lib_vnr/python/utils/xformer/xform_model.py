import argparse
import os.path
import subprocess
import tensorflow as tf
import numpy as np
import shutil
from xmos_ai_tools.xinterpreters import xcore_tflm_host_interpreter
from xmos_ai_tools import xformer as xf
import pkg_resources
import tempfile
import glob

this_filepath = os.path.dirname(os.path.abspath(__file__))

def parse_arguments():
    parser = argparse.ArgumentParser()
    parser.add_argument("tflite_model", nargs='?',
                        help="Unoptimised TensorFlow Lite model to optimise and integrate into the Avona VNR module")
    parser.add_argument("--copy-files", action='store_true', help="Copy generated files to vnr module")
    parser.add_argument("--module-path", type=str, default=None, help="Path to lib_vnr module to copy the new files to. Used with --copy-files")
    args = parser.parse_args()
    return args

def get_quant_spec(model):
    interpreter_tflite = tf.lite.Interpreter(model_path=str(model))
    input_details = interpreter_tflite.get_input_details()[0]
    output_details = interpreter_tflite.get_output_details()[0]    

    assert(input_details["dtype"] in [np.int8, np.uint8]),"Error: Only 8bit input supported"
    assert(output_details["dtype"] in [np.int8, np.uint8]),"Error: Only 8bit output supported"

    # quantization spec
    input_scale, input_zero_point = input_details["quantization"]
    output_scale, output_zero_point = output_details["quantization"]
    return input_scale, input_zero_point, output_scale, output_zero_point

if __name__ == "__main__":
    args = parse_arguments()
    model = args.tflite_model
    model = os.path.abspath(model)
    ai_tools_version = pkg_resources.get_distribution('xmos_ai_tools').version
    print(f"Running for input model {model}. Using xmos-ai-tools version {ai_tools_version}")

    tflite_model_dir = os.path.dirname(os.path.abspath(model))

    test_dir = tempfile.mkdtemp(prefix=f"convert_{os.path.basename(model).split('.')[0]}_", dir=".")
    print(f"Keeping a copy of generated files in {test_dir} directory")

    # Tflite to xcore optimised tflite micro
    xcore_opt_model = os.path.abspath(os.path.join(test_dir, os.path.basename(model).split('.')[0] + "_xcore.tflite"))
    #xf.print_help()
    #xf.convert(f"{model}", f"{xcore_opt_model}", {"mlir-disable-threading": None, "xcore-reduce-memory": None})
    convert_cmd = f"xcore-opt --xcore-thread-count 1 -o {xcore_opt_model} {model}".split()
    subprocess.run(convert_cmd, check=True)
    xcore_opt_model_size = os.path.getsize(xcore_opt_model)

    # Run tflite_micro_compiler to get the cpp file that can be compiled as part of the VNR module
    compiled_cpp_file = os.path.abspath(os.path.join(test_dir, os.path.basename(xcore_opt_model).split('.')[0] + ".cpp"))
    # Build the tflite_micro_compiler
    # Check if the directory exists
    lib_tflite_micro_path = os.path.join(this_filepath, "../../../../../build/fwk_voice_deps/lib_tflite_micro")
    assert os.path.isdir(lib_tflite_micro_path), f"ERROR: lib_tflite_micro doesn't exist in {os.path.abspath(lib_tflite_micro_path)}. Run the fwk_voice cmake command to ensure lib_tflite_repo is fetched."
    # Run a make build in the lib_tflite_micro directory which will build the tflite_micro_compiler
    save_dir = os.getcwd()
    os.chdir(lib_tflite_micro_path)
    build_cmd = "make build".split()
    #subprocess.run(build_cmd, check=True)
    # Run the tflite_micro_compiler to generate the micro compiled .cpp file from the optimised tflite model
    
    tflite_micro_compiler_exe = os.path.abspath("tflite_micro_compiler/build/tflite_micro_compiler")
    tflite_micro_compiler_cmd = f"{tflite_micro_compiler_exe} {xcore_opt_model} {compiled_cpp_file}".split()
    subprocess.run(tflite_micro_compiler_cmd, check=True)
    os.chdir(save_dir)


    # Convert tflite micro to .c and .h files that lib_vnr can compile with
    model_c_file = os.path.join(test_dir, "vnr_model_data.c")
    model_h_file = os.path.join(test_dir, "vnr_model_data.h")
    tflite_to_c_script = os.path.join(this_filepath, "convert_tflite_to_c_source.py")
    cmd = f"{tflite_to_c_script} --input {xcore_opt_model} --header {model_h_file} --source {model_c_file} --variable-name vnr".split()
    subprocess.run(cmd, check=True)
    ## Double check model size against vnr_model_data_len printed in the vnr_model_data.c
    with open(model_c_file, "r") as fp:
        lines = fp.read().splitlines()
        for l in lines:
            if "vnr_model_data_len" in l:
                model_data_len = (l.split()[-1])[:-1] #TODO use re
                assert(xcore_opt_model_size == int(model_data_len)), "model_data_len doesn't match xcore_opt tflite file size"

    # Create Tensor arena size define file
    ie = xcore_tflm_host_interpreter()
    ie.set_model(model_path=xcore_opt_model, secondary_memory = False, flash = False)
    print(f"Tensor arena size = {ie.tensor_arena_size()} bytes")

    str_index = os.path.realpath(__file__).find('fwk_voice/')
    assert(str_index != -1)
    with open(os.path.join(test_dir, "vnr_tensor_arena_size.h"), "w") as fp:
        fp.write(f"// Autogenerated from {os.path.realpath(__file__)[str_index:]}. Do not modify\n")
        fp.write(f"// Generated using xmos-ai-tools version {ai_tools_version}\n")
        fp.write("#ifndef VNR_TENSOR_ARENA_SIZE_H\n")
        fp.write("#define VNR_TENSOR_ARENA_SIZE_H\n\n")
        fp.write(f"#define TENSOR_ARENA_SIZE_BYTES    ({ie.tensor_arena_size()} - {xcore_opt_model_size}) // Remove model size from the tensor_arena_size returned by the python xcore_tflm_host_interpreter \n")
        fp.write("\n#endif")
    
    # Create Quant dequant spec defines file
    input_scale, input_zero_point, output_scale, output_zero_point = get_quant_spec(model)
    print(f"input_scale {input_scale}, input_zero_point {input_zero_point}, output_scale {output_scale}, output_zero_point {output_zero_point}")
    with open(os.path.join(test_dir, "vnr_quant_spec_defines.h"), "w") as fp:
        fp.write(f"// Autogenerated from {os.path.realpath(__file__)[str_index:]}. Do not modify\n")
        fp.write(f"// Generated using xmos-ai-tools version {ai_tools_version}\n")
        fp.write("#ifndef VNR_QUANT_SPEC_DEFINES_H\n")
        fp.write("#define VNR_QUANT_SPEC_DEFINES_H\n\n")
        fp.write(f"#define VNR_INPUT_SCALE_INV    (1.0/{input_scale})\n")
        fp.write(f"#define VNR_INPUT_ZERO_POINT   ({input_zero_point})\n")
        fp.write(f"#define VNR_OUTPUT_SCALE       ({output_scale})\n")
        fp.write(f"#define VNR_OUTPUT_ZERO_POINT   ({output_zero_point})\n")
        fp.write("\n#endif")
    
    # Optionally, copy generated files into the VNR module
    if args.copy_files:
        files_to_add = [os.path.basename(model_c_file), os.path.basename(model_h_file), os.path.basename(xcore_opt_model), "vnr_tensor_arena_size.h", "vnr_quant_spec_defines.h"]
        files_to_delete = []
        assert(args.module_path != None), "VNR module path --module-path needs to be specified when running with --copy-files"
        vnr_module_path = os.path.abspath(args.module_path)
        current_files = glob.glob(f"{vnr_module_path}/*") # Files currently in vnr_module_path
        for f in current_files:
            if not os.path.basename(f) in files_to_add:
                files_to_delete.append(f)
        
        print("files to delete\n",files_to_delete)
        print("files to copy\n", files_to_add)
        
        # List of files that will be copied
        # Check if any files from vnr_module_path would need deleting
        
        print(f"WARNING: Copying files to lib_vnr module {vnr_module_path}. Verify before committing!")
        # Copy converted model .c and .h files
        shutil.copy2(model_c_file, vnr_module_path)
        shutil.copy2(model_h_file, vnr_module_path)
        # Copy tensor arena size defines file
        shutil.copy2(os.path.join(test_dir, "vnr_tensor_arena_size.h"), vnr_module_path)
        # Copy quant dequant spec defines file
        shutil.copy2(os.path.join(test_dir, "vnr_quant_spec_defines.h"), vnr_module_path)
        # Copy xcore opt model tflite file to the model's directory
        shutil.copy2(xcore_opt_model, vnr_module_path)
        # Optionally do a git add and git rm as well??

        print(f"Adding new files and removing old files in {vnr_module_path} from GIT now")
        
        
        
        



            

