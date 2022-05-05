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
import subprocess
sys.path.append(os.path.join(os.getcwd(), "../../../examples/bare-metal/shared_src/python"))
sys.path.append(os.path.join(os.getcwd(), "../../shared/python"))
import run_xcoreai
import tensorflow_model_optimization as tfmot
import glob
import pytest
import py_vs_c_utils as pvc

hydra_audio_path = os.environ.get('hydra_audio_PATH', '~/hydra_audio')
print(hydra_audio_path)
streams = glob.glob(f'{hydra_audio_path}/test_wav_vnr_streams/*.wav')

def parse_arguments():
    parser = argparse.ArgumentParser()
    parser.add_argument("input_wav", nargs='?', help="input wav file")
    parser.add_argument("tflite_model", nargs='?', help=".tflite model file")
    args = parser.parse_args()
    return args

def print_model_details(interpreter_tflite):
    input_details = interpreter_tflite.get_input_details()[0]
    output_details = interpreter_tflite.get_output_details()[0]    

    # quantization spec
    if input_details["dtype"] in [np.int8, np.uint8]:
        input_scale, input_zero_point = input_details["quantization"]
    else:
        assert(False),"Error: Only 8bit input supported"
    if output_details["dtype"] in [np.int8, np.uint8]:
        output_scale, output_zero_point = output_details["quantization"]
    else:
        assert(False),"Error: Only 8bit output supported"
    
    print("input_scale = ",input_scale, " input_zero_point = ",input_zero_point)
    print("output_scale = ",output_scale, " output_zero_point = ",output_zero_point)


def run_test_wav_vnr(input_file, tflite_model, plot_results=False):
    interpreter_tflite = tf.lite.Interpreter(
        model_path=str(tflite_model))
    
    print_model_details(interpreter_tflite)
    
    with tfmot.quantization.keras.quantize_scope(): 
        vnr_obj = vnr.Vnr(model_file=tflite_model) 
    feature_patch_len = vnr_obj.mel_filters*fp.PATCH_WIDTH
    
    '''
    rate, wav_file = scipy.io.wavfile.read(input_file, 'r')
    wav_data, channel_count, file_length = awu.parse_audio(wav_file)
    print(wav_data.dtype)
    wav_data_32bit = awu.convert_to_32_bit(wav_data)
    print(wav_data_32bit.dtype, wav_data_32bit.shape)
    scipy.io.wavfile.write("test32.wav", rate, wav_data_32bit.T)
    return
    '''

    #################################################################################
    # Run DUT
    run_xcoreai.run("../../../build/test/lib_vnr/test_wav_vnr/bin/avona_test_wav_vnr.xe", input_file)
    # read dut output from various files
    with open("new_slice.bin", "rb") as fdut:
        dut_new_slice = np.fromfile(fdut, dtype=np.int32)
        dut_new_slice = np.array(dut_new_slice, dtype=np.float64)
        dut_new_slice = dut_new_slice * (float(2)**-24) #DUT mel log2 is always in 8.24 format 

    dut_norm_patch = np.empty(0, dtype=np.float64)
    with open("normalised_patch.bin", "rb") as fdut: #File has exponent followed by bfp data written. Deinterleave and convert to float
        dut_normalised_patch = np.fromfile(fdut, dtype=np.int32)
        #data is stored exp, 96 data values, exp, 96 data values..
        exp_indices = np.arange(0, len(dut_normalised_patch), feature_patch_len+1) #indexes in the numpy array containing the exponent. Starting from index 0, every 97th entry is the exponent

        exp = dut_normalised_patch[exp_indices]
        data = np.delete(dut_normalised_patch, exp_indices)
        data = data.astype(np.float64)
        for fr in range(0, len(exp)):
            dut_norm_patch = np.append(dut_norm_patch, data[fr*feature_patch_len : (fr+1)*feature_patch_len] * float(2)**exp[fr])

    with open("inference_output.bin", "rb") as fdut:
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
    rate, wav_file = scipy.io.wavfile.read(input_file, 'r')
    wav_data, channel_count, file_length = awu.parse_audio(wav_file)
    
    vnr_output = np.zeros(file_length // frame_advance)
    x_data = np.zeros(proc_frame_length)

    ref_mel = np.empty(0, dtype=np.float64)
    ref_new_slice = np.empty(0, dtype=np.float64)
    ref_norm_patch = np.empty(0, dtype=np.float64)
    ref_tflite_output = np.empty(0, dtype=np.float64)
    framecount = 0;
    for frame_start in range(0, file_length-frame_advance, frame_advance):
        # buffer the input data into STFT slices
        new_x_frame = awu.get_frame(wav_data, frame_start, frame_advance)
        x_data = np.roll(x_data, -frame_advance, axis = 0)
        x_data[proc_frame_length - frame_advance:] = new_x_frame
        X_spect = np.fft.rfft(x_data, vnr_obj.nfft)

        out_spect = vnr_obj.make_slice(X_spect)
        ref_new_slice = np.append(ref_new_slice, out_spect)

        vnr_obj.add_new_slice(out_spect, buffer_number=0)
        normalised_patch = vnr_obj.normalise_patch(vnr_obj.feature_buffers[0])
        ref_norm_patch = np.append(ref_norm_patch, normalised_patch)

        #print("python normalised_patch\n",normalised_patch)
        ref_tflite_output = np.append(ref_tflite_output, vnr_obj.run(normalised_patch))
        framecount = framecount + 1
    
    #################################################################################
    #print(ref_new_slice.shape, dut_new_slice.shape, ref_norm_patch.shape, dut_norm_patch.shape, ref_tflite_output.shape, dut_tflite_output.shape)
    #print(ref_new_slice.dtype, dut_new_slice.dtype, ref_norm_patch.dtype, dut_norm_patch.dtype, ref_tflite_output.dtype, dut_tflite_output.dtype)
    
    output_file = "temp.wav"
    # new_slice
    output_wav_data = np.zeros((2, len(ref_new_slice)))
    output_wav_data[0,:] = ref_new_slice
    output_wav_data[1,:] = dut_new_slice
    scipy.io.wavfile.write(output_file, 16000, output_wav_data.T)
    arith_closeness, geo_closeness, c_delay, peak2ave = pvc.pcm_closeness_metric(output_file)
    #print(f"new_slice: arith_closeness = {arith_closeness}, geo_closeness = {geo_closeness}, c_delay = {c_delay}, peak2ave = {peak2ave}")
    assert(geo_closeness > 0.98), "new_slice geo_closeness below pass threshold"
    assert(arith_closeness > 0.98), "new_slice arith_closeness below pass threshold"

    # norm_patch
    output_wav_data = np.zeros((2, len(ref_norm_patch)))
    output_wav_data[0,:] = ref_norm_patch
    output_wav_data[1,:] = dut_norm_patch
    scipy.io.wavfile.write(output_file, 16000, output_wav_data.T)
    arith_closeness, geo_closeness, c_delay, peak2ave = pvc.pcm_closeness_metric(output_file)
    #print(f"norm_patch: arith_closeness = {arith_closeness}, geo_closeness = {geo_closeness}, c_delay = {c_delay}, peak2ave = {peak2ave}")
    assert(geo_closeness > 0.98), "norm_patch geo_closeness below pass threshold"
    assert(arith_closeness > 0.98), "norm_patch arith_closeness below pass threshold"

    # tflite_output
    output_wav_data = np.zeros((2, len(ref_tflite_output)))
    output_wav_data[0,:] = ref_tflite_output
    output_wav_data[1,:] = dut_tflite_output
    scipy.io.wavfile.write(output_file, 16000, output_wav_data.T)
    arith_closeness, geo_closeness, c_delay, peak2ave = pvc.pcm_closeness_metric(output_file)
    #print(f"tflite_output: arith_closeness = {arith_closeness}, geo_closeness = {geo_closeness}, c_delay = {c_delay}, peak2ave = {peak2ave}")
    assert(geo_closeness > 0.98), "tflite_output geo_closeness below pass threshold"
    assert(arith_closeness > 0.98), "tflite_output arith_closeness below pass threshold"
    
    # Calculate and print various ref-dut differences
    max_mel_log2_diff_per_frame = np.empty(0, dtype=np.float64) 
    max_norm_patch_diff_per_frame = np.empty(0, dtype=np.float64)
    for i in range(0, len(ref_new_slice), vnr_obj.mel_filters):
        fr = i/vnr_obj.mel_filters
        ref = ref_new_slice[i:i+vnr_obj.mel_filters]
        dut = dut_new_slice[i:i+vnr_obj.mel_filters]
        max_mel_log2_diff_per_frame = np.append(max_mel_log2_diff_per_frame, np.max(np.abs(ref-dut)))

    for i in range(0, len(ref_norm_patch), feature_patch_len):
        dut = dut_norm_patch[i:i+feature_patch_len]
        ref = ref_norm_patch[i:i+feature_patch_len]
        max_norm_patch_diff_per_frame = np.append(max_norm_patch_diff_per_frame, np.max(np.abs(ref-dut))) 

    print("Max mel log2 diff across all frames = ",np.max(max_mel_log2_diff_per_frame)) 
    print("Max normalised_patch diff across all frames = ",np.max(max_norm_patch_diff_per_frame)) 
    print("Max ref-dut tflite inference output diff = ",np.max(np.abs(ref_tflite_output - dut_tflite_output))) 
    
    #################################################################################
    # Plot
    fig,ax = plt.subplots(2,2)
    fig.set_size_inches(20,10)
    ax[0,0].set_title('max mel log2 diff per frame')
    ax[0,0].plot(max_mel_log2_diff_per_frame)

    ax[0,1].set_title('max_norm_patch_diff_per_frame')
    ax[0,1].plot(max_norm_patch_diff_per_frame)

    ax[1,0].set_title('ref-dut inference output diff')
    ax[1,0].plot(np.abs(ref_tflite_output - dut_tflite_output))

    ax[1,1].set_title('tflite inference output')
    ax[1,1].plot(ref_tflite_output, label="ref")
    ax[1,1].plot(dut_tflite_output, label="dut", linestyle='--')
    ax[1,1].legend(loc="upper right")
    fig_instance = plt.gcf() #get current figure instance so we can save in a file later
    if(plot_results):
        plt.show()
    plot_file = f"vnr_plot_{os.path.splitext(os.path.basename(input_file))[0]}.png"
    fig.savefig(plot_file)

@pytest.mark.parametrize('input_wav', streams)
def test_wav_vnr(input_wav):
    #run_test_wav_vnr(input_wav, "model/model_output_0_0_2/model_qaware.tflite", "model/model_output_0_0_2/model_qaware.h5", plot_results=False)
    # TODO Not using model/model_output_0_0_2/model_qaware.h5 since tfmot import is giving an error on the jenkins agent. tf model is not used in this test so should be okay
    run_test_wav_vnr(input_wav, "model/model_output_0_0_2/model_qaware.tflite", plot_results=False)

if __name__ == "__main__":
    args = parse_arguments()
    print("tflite_model = ",args.tflite_model)
    run_test_wav_vnr(args.input_wav, args.tflite_model, plot_results=False)
