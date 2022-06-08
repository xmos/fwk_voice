from xmos_ai_tools import xcore_tflm_host_interpreter as xtflm
from xmos_ai_tools import xformer as xf
import argparse

def parse_arguments():
    parser = argparse.ArgumentParser()
    parser.add_argument("xcore_opt_tflite_model", nargs='?',
                        help=".xe file to run")
    args = parser.parse_args()
    return args

# xmos-ai-tools version 0.1.8.dev93 has the fix for tensor_arena_size to indicate the actual scratch memory that is required by the inference engine
def run_xtflm_interpreter(xcore_opt_model):
    #xf.convert("model/model_output/model_qaware.tflite", "xcore_opt.tflite", params=None)
    #ie = xtflm.XTFLMInterpreter(model_path="xcore_opt.tflite")
    #print(f"Arena size = {ie.tensor_arena_size} bytes")
    ie = xtflm.XTFLMInterpreter(model_path=xcore_opt_model)
    ie.allocate_tensors()
    print(f"Tensor arena size = {ie.tensor_arena_size} bytes", dir(ie))

if __name__ == "__main__":
    args = parse_arguments()
    run_xtflm_interpreter(args.xcore_opt_tflite_model)
