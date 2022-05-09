from xmos_ai_tools import xcore_tflm_host_interpreter as xtflm
from xmos_ai_tools import xformer as xf
import argparse

def parse_arguments():
    parser = argparse.ArgumentParser()
    parser.add_argument("xcore_opt_tflite_model", nargs='?',
                        help=".xe file to run")
    args = parser.parse_args()
    return args


def run_xtflm_interpreter(xcore_opt_model):
    #xf.convert("model/model_output_0_0_2/model_qaware.tflite", "xcore_opt.tflite", params=None)
    #ie = xtflm.XTFLMInterpreter(model_path="xcore_opt.tflite")
    #print(f"Arena size = {ie.tensor_arena_size} bytes")
    ie = xtflm.XTFLMInterpreter(model_path=xcore_opt_model)
    ie.allocate_tensors()
    print(f"Tensor arena size = {ie.tensor_arena_size} bytes", dir(ie))

if __name__ == "__main__":
    args = parse_arguments()
    run_xtflm_interpreter(args.xcore_opt_tflite_model)
