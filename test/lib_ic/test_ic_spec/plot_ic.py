# Copyright 2022 XMOS LIMITED.
# This Software is subject to the terms of the XMOS Public Licence: Version 1.
from __future__ import division
from __future__ import print_function
from builtins import input
from builtins import range
import numpy as np
import scipy.io.wavfile
import matplotlib.pyplot as plt
import sys
import scipy.fftpack
import os

# update PYTHONPATH to avoid using an external script
sys.path.append('../../../../audio_test_tools/python/')
sys.path.append('../../../../lib_interference_canceller/python')
import audio_generation
USE_POLAR_PLOT = 1

import argparse

import IC as ic
import audio_wav_utils as awu


import matplotlib.pyplot as plt

if __name__ == "__main__":
    
    # Parameters for the input signals
    Fs = 16000
    frequencies = [125, 250, 500, 1000, 2000, 4000, 6000]
    v = 343.0 # speed of sound
    d = 0.072
    A = 0.1
    sample_m = v/Fs

    noise_duration_s = 5
    noise_samples = noise_duration_s*Fs
    settling_time_s = 1
    sine_duration_s = 1 + 2*settling_time_s
    settling_samples = int(settling_time_s*Fs)
    sine_samples = int(sine_duration_s*Fs)
    orientations_degrees = []

    audiofiles_dir = './spec_audio/'

    if USE_POLAR_PLOT==1:
        sine_phases = list(range(0,360,1))
        orientations_degrees = list(range(0,360,30))
    else:
        sine_phases = list(range(-90,90+1,1))
        orientations_degrees = list(range(-90,90+1,30))

    duration_total_s = int(noise_duration_s+len(sine_phases)*sine_duration_s+2) # add one second to allow the filter effect to wear off
    total_samples = duration_total_s * Fs
  

    noise_mean = 0
    noise_std = 1
    noise_max_delay_samples = int(d/sample_m + 0.5)+1


    channel_count = 2
    frame_length = 512
    frame_advance = 240
    phases = 10
    y_phase_offset = 0
    output = np.zeros((channel_count-1, total_samples))

    MAX_SUPPRESSION_DB = -30
    MAX_SUPPRESSION_LIN = np.power(10.0, MAX_SUPPRESSION_DB / 20)
    FREQ_RANGE_TOLERANCE_HZ = 50
    NULL_POINT_TOLERANCE_DEG = 3
    MAX_SUPPRESSED_VALUE = A*MAX_SUPPRESSION_LIN

    noise_all = A*np.random.normal(noise_mean, noise_std, size=noise_samples + 2*noise_max_delay_samples)
    noise_all_x = np.linspace(0,noise_samples+ noise_max_delay_samples,noise_samples + noise_max_delay_samples)

    noise_0 = noise_all[ noise_max_delay_samples:-noise_max_delay_samples]
    noise_1 = np.zeros(noise_samples)

    labels = []
    sine_x = np.linspace(0, sine_duration_s * 2 * np.pi, sine_duration_s * Fs)

    sine = np.zeros(sine_samples)
    sine_delayed = np.zeros(sine_samples)
    for f in frequencies:
        phase_delays = [ (2*np.pi*f*d*np.sin(2*np.pi*deg / 360) / v) for deg in sine_phases]

        mic_wav_data = np.zeros((2, total_samples))
        mic_wav_data[0][:noise_samples] = noise_0

        start_sine_sample = noise_samples+settling_samples
        
        frequencies = [f] 
        sine = audio_generation.get_sine(duration=sine_duration_s, frequencies=[f], amplitudes=[A], sample_rate=Fs)
        max_input = np.abs(np.fft.rfft(sine[:sine_samples-2*settling_samples])).max()

        # Generate all the sinewaves with different phases 
        for phi in phase_delays:
            sine_delayed = audio_generation.get_sine(duration=sine_duration_s, frequencies=[f], amplitudes=[A], phases = [phi],  sample_rate=Fs)            
            mic_wav_data[0][start_sine_sample:start_sine_sample+sine_samples] = sine
            mic_wav_data[1][start_sine_sample:start_sine_sample+sine_samples] = sine_delayed
            start_sine_sample += sine_samples

        plot_idx = 0
        max_peak_values = []

        # Test for the different delays in the noise signals
        for orient_deg in orientations_degrees:
            noise_delay_m = d*np.sin(2*np.pi*orient_deg / 360)
            delay_samples =  (noise_delay_m / sample_m) #d*np.sin(2*np.pi*degree/360)
            sample_offset = int(delay_samples) + noise_max_delay_samples
            if delay_samples>=0:
                interp_coeff = delay_samples - int(delay_samples)
            else:
                sample_offset = sample_offset-1
                interp_coeff = 1 - (int(delay_samples) - delay_samples)

            print("Running test at freq %dHz with Noise Delay of %.1f mm (%.2f samples, %d degrees)" % (f, delay_samples*sample_m*1000, delay_samples, orient_deg))
            
            labels.append("%.1f mm (%.2f samples, %d degrees)" % (delay_samples*sample_m*1000, delay_samples, orient_deg))
            
            #print noise_max_delay_samples, delay_samples, sample_offset
            for i in range(noise_samples-1):
                noise_1[i] = noise_all[(sample_offset+i)] + interp_coeff * (noise_all[(sample_offset+i+1)] - noise_all[(sample_offset+i)])
            noise_1[noise_samples-1] = noise_all[noise_samples-1+sample_offset]
            
            mic_wav_data[1][:noise_samples] = noise_1[:noise_samples]#+sine[:noise_samples]
            
            # Save the input in a wav file
            if not os.path.exists(audiofiles_dir):
                os.makedirs(audiofiles_dir)
            audio_generation.write_data(mic_wav_data, audiofiles_dir+"test_input_%ddeg_%dHz.wav"%(orient_deg,f))

            # Initialize the IC model
            ifc = ic.adaptive_interference_canceller(frame_advance, frame_length, phases, 0)

            # Process the input files for IC
            for frame_start in range(0, total_samples-frame_length*2, frame_advance):
                adapt = True
                if frame_start>(noise_samples):
                    adapt=False
                mic_mix = awu.get_frame(mic_wav_data, frame_start, frame_advance)
                all_channels_output, Error_ap = ifc.process_frame(mic_mix, verbose=False)
                if adapt:
                     ifc.adapt(Error_ap, verbose=False)
           
                output[:, frame_start: frame_start + frame_advance] = all_channels_output[0]

            # Save the output in a wav file
            audio_generation.write_data(output, audiofiles_dir+"test_output_%ddeg_%dHz.wav"%(orient_deg,f))

            # the output is delayed by a frame length
            start_sine_out_sample =  noise_samples+settling_samples+frame_length

            # Perform the FFT in all the intervals, the signals are all real
            max_peak_values.append([])
            min_peak_val = 1000
            max_peak_val = 0
            min_peak_deg = 1000
            peak_at_orient = -1
            #max_peak_ang = 1000
            for deg_sine in sine_phases:#range(num_phases):
                F = np.fft.rfft(output[0, start_sine_out_sample+settling_samples:start_sine_out_sample+sine_samples-settling_samples])
                peak_val = audio_generation.get_magnitude(f,F,Fs,FREQ_RANGE_TOLERANCE_HZ) / A / len(F) # factor 2 is included in len(F)
                suppressed_max_val = audio_generation.get_suppressed_magnitude([f],F,Fs,FREQ_RANGE_TOLERANCE_HZ)[0] / A / len(F)
                #print np.abs(F[f]), np.abs(F).max(), np.abs(F).argmax()
                assert np.abs(F).max()==np.abs(F)[f], "Error: the max amplitude is not at the sine frequency %d, but at %d" % (f, np.abs(F).argmax())
                if peak_val>1:
                    print("Error: the IC gain at angle %d degrees is greater than 1: %f" % (deg_sine, peak_val))
                    #in_data = output[0, start_sine_out_sample+settling_samples:start_sine_out_sample+sine_samples-settling_samples]
                    #in_16bit = np.asarray(in_data * np.iinfo(np.int16).max, dtype=np.int16)
                    #scipy.io.wavfile.write("wrong_atten.wav", Fs, in_16bit.T)

                suppressed_max = A
                if peak_val>A:
                    suppressed_max = peak_val
                suppressed_val_limit = suppressed_max*MAX_SUPPRESSION_LIN
                # assert (suppressed_max_val< suppressed_val_limit), \
                if suppressed_max_val > suppressed_val_limit:
                    print("Error: Suppressed harmonics too high for phase %d and delay %.1f: %f, max value %f" \
                    % (deg_sine, delay_samples, suppressed_max_val, suppressed_val_limit))
                max_peak_values[plot_idx].append(peak_val)
                if peak_val<min_peak_val:
                    min_peak_val = peak_val
                    min_peak_deg = deg_sine
                if peak_val>max_peak_val:
                    max_peak_val = peak_val
                    max_peak_deg = deg_sine
                if deg_sine == orient_deg:
                    peak_at_orient = peak_val
                start_sine_out_sample += sine_samples
                max_peak_limit = max_peak_val*MAX_SUPPRESSION_LIN 
            # Check that the minimum peak value is within the tolerance values and degrees 
            #assert (min_peak_deg>=orient_deg-NULL_POINT_TOLERANCE_DEG) and (min_peak_deg<=orient_deg+NULL_POINT_TOLERANCE_DEG), \
            if (min_peak_deg<orient_deg-NULL_POINT_TOLERANCE_DEG) and (min_peak_deg>orient_deg+NULL_POINT_TOLERANCE_DEG):
                print("Error: the null point is not at the orientation angle %d+/-%d, but at %d" % (orient_deg,NULL_POINT_TOLERANCE_DEG,  min_peak_deg))
            #assert (peak_at_orient-min_peak_val<(max_peak_val-min_peak_val)*MAX_SUPPRESSION_LIN), \
            if (peak_at_orient-min_peak_val>(max_peak_val-min_peak_val)*MAX_SUPPRESSION_LIN):
                print("Error: the null point at the orientation angle is too large with respect to the min val: %f vs %f" % (peak_at_orient,min_peak_val))
            # Check that the maximum peak value is at the right frequency
            #assert (max_peak_values[plot_idx][sine_phases.index(orient_deg)] < max_peak_limit), \
            if max_peak_values[plot_idx][sine_phases.index(orient_deg)] > max_peak_limit:
                print("Error: Max value at the null point is too high: %f, max value %f" \
                    % (max_peak_values[plot_idx][sine_phases.index(orient_deg)], max_peak_limit))

            plot_idx+=1
        
        # Plot the figure
        plt.figure(num=None, figsize=(18, 12), dpi=80, facecolor='w', edgecolor='k')
        
        for i in range(plot_idx):
            if USE_POLAR_PLOT:
                theta = [ 2 * np.pi * phase / 360 for phase in sine_phases ]
                plt.polar(theta, max_peak_values[i])
            else:
                plt.semilogy(sine_phases, max_peak_values[i])
                plt.xlabel("DOA (Degrees)")
                plt.ylabel("Normalized max amplitude (log)")
                plt.xticks(orientations_degrees)

        plt.legend(labels, loc=1)
        plt.title("Attenuation at %dHz for different null point orientations" % (f))
        plt.grid(True)
        plt.draw()
        plt.pause(0.1)

        # raw_input("Press Enter to continue and save the plot")
        plt.savefig("IC_performance_%dHz.svg"%(f))

    # Fix for Python 2/3
    try: input = raw_input
    except NameError: pass

    input("Press Enter to exit and close the plot")
    plt.close()
    exit(0)
