import py_vnr.run_wav_vnr as rwv
import py_vnr.vnr as vnr
import argparse
import audio_wav_utils as awu
import scipy.io.wavfile
import numpy as np
import tensorflow as tf
import matplotlib.pyplot as plt

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
    if output_details["dtype"] in [np.int8, np.uint8]:
        output_scale, output_zero_point = output_details["quantization"]

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
            output_data = output_data.astype(np.float64)
            output_data = (output_data - output_zero_point)*output_scale
        predict_tflite[n] = output_data[0, 0]
    return predict_tflite[0]


def test_wav_vnr(input_file, model_file, tflite_model_file, plot_results=False):
    interpreter_tflite = tf.lite.Interpreter(
        model_path=str(args.tflite_model))

    # Get input and output tensors
    '''
    input_details = interpreter_tflite.get_input_details()[0]
    output_details = interpreter_tflite.get_output_details()[0]

    # quantization spec
    if input_details["dtype"] in [np.int8, np.uint8]:
        input_scale, input_zero_point = input_details["quantization"]
        print(f"input_scale {input_scale}, input_zero_point {input_zero_point}")
    else:
        print(f"ERROR: input dtype {input_details['dtype']} is not np.int8 or np.uint8")
    if output_details["dtype"] in [np.int8, np.uint8]:
        output_scale, output_zero_point = output_details["quantization"]
        print(f"output_scale {output_scale}, output_zero_point {output_zero_point}")
    else:
        print(f"ERROR: output dtype {output_details['dtype']} is not np.int8 or np.uint8")
    '''
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
    x_data = np.zeros(proc_frame_length)

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
        vnr_output_tflite[frame_start // frame_advance] = tflite_predict(interpreter_tflite, normalised_patch)

    if plot_results:
        fig,ax = plt.subplots(2)
        ax[0].set_title('tf plot')
        ax[0].plot(vnr_output_tf)
        ax[1].set_title('tflite 8bit quant model plot')
        ax[1].plot(vnr_output_tflite)
        plt.show()
        # plt.ylim(0, 1)
        #plt.show()
        #plt.savefig("tf_qaware.png")

if __name__ == "__main__":
    args = parse_arguments()
    print(args.input_wav)
    print(args.model)
    print(args.tflite_model)

    test_wav_vnr(args.input_wav, args.model, args.tflite_model, plot_results=True)

    
    #rwv.run_wav_vnr(args.input_wav, args.model, plot_results=True)
