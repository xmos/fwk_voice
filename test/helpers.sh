
uname=`uname`

# Get the USB audio device name
get_device_name() {
    if [[ "$uname" == 'Linux' ]]; then
        echo "hw:CARD=Avona Voice Reference Design,DEV=0"
    elif [[ "$uname" == 'Darwin' ]]; then
        echo "Avona Voice Reference Design"
    else
        printf '%s\n' "Unsupported unix name: $uname" >&2
        exit 1    
    fi
}

# Get the USB audio device driver
get_device_driver() {
    if [[ "$uname" == 'Linux' ]]; then
        echo "alsa"
    elif [[ "$uname" == 'Darwin' ]]; then
        echo "coreaudio"
    else
        printf '%s\n' "Unsupported unix name: $uname" >&2
        exit 1    
    fi
}

# Return the maximum xrun startup duration (in seconds)
get_firmware_startup_duration() {
    echo "12";
}

# Get the options for sox when playing a wav file
#
#  Arguments:
#
#  sample_rate     - Sample rate (in Hz) to play audio
#  sample_bitdepth - Sample bit depth
get_sox_play_options() {
    sample_rate=$1
    sample_bitdepth=$2
    echo "--buffer=65536 --rate=$sample_rate --bits=$sample_bitdepth --encoding=signed-integer --endian=little --no-dither"
}

# Get the options for sox when recording a wav file
#
#  Arguments:
#
#  sample_rate     - Sample rate (in Hz) to play audio
#  sample_bitdepth - Sample bit depth
get_sox_record_options() {
    sample_rate=$1
    sample_bitdepth=$2
    echo "--buffer=65536 --channels=6 --rate=$sample_rate --bits=$sample_bitdepth --encoding=signed-integer --endian=little --no-dither"
}

# Determine input remix pattern
#  The test vector input channel order is: Mic 1, Mic 0, Ref L, Ref R
#  NOTE: 3x10 output channel order is: Ref L, Ref R, Mic 1, Mic 0, ASR, Comms
#        Avona's output channel order is: ASR, Comms, Ref L, Ref R, Mic 0, Mic 1
#
#  Arguments:
#
#  channels - Number of input channels
get_input_remix_pattern() {
    channels=$1

    if [[ "$channels" == 1 ]]; then # reference-less test vector
        # file only has 1 microphone channel
        #   need to insert 2 silent reference channels and repeat microphone channel
        echo "remix 0 0 1 1"
    elif [[ "$channels" == 2 ]]; then # reference-less test vector
        # file only has microphone channels
        #   need to insert 2 silent reference channels
        echo "remix 0 0 2 1"
    elif [[ "$channels" == 4 ]]; then # standard test vector
        echo "remix 3 4 2 1"
    elif [[ "$channels" == 6 ]]; then  # assuming test vector from Avona
        echo "remix 3 4 5 6"
    else
        echo ""
    fi
}