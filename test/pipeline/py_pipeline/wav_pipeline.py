import argparse
import numpy as np
import scipy.io.wavfile
import pipeline
import audio_wav_utils as awu
from py_voice.core.utils import json_to_dict

def parse_arguments():
    parser = argparse.ArgumentParser()
    parser.add_argument("input", nargs='?', help="near end(y) and  far end (x) wav file", default='input.wav')
    parser.add_argument("output", nargs='?', help="error (e) and passthrough(x) wav file", default='output.wav')
    parser.add_argument("config_file", type=str, help="Config file")
    parser.add_argument("--vnr-override-file", type=str)
    parser.add_argument("--verbose", action='store_true', help="Turn verbose mode on", default=False)
    parser.add_argument("--process_until_frame", help="Process until this frame", default=-1, type=int)
    args = parser.parse_args()
    return args


def test_data(input_wav_data, file_length, ap_conf,
              disable_aec=False, disable_ic=False, disable_ns=False, disable_agc=False,
              vnr_input_override=np.empty(0, dtype=np.float64), mu_override=np.empty(0, dtype=np.float64),
              verbose = False, process_until_frame = -1):

    x_channel_count = ap_conf["aec"]['x_channel_count']
    y_channel_count = ap_conf["aec"]['y_channel_count']
    frame_advance = ap_conf["general"]['frame_advance']
    input_rate = ap_conf["general"]["fs"]
    if verbose:
        print('frame_advance: ' + str(frame_advance))
        print('File length(seconds): ' + str(float(file_length)/input_rate))
        print('X Channels: ' + str(x_channel_count))
        print('Y Channels: ' + str(y_channel_count))
        print('Rate: ' + str(input_rate))

    ap = pipeline.pipeline(disable_aec, disable_ic, disable_ns, disable_agc, ap_conf)

    output = np.zeros((y_channel_count, file_length))
    count = 0
    for frame_start in range(0, file_length-frame_advance, frame_advance):
        new_frame = awu.get_frame(input_wav_data, frame_start, frame_advance)
        #print(f"y_wav_data.shape = {y_wav_data.shape}, {y_wav_data.dtype}")
        if len(vnr_input_override) != 0: 
            ap_output = ap.process_frame(new_frame, vnr_input_override=vnr_input_override[count])
        elif len(mu_override) != 0: 
            ap_output = ap.process_frame(new_frame, mu_override=mu_override[count])
        else:
            ap_output = ap.process_frame(new_frame)
        count = count + 1
        output[:, frame_start: frame_start + frame_advance] = ap_output

        if (frame_start // frame_advance) == process_until_frame:
            break

    return output, ap.vnr_pred_log, ap.mu_log


def test_file(input_file, output_file, config_file, disable_aec=False, disable_ic=False, disable_ns=False, disable_agc=False,
              vnr_input_override=np.empty(0, dtype=np.float64), mu_override=np.empty(0, dtype=np.float64),
              verbose = False, process_until_frame = -1):
    input_rate, input_wav_file = scipy.io.wavfile.read(input_file, 'r')
    input_wav_data, input_channel_count, file_length = awu.parse_audio(input_wav_file)
    ap_conf = json_to_dict(config_file)
    fs = ap_conf["general"]["fs"]
    assert input_rate == fs, f"{input_file} has a {input_rate} sample rate while config is set to {fs}"

    output, vnr_pred, mu_log = test_data(input_wav_data, file_length, ap_conf, disable_aec=disable_aec, disable_ic=disable_ic, disable_ns=disable_ns, disable_agc=disable_agc,
                                         vnr_input_override=vnr_input_override, mu_override=mu_override, verbose=verbose, process_until_frame=process_until_frame)

    output_32bit = np.asarray(output*np.iinfo(np.int32).max, dtype=np.int32)
    scipy.io.wavfile.write(output_file, input_rate, output_32bit.T)
    #np.save("test_vnr_py.npy", vnr_pred)
    #np.save("test_mu_py.npy", mu_log)


if __name__ == "__main__":
    args = parse_arguments()
    test_file(args.input, args.output, args.config_file, verbose=args.verbose, process_until_frame=int(args.process_until_frame))    
