#!/bin/bash
set -e

source helpers.sh

help()
{
   echo "Voice reference design wakeword detection test"
   echo
   echo "Syntax:"
   echo "test_wakeword_detection.sh [-c|h] <firmware.xe> <list_file>"
   echo 
   echo "Options:"
   echo "h     Print this Help."
   echo "c     Number of channels in input wavs"
   echo
   echo "Example that saves output to a log file:"
   echo "   $ test_wakeword_detection.sh -c 1 <firmware.xe> <list_file> | tee test_wakeword_detection.log"
   echo 
   
}

# flag arguments
while getopts c:h option
do
    case "${option}" in
        c) CHANNELS=${OPTARG};;
        h) help
           exit;;
    esac
done

# positional arguments
FIRMWARE=${@:$OPTIND:1}
LIST_FILE=${@:$OPTIND+1:1}

# load input wav files
WAV_DIR=$(dirname "$LIST_FILE")
WAV_LIST="$(< $LIST_FILE)"

DEVICE_DRIVER=$(get_device_driver)
DEVICE_NAME=$(get_device_name)
REMIX_PATTERN=$(get_input_remix_pattern $CHANNELS)
SOX_PLAY_OPTS=$(get_sox_play_options 16000 16)

#set -x

# launch firmware
xrun --xscope $FIRMWARE 2>&1 &

# wait for it to start
sleep $(get_firmware_startup_duration)

# play the input wav files
for WAV_FILE in $WAV_LIST; do
    echo "Wakeword Test: $WAV_FILE"
    
    sox $WAV_DIR/$WAV_FILE $SOX_PLAY_OPTS -t wav - $REMIX_PATTERN | sox -t wav - -t $DEVICE_DRIVER "$DEVICE_NAME"
    sleep 2
done

# kill the firmware
pkill xgdb
pkill xrun
