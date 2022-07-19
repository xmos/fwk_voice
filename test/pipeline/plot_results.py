#!/usr/bin/python
# Copyright 2018-2022 XMOS LIMITED.
# This Software is subject to the terms of the XMOS Public Licence: Version 1.

from __future__ import absolute_import
from __future__ import division
from __future__ import print_function

from builtins import zip
import argparse
import re
import matplotlib.pyplot as plt
import numpy as np
import os
from pathlib import Path
import csv

KEYS = ['device', 'test_spec', 'date', 'location', 'env_audio', 'env_level',
        'pb_audio', 'pb_level', 'take']
COLOURS = ['b', 'r', 'g', 'c', 'm', 'y', 'k', 'w']
LINESTYLES = ['-', '--', ':', '-.']
MARKERSTYLES = ['o', 'x', '.', 'D']

def add_id(test_dict, ignore_keys):
    test_dict['id'] = '_'.join(test_dict[key] for key in KEYS if key not in
                                ignore_keys and test_dict[key] is not None)


def create_aggregate(test_set, average_keys):
    av_test_set = []
    for test in test_set:
        new_test = test
        add_id(new_test, average_keys)
        found = False
        for av_test in av_test_set:
            if av_test['id'] == new_test['id']:
                av_test['results'] = av_test['results'] + new_test['results']
                found = True
                break
        if not found:
            av_test_set.append(new_test)

    return av_test_set


def create_subplots(title, plot_data, test_type, env_audio, pb_audio):
    # Get number of subplots
    subplot_set = []
    test_name = ""
    test_spec = ""
    for data_set in plot_data:
        for module_set in data_set['module_sets']:
            for test in module_set:
                if (test["env_audio"] == env_audio) and (test["pb_audio"] == pb_audio):
                    subplot_set.append(test['location'])

    subplot_set = sorted(set(subplot_set))

    fig, subplot_arr = plt.subplots(1, len(subplot_set), figsize=(15, 7))

    for data_set_index, data_set in enumerate(plot_data):
        for module_set_index, module_set in enumerate(data_set['module_sets']):
            for subplot_index, subplot_title in enumerate(subplot_set):
                subplt_data = [test for test in module_set if subplot_title == test['location']]
                if not subplt_data:
                    continue

                test_name = subplt_data[-1]['device']
                test_spec = subplt_data[-1]['test_spec']
                sorted_subplt_data = []
                if test_type == "PB":
                    for i in subplt_data:
                        if i['pb_level'] is None:
                            i['pb_level'] = '0dB'
                    sorted_subplt_data = sorted(subplt_data, key=lambda k: k['pb_level'])
                elif test_type == "ENV":
                    for i in subplt_data:
                        if i['env_level'] is None:
                            i['env_level'] = '0dB'
                    sorted_subplt_data = sorted(subplt_data, key=lambda k: k['env_level'])
                subplot = subplot_arr if len(subplot_set) == 1 else subplot_arr[subplot_index]

                x_labels = []
                if test_type == "PB":
                    x_labels = ["0dB" if test['pb_level'] is None else test['pb_level'] for test in sorted_subplt_data]
                    subplot.set_xlabel('DUT Ref Level')
                elif test_type == "ENV":
                    x_labels = ["0dB" if test['env_level'] is None else test['env_level'] for test in sorted_subplt_data]
                    subplot.set_xlabel('Env Audio Level')

                y = [sum(test['results'])/len(test['results']) for test in sorted_subplt_data]

                # Ensure even x axis spacing. Define 0dB as 50.
                x = [50 if level is "0dB" else int(level.split("dB")[0]) for level in x_labels]
                subplot.plot(x, y,
                             marker=MARKERSTYLES[module_set_index],
                             linestyle=LINESTYLES[module_set_index],
                             color=COLOURS[data_set_index],
                             label="Data Set {}, {}".format(data_set_index + 1, module_set[0]['modules']))
                subplot.xaxis.set_ticks(x)
                subplot.xaxis.set_ticklabels(x_labels)

                # Add values next to points
                for i, j in zip(x, y):
                    subplot.annotate("{:.0f}".format(j), xy=(i, j+0.6))

                subplot.title.set_text(subplot_title)

                if subplot_index == 0:
                    subplot.set_ylabel('Result (%)')
                subplot.set_ylim(-3, 103)
                subplot.margins(0.1)

        # Get list of modules for plot description
        module_names = ""
        for module_set in data_set['module_sets']:
            if not module_set:
                continue
            if module_names:
                module_names += ", "
            module_names += "{}".format(module_set[0]['modules'])

        data_set_text = "Data Set {}\nTest: {}\nTest Spec: {}\nModules: {}\n".format(data_set_index+1,
                                                                test_name.split('/')[-1],
                                                                test_spec,
                                                                module_names)
        plt.gcf().text(0.81, 0.6 - (0.25 * data_set_index), data_set_text, fontsize=9, horizontalalignment='left')

    overview_test_box = "Env Audio: {}\nDUT Ref Audio: {}".format(env_audio, pb_audio)
    # Legend position needs to account for variable subplots
    fig.subplots_adjust(top=0.9, left=0.05, right=0.79, bottom=0.14)
    if len(subplot_set) == 1:
        subplot_arr.legend(bbox_to_anchor=(0.7, -0.1), ncol=len(plot_data))
    elif len(subplot_set) > 1:
        subplot_arr.flatten()[-2].legend(bbox_to_anchor=(1.1, -0.1), ncol=len(plot_data))
    plt.gcf().text(0.81, 0.85, overview_test_box, fontsize=9, horizontalalignment='left')
    plt.suptitle(title)
    fig_name = "{}.png".format(title.replace('\n', ' ').replace('-', ' ').replace(' ', '_'))
    plt.savefig(fig_name, bbox_inches="tight")



# Create one plot per combination of env_audio and pb_audio
def create_plots(test_sets, aggregate, title=None):
    # Find all (env_audio, pb_audio) combinations across all test sets
    audio_combinations = []
    for test_set in test_sets:
        audio_combinations += [[test['env_audio'],test['pb_audio']] for test in test_set['tests']]
    audio_set = set(tuple(combination) for combination in audio_combinations if combination != ['Clean', None])
    
    # Seperate plots by (env_audio, pb_audio) combination
    for audio in audio_set:
        plot_data = []
        for test_set in test_sets:
            audio_test_set = [test for test in test_set['tests']
                                if (audio[0] == test["env_audio"] and audio[1] == test["pb_audio"])
                                or ("Clean" == test["env_audio"] and None == test["pb_audio"])]

            plot_title = ""
            if title != None:
                plot_title = title+"_"
            test_type = ""
            # If pb_audio is not present then IC test, otherwise AEC test
            if audio[1] is None:
                plot_title = plot_title + "Env Audio - {}".format(audio[0])
                test_type = "ENV"
            else:
                plot_title = plot_title + "DUT Audio - {}\nEnv Audio - {}".format(audio[1], audio[0])
                test_type = "PB"

            # Reduce the test set by aggregating results
            aggregate_pipeline_tests = create_aggregate(audio_test_set, ['results'] + aggregate)

            # Bundle aggregated tests, grouped by module
            module_sets = []
            pipeline_set = set([test["modules"] for test in aggregate_pipeline_tests])
            for pipeline in pipeline_set:
                module_sets.append([test for test in aggregate_pipeline_tests if test["modules"] == pipeline])

            plot_data.append({'module_sets': module_sets})
        create_subplots(plot_title, plot_data, test_type, env_audio=audio[0], pb_audio=audio[1])

def parse_csv(csv_file, kwd_column=0, add_wwe_title=False):
    print(f"file: {csv_file}, kwd_column={kwd_column}")
    module_name = os.path.splitext(Path(os.path.basename(csv_file)))[0]
    file_data = {'metadata': [], 'tests': []}
    with open(csv_file, 'r') as f:
        lines = f.readlines()
        kwd_title = ""
        if add_wwe_title:
            kwd_title = "_" + lines[0].split(",")[1+kwd_column]
        for line in lines:
            cols = line.rstrip().split(",")
            test_line = re.search("(.*)_(v.*)_(\d*)_(Loc\d)_(Noise\d|Clean)_?(\d*dB)?_(.*DUT\d)?_?(\d*dB)?_(Take\d).*.wav.*", cols[0])
            metadata_line = re.search("(.*),(.{7}),(.*)(,)?", line.rstrip())
            if test_line:
                new_test = dict(list(zip(KEYS, test_line.groups())))
                new_test['results'] = [int(cols[1+kwd_column])] #kwd_column is the column index from where to read the keywords from. 1 is added to account for the test stream column
                new_test['modules'] = module_name + kwd_title
                file_data['tests'].append(new_test)
            elif metadata_line:
                new_metadata = dict(pair for pair in zip(['repo', 'hash', 'tag'], metadata_line.groups()) if pair[1] != '')
                file_data['metadata'].append(new_metadata)
    return file_data

def read_csv(input_file):
    with open(input_file, 'r') as csvfile:
        reader = csv.reader(csvfile)
        data = list(reader)
    return data

# Plot all streams in a single plot. Write whatever's plotted in a CSV file as well.
def single_plot(input_files, file_column_dict, add_wwe_title, figname="plot"):
    combined_list = [] # List of everything that's being plotted, so it can be written to a csv file later.
    if figname == None:
        figname = "kwd_plot"
        
    # Get test stream names. Always present in column 0
    data = np.array(read_csv(input_files[0]))
    combined_list = data[:,0].T
    streams = data[1:,0] # Skip first row. It has the column titles

    scores = []
    count = 0
    fig,ax = plt.subplots(1)
    fig.set_size_inches(20,10)

    # For every input file, get keywords from the relevant column and plot against stream names
    for file in input_files:
        name = os.path.splitext(Path(os.path.basename(file)))[0] # legend will be <filename>_<column_title>
 
        # Get the keyword column we're plotting. Plot 0th (i.e first column after test case column) if user hasn't specified anyhting.
        kwd_column = 0 # First column after the stream names. We start from 0. Do a +1 to account for test streams column later
        if str(count) in file_column_dict: # If user has specified for this file
            kwd_column=int(file_column_dict[str(count)])

        print(f"file {file}, kwd_column = {kwd_column}")
        kwd_column = kwd_column + 1 # To account for first column being test streams
        count = count + 1
        
        # Get kwd list from kwd_column
        kwds = []
        data = np.array(read_csv(file))
        kwds = data[:,kwd_column]
        kwds[0] = name + "_" + kwds[0] # Read column title from 0th row
        # Add to the list tracking everything plotted
        combined_list = np.column_stack((combined_list, kwds))
        # Plot
        plot_kwds = [int(i) for i in kwds[1:]] # Convert from string to integers
        ax.plot(streams, plot_kwds,'-o', label=kwds[0]) #skip kwd[0] since that's the title of the column which is going to appear as the plot legend

    with open(figname+'.csv', 'w', newline='') as csvfile:
        csvwriter = csv.writer(csvfile)
        for l in combined_list:
            csvwriter.writerow(l)

    ax.legend(loc='upper right', bbox_to_anchor=(1, 1.15)) # Move legend outside the box
    ax.set_xticklabels(streams, rotation=90, fontsize=7)
    fig.subplots_adjust(bottom=0.4)
    fig_instance = plt.gcf() #get current figure instance so we can save in a file later
    plt.show()
    fig_instance.savefig(figname+".png")
        


def get_args():
    parser = argparse.ArgumentParser("Script to plot the Sensory results")
    parser.add_argument("input_files", nargs='*', help="Input csv files with test results")
    parser.add_argument("--ww-column", type=str,
                        help="<file index>_<wakewords column index to plot>. Used when csv files contain wakewords from multiple WW engines. Eg. For a csv file of form <stream_name>,<kwds ww engine1>,<kwds ww engine2>, to plot keywords for WW engine1, set --ww-column=0_0 and to plot WW engine2, set --ww-column=0_1. For specifying multiple files, use multiple fileindex_columnindex values. By default kwd column 0 is plotted. kwd column 0 is the first column adter the test case name, kwd column 1, the second column and so on. ")
    parser.add_argument("--add-wwe-title", action="store_true", help="Add WWE name to the plot legend. The WWE needs to be specified on the kwd scores column of the first row in the csv for this to work.")
    parser.add_argument("--figname", type=str, help="title to add to the output file name")
    parser.add_argument("--single-plot", action="store_true", help="Plot kwd scores for all streams in a single plot, instead of generating stream category wise multiple plots")
    return parser.parse_args()

#Expected input csv file format:
# Row 0 to contain the column titles, something like stream_name,kwds_WWE_engine_0,kwds_WWE_engine_1,...
# Row 1 onwards, the test case name, followed by keywords for various WW Engines, one WWE per column.

#Usage example: Run with 2 input files and plot keywords column 1 for both input files.
#python plot_results.py file1.csv file2.csv --ww-column="0_1 1_1"

#Usage example: Run with 1 input file and plot keywords column 0, 1, 2 for the same input file.
#python plot_results.py file1.csv file1.csv file1.csv --ww-column="0_0 0_1 0_2"

#Usage example: Run with 1 input file and plot keywords column 0, 1, 2 for the same input file, all plotted in a single plot with the figname specified
#python plot_results.py file1.csv file1.csv file1.csv --ww-column="0_0 0_1 0_2" --figname="file1_plot" --single-plot
def main():
    args = get_args()
    file_column_dict = {} # Dictionary of form dict[file_index] = kwd_column_index_to_plot
    if(args.ww_column != None):
        l = args.ww_column.split() # Split into list of "file_column" values
        file_column_dict = dict([(f.split('_')[0], int(f.split('_')[1])) for f in l]) # Split into [(file,column), (file,column)] values and store as dict['file']=column
    print(f"file_column_dict = {file_column_dict}")    
    if args.single_plot: # Plot all streams in one plot.
        single_plot(args.input_files, file_column_dict, args.add_wwe_title, args.figname)
        return

    input_data = []
    count = 0
    for file in args.input_files:
        if str(count) in file_column_dict: 
            input_data.append(parse_csv(file, kwd_column=int(file_column_dict[str(count)]), add_wwe_title=args.add_wwe_title))
        else:
            input_data.append(parse_csv(file, add_wwe_title=args.add_wwe_title))
        count = count + 1
    

    create_plots(input_data, ['take'], title=args.figname)
    

if __name__ == "__main__":
    main()
