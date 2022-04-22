import py_vnr.run_wav_vnr as rwv
import py_vnr.vnr as vnr
import data_processing.frame_preprocessor as fp
import argparse
import audio_wav_utils as awu
import scipy.io.wavfile
import numpy as np
import tensorflow as tf
import matplotlib.pyplot as plt
import os
import sys
sys.path.append(os.path.join(os.getcwd(), "../shared_src/python"))
import run_xcoreai

def parse_arguments():
    parser = argparse.ArgumentParser()
    parser.add_argument("tflite_model", nargs='?', help=".tflite_model")
    args = parser.parse_args()
    return args

def quantise_patch(interpreter_tflite, input_patches):
    interpreter_tflite.allocate_tensors()

    # Get input and output tensors.
    input_details = interpreter_tflite.get_input_details()[0]
    output_details = interpreter_tflite.get_output_details()[0]    

    # quantization spec
    if input_details["dtype"] in [np.int8, np.uint8]:
        input_scale, input_zero_point = input_details["quantization"]
        #print("input_scale = ", input_scale)
        #print("input_zero_point = ", input_zero_point)
        input_patches = input_patches / input_scale + input_zero_point
        input_patches_8b = np.round(input_patches)
        input_patches_8b = input_patches_8b.astype(input_details["dtype"])
    else:
        assert(False),"Error: Only 8bit input supported"

    return input_patches_8b

def run_tflite_inference(interpreter_tflite, input_patches):
    interpreter_tflite.allocate_tensors()

    # Get input and output tensors.
    input_details = interpreter_tflite.get_input_details()[0]
    output_details = interpreter_tflite.get_output_details()[0]    
    predict_tflite = np.zeros(input_patches.shape[0])
    for n in range(len(predict_tflite)):
        this_patch = input_patches[n:n+1]

        # set the input tensor to a test_audio slice
        interpreter_tflite.set_tensor(input_details['index'], this_patch)

        # run the model
        interpreter_tflite.invoke()

        # get output
        output_data = interpreter_tflite.get_tensor(output_details['index'])
        predict_tflite[n] = output_data[0, 0]
    return predict_tflite[0]

def dequantise_tflite_output(interpreter_tflite, output_data):
    interpreter_tflite.allocate_tensors()
    output_details = interpreter_tflite.get_output_details()[0]    
    if output_details["dtype"] in [np.int8, np.uint8]:
        output_scale, output_zero_point = output_details["quantization"]
        #print("output_scale = ", output_scale)
        #print("output_zero_point = ", output_zero_point)
        output_data_float = output_data.astype(np.float64)
        output_data_float = (output_data_float - output_zero_point)*output_scale
        return output_data_float
    else:
        assert(False),"Error: Only 8bit output supported"


def tflite_predict(interpreter_tflite, input_patches):
    interpreter_tflite.allocate_tensors()

    # Get input and output tensors.
    input_details = interpreter_tflite.get_input_details()[0]
    output_details = interpreter_tflite.get_output_details()[0]    

    # quantization spec
    if input_details["dtype"] in [np.int8, np.uint8]:
        input_scale, input_zero_point = input_details["quantization"]
        #print("input_scale = ", input_scale)
        #print("input_zero_point = ", input_zero_point)
    else:
        assert(False),"Error: Only 8bit input supported"
    if output_details["dtype"] in [np.int8, np.uint8]:
        output_scale, output_zero_point = output_details["quantization"]
        #print("output_scale = ", output_scale)
        #print("output_zero_point = ", output_zero_point)
    else:
        assert(False),"Error: Only 8bit output supported"

    predict_tflite = np.zeros(input_patches.shape[0])

    for n in range(len(predict_tflite)):
        this_patch = input_patches[n:n+1]
        #print("this_patch.shape = ", this_patch.shape)

        # quantize and set type as required 
        if input_details['dtype'] in [np.int8, np.uint8]:            
            this_patch = this_patch / input_scale + input_zero_point
            this_patch = np.round(this_patch)
        this_patch = this_patch.astype(input_details["dtype"])

        # set the input tensor to a test_audio slice
        interpreter_tflite.set_tensor(input_details['index'], this_patch)

        # run the model
        interpreter_tflite.invoke()

        # get output
        output_data = interpreter_tflite.get_tensor(output_details['index'])

        # dequantize output as required
        if output_details['dtype'] in [np.int8, np.uint8]:
            output_data_float = output_data.astype(np.float64)
            output_data_float = (output_data_float - output_zero_point)*output_scale
        predict_tflite[n] = output_data_float[0, 0]
    return predict_tflite[0], output_data[0,0], this_patch


def test_mel(tflite_model, plot_results=False):
    run_xcoreai.run("../../../build/examples/bare-metal/test_mel/bin/avona_test_mel.xe")
    
    interpreter_tflite = tf.lite.Interpreter(
        model_path=str(tflite_model))
    
    vnr_obj = vnr.Vnr(model_file=None)    
    dtype=np.dtype([('r', np.int32),('q', np.int32)])
    # read dut fft output that will be used as input to the python mel
    with open("dut_fft.bin", "rb") as fdut:        
        dut_fft_out = np.fromfile(fdut, dtype=dtype)
        #print("dut_fft_out from file", dut_fft_out[0])
        dut_fft_out = [complex(*item) for item in dut_fft_out]
        dut_fft_out = np.array(dut_fft_out, dtype=np.complex128)
    # Read dut FFT output exponents
    with open("dut_fft_exp.bin", "rb") as fdut:
        dut_fft_out_exp = np.fromfile(fdut, dtype=np.int32)

    # read dut mel output
    with open("dut_mel.bin", "rb") as fdut:        
        dut_mel_mant = np.fromfile(fdut, dtype=np.int32)
        dut_mel_mant = np.array(dut_mel_mant, dtype=np.float64)
        with open("dut_mel_exp.bin", "rb") as fdutexp:        
            dut_mel_exp = np.fromfile(fdutexp, dtype=np.int32)
            dut_mel = np.array(dut_mel_mant*(float(2)**dut_mel_exp), dtype=np.float64)
    
    with open("mel_log2.bin", "rb") as fdut:
        dut_mel_log2 = np.fromfile(fdut, dtype=np.int32)
        dut_mel_log2 = np.array(dut_mel_log2, dtype=np.float64)
        dut_mel_log2 = dut_mel_log2 * (float(2)**-24) #DUT mel log2 is always in 8.24 format 

    with open("quant_patch.bin", "rb") as fdut:
        dut_quant_patch = np.fromfile(fdut, dtype=np.int8)

    # Convert FFT output to float
    frames = len(dut_fft_out_exp)
    print(f"{frames} frames sent to python")

    ref_X_spect_full = np.empty(0, dtype=np.complex128)
    ref_mel = np.empty(0, dtype=np.float64)
    ref_mel_log2 = np.empty(0, dtype=np.float64)
    ref_quant_patch = np.empty(0, dtype=np.int8)
    ref_tflite_output = np.empty(0, dtype=np.float64)
    for fr in range(frames):
        exp = dut_fft_out_exp[fr]
        X_spect = np.array(dut_fft_out[fr*257:(fr+1)*257] * (float(2)**exp))
        ref_X_spect_full = np.append(ref_X_spect_full, X_spect)

        out_spect = np.abs(X_spect)**2
        #if(fr == 66):
        #    print("abs_spect\n",out_spect[:20])
        out_spect = np.dot(out_spect, vnr_obj.mel_fbank)        
        ref_mel = np.append(ref_mel, out_spect)
        out_spect = np.log2(out_spect)        
        ref_mel_log2 = np.append(ref_mel_log2, out_spect)
        new_slice = out_spect
        vnr_obj.add_new_slice(new_slice, buffer_number=0)
        normalised_patch = vnr_obj.normalise_patch(vnr_obj.feature_buffers[0])
        #print("python normalised_patch\n",normalised_patch)
        quantised_patch_8b = quantise_patch(interpreter_tflite, normalised_patch)
        ref_quant_patch = np.append(ref_quant_patch, quantised_patch_8b)
        ref_tflite_output_quant = run_tflite_inference(interpreter_tflite, quantised_patch_8b)
        ref_tflite_output = np.append(ref_tflite_output, dequantise_tflite_output(interpreter_tflite, ref_tflite_output_quant)) 
        #ref_tflite_output = np.append(ref_tflite_output, tflite_output)
        #print("python quantised_patch\n",quantised_patch)
    
    #DUT features + ref tflite inference
    quant_patch_len = vnr_obj.mel_filters*fp.PATCH_WIDTH
    dut_tflite_output = np.empty(0, dtype=np.float64)
    for i in range(0, len(dut_quant_patch), quant_patch_len):
        patch = dut_quant_patch[i:i+quant_patch_len]
        patch = patch.reshape((1,1,4,24))
        dut_tflite_output_quant = run_tflite_inference(interpreter_tflite, patch)
        dut_tflite_output = np.append(dut_tflite_output, dequantise_tflite_output(interpreter_tflite, dut_tflite_output_quant)) 

    max_mel_diff_per_frame = np.empty(0, dtype=np.float64) 
    max_mel_log2_diff_per_frame = np.empty(0, dtype=np.float64) 
    max_quant_patch_diff_per_frame = np.empty(0, dtype=np.int8)
    for i in range(0, len(ref_mel), vnr_obj.mel_filters):
        fr = i/vnr_obj.mel_filters
        ref = ref_mel[i:i+vnr_obj.mel_filters]
        dut = dut_mel[i:i+vnr_obj.mel_filters]
        max_mel_diff_per_frame = np.append(max_mel_diff_per_frame, np.max(np.abs(ref-dut)))
        ref = ref_mel_log2[i:i+vnr_obj.mel_filters]
        dut = dut_mel_log2[i:i+vnr_obj.mel_filters]
        max_mel_log2_diff_per_frame = np.append(max_mel_log2_diff_per_frame, np.max(np.abs(ref-dut)))

        #if fr == 66:
        #    print("ref mel\n",ref)
        #    print("dut mel\n",dut)
        #    print(f"frame {i} diff {max(abs(ref-dut))}, max_ref {max(abs(ref))}, max_dut {max(abs(dut))}")
    
    for i in range(0, len(ref_quant_patch), quant_patch_len):
        dut = dut_quant_patch[i:i+quant_patch_len]
        ref = ref_quant_patch[i:i+quant_patch_len]
        max_quant_patch_diff_per_frame = np.append(max_quant_patch_diff_per_frame, np.max(np.abs(ref-dut))) 

    print("Max mel diff across all frames = ",np.max(max_mel_diff_per_frame)) 
    print("Max mel log2 diff across all frames = ",np.max(max_mel_log2_diff_per_frame)) 
    print("Max quant_patch diff across all frames = ",np.max(max_quant_patch_diff_per_frame)) 
    print("Max ref-dut tflite output diff = ",np.max(np.abs(ref_tflite_output - dut_tflite_output))) 
    if(plot_results):
        fig,ax = plt.subplots(3,2)
        fig.set_size_inches(20,10)
        ax[0,0].set_title('mel output')
        ax[0,0].plot(ref_mel)
        ax[0,0].plot(dut_mel)
        ax[0,1].set_title('max mel output diff per frame')
        ax[0,1].plot(max_mel_diff_per_frame)
        ax[1,0].set_title('max mel log2 diff per frame')
        ax[1,0].plot(max_mel_log2_diff_per_frame)
        ax[2,0].set_title('ref tflite output')
        ax[2,0].plot(ref_tflite_output)
        ax[2,1].set_title('dut tflite output')
        ax[2,0].plot(ref_tflite_output)
        ax[2,1].plot(dut_tflite_output)
        fig_instance = plt.gcf() #get current figure instance so we can save in a file later
        plt.show()
        plot_file = "ref_dut_mel_compare.png"
        fig.savefig(plot_file)
        
if __name__ == "__main__":
    args = parse_arguments()
    test_mel(args.tflite_model, plot_results=True)
