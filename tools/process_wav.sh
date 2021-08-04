#!/bin/bash
set -e

help()
{
   echo "Voice reference design wav file processor"
   echo
   echo "Syntax: process_wav.sh [-c|h] to_device.wav from_device.wav"
   echo "options:"
   echo "h     Print this Help."
   echo "c     Number of channels in input wav"
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
INPUT_FILE=${@:$OPTIND:1}
OUTPUT_FILE=${@:$OPTIND+1:1}

# determine driver & device
uname=`uname`
if [[ "$uname" == 'Linux' ]]; then
    DEVICE_DRIVER="alsa"
    DEVICE_NAME="hw:CARD=XVF3652,DEV=0"
elif [[ "$uname" == 'Darwin' ]]; then
    DEVICE_DRIVER="coreaudio"
    DEVICE_NAME="UAC2"
fi

# determine input remix pattern
#  the input channel order is: ref L, ref R, mic 0, mic 1
if [[ "$CHANNELS" == 2 ]]; then
    # file only has microphone channels
    #   need to insert 2 silent reference channels
    REMIX_PATTERN="remix 0 0 1 2"
elif [[ "$CHANNELS" == 6 ]]; then
    REMIX_PATTERN="remix 3 4 5 6"
else
    REMIX_PATTERN=""
fi

# call sox pipelines
SOX_PLAY_OPTS="--buffer=65536 --rate=16000 --bits=16 --encoding=signed-integer --endian=little --no-dither"
SOX_REC_OPTS="--buffer=65536 --channels=6 --rate=16000 --bits=16 --encoding=signed-integer --endian=little --no-dither"

#set -x
sox -t $DEVICE_DRIVER $DEVICE_NAME $SOX_REC_OPTS -t wav $OUTPUT_FILE &

#sleep 1 # need to wait a bit

sox $INPUT_FILE $SOX_PLAY_OPTS -t wav - $REMIX_PATTERN | sox -t wav - -t $DEVICE_DRIVER $DEVICE_NAME

pkill -P $$
wait # to die
