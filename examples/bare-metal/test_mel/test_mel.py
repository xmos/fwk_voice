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
import audio_wav_utils as awu
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
    
    # read dut output from various files
    with open("mel_log2.bin", "rb") as fdut:
        dut_mel_log2 = np.fromfile(fdut, dtype=np.int32)
        dut_mel_log2 = np.array(dut_mel_log2, dtype=np.float64)
        dut_mel_log2 = dut_mel_log2 * (float(2)**-24) #DUT mel log2 is always in 8.24 format 

    with open("quant_patch.bin", "rb") as fdut:
        dut_quant_patch = np.fromfile(fdut, dtype=np.int8)

    with open("dequant_output.bin", "rb") as fdut:
        dut_output = np.fromfile(fdut, dtype=np.int32)
        mant = np.array(dut_output[0::2], dtype=np.float64)
        exp = dut_output[1::2]
        dut_tflite_output = mant * (float(2)**exp)
    
    #################################################################################
    # Reference feature extraction and inference
    #The number of samples of data in the frame
    proc_frame_length = 2**9
    frame_advance = 240
    frame_buffer = np.zeros(3*proc_frame_length)
    vnr_obj = vnr.Vnr(model_file=None)    
    rate, wav_file = scipy.io.wavfile.read("data_16k/2035-152373-0002001.wav", 'r')
    wav_data, channel_count, file_length = awu.parse_audio(wav_file)
    
    vnr_output = np.zeros(file_length // frame_advance)
    x_data = np.zeros(proc_frame_length)

    ref_mel = np.empty(0, dtype=np.float64)
    ref_mel_log2 = np.empty(0, dtype=np.float64)
    ref_quant_patch = np.empty(0, dtype=np.int8)
    ref_tflite_output = np.empty(0, dtype=np.float64)
    framecount = 0;
    for frame_start in range(0, file_length-frame_advance, frame_advance):
        # buffer the input data into STFT slices
        new_x_frame = awu.get_frame(wav_data, frame_start, frame_advance)
        x_data = np.roll(x_data, -frame_advance, axis = 0)
        x_data[proc_frame_length - frame_advance:] = new_x_frame
        X_spect = np.fft.rfft(x_data, vnr_obj.nfft)

        out_spect = vnr_obj.make_slice(X_spect)
        ref_mel_log2 = np.append(ref_mel_log2, out_spect)

        vnr_obj.add_new_slice(out_spect, buffer_number=0)
        normalised_patch = vnr_obj.normalise_patch(vnr_obj.feature_buffers[0])
        #print("python normalised_patch\n",normalised_patch)
        quantised_patch_8b = quantise_patch(interpreter_tflite, normalised_patch)
        ref_quant_patch = np.append(ref_quant_patch, quantised_patch_8b)
        ref_tflite_output_quant = run_tflite_inference(interpreter_tflite, quantised_patch_8b)
        ref_tflite_output = np.append(ref_tflite_output, dequantise_tflite_output(interpreter_tflite, ref_tflite_output_quant))
        framecount = framecount + 1
    
    #################################################################################

    #DUT features + ref tflite inference
    quant_patch_len = vnr_obj.mel_filters*fp.PATCH_WIDTH
    dut_features_ref_tflite_output = np.empty(0, dtype=np.float64)
    for i in range(0, len(dut_quant_patch), quant_patch_len):
        patch = dut_quant_patch[i:i+quant_patch_len]
        patch = patch.reshape((1,1,4,24))
        dut_tflite_output_quant = run_tflite_inference(interpreter_tflite, patch)
        dut_features_ref_tflite_output = np.append(dut_features_ref_tflite_output, dequantise_tflite_output(interpreter_tflite, dut_tflite_output_quant)) 

    max_mel_log2_diff_per_frame = np.empty(0, dtype=np.float64) 
    max_quant_patch_diff_per_frame = np.empty(0, dtype=np.int8)
    for i in range(0, len(ref_mel_log2), vnr_obj.mel_filters):
        fr = i/vnr_obj.mel_filters
        ref = ref_mel_log2[i:i+vnr_obj.mel_filters]
        dut = dut_mel_log2[i:i+vnr_obj.mel_filters]
        max_mel_log2_diff_per_frame = np.append(max_mel_log2_diff_per_frame, np.max(np.abs(ref-dut)))

    for i in range(0, len(ref_quant_patch), quant_patch_len):
        dut = dut_quant_patch[i:i+quant_patch_len]
        ref = ref_quant_patch[i:i+quant_patch_len]
        max_quant_patch_diff_per_frame = np.append(max_quant_patch_diff_per_frame, np.max(np.abs(ref-dut))) 

    print("Max mel log2 diff across all frames = ",np.max(max_mel_log2_diff_per_frame)) 
    print("Max quant_patch diff across all frames = ",np.max(max_quant_patch_diff_per_frame)) 
    print("Max ref-dut_features_ref_inference tflite output diff = ",np.max(np.abs(ref_tflite_output - dut_features_ref_tflite_output))) 
    print("Max dut_features_ref_inference-dut tflite output diff = ",np.max(np.abs(dut_features_ref_tflite_output - dut_tflite_output))) 
    print("Max ref-dut tflite output diff = ",np.max(np.abs(ref_tflite_output - dut_tflite_output))) 
    a = np.abs(ref_tflite_output - dut_tflite_output)
    b = np.abs(ref_tflite_output - dut_features_ref_tflite_output)

    if(plot_results):
        fig,ax = plt.subplots(2,2)
        fig.set_size_inches(20,10)
        ax[0,0].set_title('max mel log2 diff per frame')
        ax[0,0].plot(max_mel_log2_diff_per_frame)

        ax[0,1].set_title('max_quant_patch_diff_per_frame')
        ax[0,1].plot(max_quant_patch_diff_per_frame)

        ax[1,0].set_title('tflite output')
        ax[1,0].plot(ref_tflite_output, label='ref')
        ax[1,0].plot(dut_features_ref_tflite_output, label='dut_features_ref_inf', linestyle="--")
        ax[1,0].plot(dut_tflite_output, label='dut', linestyle=':')
        ax[1,0].legend(loc="upper right")

        ax[1,1].set_title('tflite output')
        ax[1,1].plot(ref_tflite_output, label="ref")
        ax[1,1].plot(dut_tflite_output, label="dut", linestyle='--')
        ax[1,1].legend(loc="upper right")
        fig_instance = plt.gcf() #get current figure instance so we can save in a file later
        plt.show()
        plot_file = "ref_dut_mel_compare.png"
        fig.savefig(plot_file)
        
if __name__ == "__main__":
    args = parse_arguments()
    test_mel(args.tflite_model, plot_results=True)
