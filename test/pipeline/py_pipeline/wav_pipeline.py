import argparse
import numpy as np
import scipy.io.wavfile
# import matplotlib.pyplot as plt
import pipeline
import audio_wav_utils as awu
import re
import json

def json_to_dict(config_file):
    datastore = None
    with open(config_file, "r") as f:
        input_str = f.read()
        # Remove '//' comments
        json_str = re.sub(r'//.*\n', '\n', input_str)
        datastore = json.loads(json_str)
        f.close()
    return datastore

def parse_arguments():
    parser = argparse.ArgumentParser()
    parser.add_argument("input", nargs='?', help="near end(y) and  far end (x) wav file", default='input.wav')
    parser.add_argument("output", nargs='?', help="error (e) and passthrough(x) wav file", default='output.wav')
    parser.add_argument("config_file", type=str, help="Config file")

    parser.add_argument("--verbose", action='store_true', help="Turn verbose mode on", default=False)
    parser.add_argument("--process_until_frame", help="Process until this frame", default=-1, type=int)
    args = parser.parse_args()
    return args


def test_data(input_wav_data, input_rate, file_length, ap_conf, verbose = False, process_until_frame = -1, stage_a_only = False):
    input_channel_count = len(input_wav_data)
    x_channel_count = ap_conf['x_channel_count']
    y_channel_count = ap_conf['y_channel_count']
    frame_advance = ap_conf['frame_advance']
    if verbose:
        print('frame_advance: ' + str(frame_advance))
        print('File length(seconds): ' + str(float(file_length)/input_rate))
        print('X Channels: ' + str(x_channel_count))
        print('Y Channels: ' + str(y_channel_count))
        print('Rate: ' + str(input_rate))

    ap = pipeline.pipeline(input_rate, verbose, **ap_conf)

    output = np.zeros((y_channel_count, file_length))

    for frame_start in range(0, file_length-frame_advance, frame_advance):
        new_frame = awu.get_frame(input_wav_data, frame_start, frame_advance)
        #print(f"y_wav_data.shape = {y_wav_data.shape}, {y_wav_data.dtype}")
        
        ap_output = ap.process_frame(new_frame)
        output[:, frame_start: frame_start + frame_advance] = ap_output

        if (frame_start // frame_advance) == process_until_frame:
            break

    return output


def test_file(input_file, output_file, ap_conf, verbose = False, process_until_frame = -1, stage_a_only = False):

    input_rate, input_wav_file = scipy.io.wavfile.read(input_file, 'r')
    input_wav_data, input_channel_count, file_length = awu.parse_audio(input_wav_file)

    output = test_data(input_wav_data, input_rate, file_length, ap_conf, verbose, process_until_frame, stage_a_only)

    output_32bit = np.asarray(output*np.iinfo(np.int32).max, dtype=np.int32)
    scipy.io.wavfile.write(output_file, input_rate, output_32bit.T)


if __name__ == "__main__":
    args = parse_arguments()
    ap_conf = json_to_dict(args.config_file)
    test_file(args.input, args.output, ap_conf, args.verbose, int(args.process_until_frame))    


