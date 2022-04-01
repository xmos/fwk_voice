import py_vnr.run_wav_vnr as rwv
import py_vnr.vnr as vnr
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
    parser.add_argument("input_wav", nargs='?',
                        help="16kHz monno wav file to run")
    parser.add_argument("model", nargs='?',
                        help=".h5 model")
    parser.add_argument("tflite_model", nargs='?',
                        help=".tflite model")
    args = parser.parse_args()
    return args

def tflite_predict(interpreter_tflite, input_patches):
    interpreter_tflite.allocate_tensors()

    # Get input and output tensors.
    input_details = interpreter_tflite.get_input_details()[0]
    output_details = interpreter_tflite.get_output_details()[0]

    # quantization spec
    if input_details["dtype"] in [np.int8, np.uint8]:
        input_scale, input_zero_point = input_details["quantization"]
        #print(f"input_scale {input_scale} input_zero_point {input_zero_point}") 
    if output_details["dtype"] in [np.int8, np.uint8]:
        output_scale, output_zero_point = output_details["quantization"]
        #print(f"output_scale {output_scale} output_zero_point {output_zero_point}") 

    predict_tflite = np.zeros(input_patches.shape[0])

    for n in range(len(predict_tflite)):
        this_patch = input_patches[n:n+1]

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


def test_wav_vnr(input_file, model_file, tflite_model_file, plot_results=False):
    interpreter_tflite = tf.lite.Interpreter(
        model_path=str(args.tflite_model))

    #The number of samples of data in the frame
    proc_frame_length = 2**9
    frame_advance = 240
    frame_buffer = np.zeros(3*proc_frame_length)
    vnr_obj = vnr.Vnr(model_file=model_file)
    
    rate, wav_file = scipy.io.wavfile.read(input_file, 'r')
    wav_data, channel_count, file_length = awu.parse_audio(wav_file)    
    print(channel_count, wav_data.dtype, wav_data.shape)

    vnr_output_tf = np.zeros(file_length // frame_advance)
    vnr_output_tflite = np.zeros(file_length // frame_advance)
    vnr_output_tflite_quant = np.zeros(file_length // frame_advance, dtype=np.int8)
    x_data = np.zeros(proc_frame_length)
    
    full_features = np.empty(0, dtype=np.int8)
    for frame_start in range(0, file_length-proc_frame_length, frame_advance):
        # buffer the input data into STFT slices
        new_x_frame = awu.get_frame(wav_data, frame_start, frame_advance)
        x_data = np.roll(x_data, -frame_advance, axis = 0)
        x_data[proc_frame_length - frame_advance:] = new_x_frame
        # Extract features
        normalised_patch = rwv.extract_features(x_data, vnr_obj)

        # do the prediction with tf model
        vnr_output_tf[frame_start // frame_advance] = vnr_obj.run(normalised_patch)

        # do the prediction with tflite model
        vnr_output_tflite[frame_start // frame_advance], vnr_output_tflite_quant[frame_start // frame_advance], features = tflite_predict(interpreter_tflite, normalised_patch)
         
        full_features = np.append(full_features, np.array(features, dtype=np.int8).flatten())
    
    with open("features.bin", "wb") as fp_features:
        fp_features.write(full_features)
    
    # TODO Run xcore here
    run_xcoreai.run("../../../build/examples/bare-metal/vnr/bin/avona_example_bare_metal_vnr.xe")

    #read inference results from xcore
    with open("xcore_inference.bin", "rb") as fxcore:
        vnr_output_xcore_quant = np.fromfile(fxcore, dtype=np.int8)

    # dequantise values from xcore 
    #print(vnr_output_xcore_quant)
    output_details = interpreter_tflite.get_output_details()[0]
    if output_details["dtype"] in [np.int8, np.uint8]:
        output_scale, output_zero_point = output_details["quantization"]
        #print(f"output_scale {output_scale} output_zero_point {output_zero_point}") 

    if output_details['dtype'] in [np.int8, np.uint8]:
        vnr_output_xcore = np.array(vnr_output_xcore_quant, dtype=np.float64)
        vnr_output_xcore = (vnr_output_xcore - output_zero_point)*output_scale
    
    frames = min(len(vnr_output_xcore), len(vnr_output_tflite))
    diff = max(np.abs(vnr_output_tflite[:frames] - vnr_output_xcore[:frames]))
    
    diff_quant = max(np.abs(vnr_output_tflite_quant[:frames] - vnr_output_xcore_quant[:frames]))
    print("max diff tflite xcore", diff, diff_quant)

    if plot_results:
        fig,ax = plt.subplots(3, sharex=True)
        print(fig.get_size_inches())
        fig.set_size_inches(10,8)
        ax[0].set_title('tf model plot')
        ax[0].plot(vnr_output_tf)
        ax[1].set_title('tflite model plot')
        ax[1].plot(vnr_output_tflite)
        ax[2].set_title('xcore model plot')
        ax[2].plot(vnr_output_xcore)
        fig_instance = plt.gcf() #get current fig instance so it can be saved later
        plt.show()
        plotfile = "plot_" + os.path.splitext(os.path.basename(tflite_model_file))[0] + ".png"
        fig.savefig(plotfile)

if __name__ == "__main__":
    args = parse_arguments()
    print(args.input_wav)
    print(args.model)
    print(args.tflite_model)

    #test()
    test_wav_vnr(args.input_wav, args.model, args.tflite_model, plot_results=True)



    
    #rwv.run_wav_vnr(args.input_wav, args.model, plot_results=True)
