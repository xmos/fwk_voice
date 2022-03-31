# Copyright 2022 XMOS LIMITED.
# This Software is subject to the terms of the XMOS Public Licence: Version 1.
# Convolve room impulse responses with WAV files
#
# Dependencies:
# - Matplotlib 2.2.4
# - Numpy 1.16.2
# - Scipy 1.2.1
# - SoundFile 0.10.2

import os
import re
import numpy as np
import scipy.signal as spsig
import matplotlib.pyplot as plt
import soundfile as sf
import time
import sys

# simulate multichannel playback arriving at microphones of 3510


#Load file and apply gain correction from file name
def import_calibrate_impulse(file):
    data, fs = sf.read(file)
    gaindB = int(re.findall(r'(?<=gain)[0-9]+(?=dB)', file)[0])
    gain = 10.0 ** (gaindB / 20.0)

    signal = data / gain

    impulse_file_frames = sf.info(file, verbose=True).frames
    return signal, fs, impulse_file_frames

#input audio numpy array, volume changes in sample inicies and decibels
def apply_volume_changes(input_audio, volume_changes):
    
    last_volume_change = volume_changes[0]
    list_to_iterate = volume_changes[1:]
    list_to_iterate.append([input_audio.shape[0] - 1,0])
    print (list_to_iterate)
    for volume_change in list_to_iterate:
        start_idx = int(last_volume_change[0])
        end_idx = int(volume_change[0])
        print("Change volume by {}dB from {} until {}".format(last_volume_change[1], start_idx, end_idx))
        if end_idx > input_audio.shape[0]:
            print ("skipping vol change index {} beyond input {}".format(end_idx, input_audio.shape[0]))
            continue
        gain = 10 ** (last_volume_change[1] / 20)
        input_audio[start_idx:end_idx] *= gain

        last_volume_change = volume_change

    return input_audio



def do_simulation(input_audio_dir, input_audio_files, model_dir, output_audio_dir, max_frames=False, volume_changes=None):

    mic0 = []
    mic1 = []

    #XMOS TEST LAB BRISTOL
    impulse_files = [
                    'bristol_lab/DUT_spkr1_fs48kHz.npy',           #Left soundbar speaker
                    'bristol_lab/DUT_spkr2_fs48kHz.npy',           #Right soundbar speaker
                    'bristol_lab/Env2_spkr2_fs48kHz.npy'           #Near-enb source, 60 degrees 2.5m
                    ]

    #XMOS BOARDROOM
    impulse_files = [
                    'boardroom/005_xmostest_dutL_dutR_repeat_spkr1_fs48kHz.npy',           #Left soundbar speaker
                    'boardroom/005_xmostest_dutL_dutR_repeat_spkr2_fs48kHz.npy',           #Right soundbar speaker
                    'boardroom/006_xmostest_noise_loc1_extended_spkr1_fs48kHz.npy'         #Near source, 90 degrees, 2.5m
                    ]

    for i in impulse_files:
        s = np.load(os.path.join(model_dir, i))
        mic0.append(s[:, 0])
        mic1.append(s[:, 1])
        impulse_file_frames = s.shape[0]

    mic0 = np.stack(mic0)
    mic1 = np.stack(mic1)

    #Build list of channel arrays from input files
    input_channels = []
    max_length = 0
    for af in input_audio_files:
        audio, fs = sf.read(os.path.join(input_audio_dir, af))
        length = audio.shape[0]
        if max_frames:
            while length < max_frames:
                # If too short, loop the recording until it is of sufficient length
                audio = np.concatenate((audio, audio), axis=0)
                length = audio.shape[0]
            if length > max_frames:
                length = max_frames
                audio = audio[:length]
        max_length = length if length > max_length else max_length

        if len(audio.shape) > 1:
            channel_count = audio.shape[1]
            for ch in range(channel_count):
                input_channels.append(audio[:,ch])
        else:
            channel_count = 1
            input_channels.append(audio[:])

        for i in range(channel_count):
            path_number = len(input_channels) - len(audio.shape) + i + 1
            print("Audio path {} is: {}, channel: {}".format(path_number, af, i))

    num_audio_input_channels = len(input_channels)

    print("All input files padded to length: {}s".format(max_length//fs))

    #Two channel mic file
    mic_output = np.zeros((max_length, 2))


    for ic, audio_input_channel_idx in zip(input_channels, list(range(num_audio_input_channels))):
        print("Convolving path: {} of {}".format(audio_input_channel_idx + 1, num_audio_input_channels))
        #Pad all to same length
        length = ic.shape[0]
        ic = np.pad(ic, (0, max_length - length), mode='constant', constant_values=0)

        if volume_changes and audio_input_channel_idx < 2:
            ic = apply_volume_changes(ic, volume_changes)

        #Do convolution
        mic_output[:, 0] += spsig.fftconvolve(ic, mic0[audio_input_channel_idx, :], 'same')
        mic_output[:, 1] += spsig.fftconvolve(ic, mic1[audio_input_channel_idx, :], 'same')

    
    #Account for offset generated during convolution 
    half_impulse_len_zeros = np.zeros((int(impulse_file_frames/2), 2))
    mic_output = np.concatenate((half_impulse_len_zeros, mic_output[: -int(impulse_file_frames/2), :]))

    # input_rms = np.sqrt(np.mean(audio**2.0))
    # output_rms = np.sqrt(np.mean(output**2.0))
    # outgain = input_rms/output_rms
        
    #Add in far end as channels 3 & 4
    print("Adding far end reference channels to simulated mics")
    far_end_transposed = np.stack(input_channels[0:2])
    output = np.transpose(np.append(np.transpose(mic_output), far_end_transposed).reshape(4, -1))

    if not os.path.exists(output_audio_dir):
        os.makedirs(output_audio_dir)
    
    sf.write(os.path.join(output_audio_dir, "simulated_room_output.wav"), output, fs)



if __name__ == '__main__':
    input_audio_dir = 'input_audio_to_room_model'
    output_audio_dir = 'output_audio_from_room_model'
    model_dir = 'room_model'
    input_audio_files = ['Pharrell Williams - Happy (Official Music Video).wav', 'alexa_utterances.wav']
    #input_audio_files = ['silence.wav', 'alexa_utterances.wav']
    do_simulation(input_audio_dir, input_audio_files, output_audio_dir)

    input_audio_dir = 'output_audio_from_room_model'
    input_audio_file = 'simulated_room_output.wav'
    output_audio_dir = '.'
    output_audio_file = 'stage_a_input.wav'
    input_audio = os.path.join(input_audio_dir, input_audio_file)
    output_audio = os.path.join(output_audio_dir, output_audio_file)
    copyfile(input_audio, output_audio)
    remove_segment(output_audio, 48000*60, 240, cut_far_end = True)

