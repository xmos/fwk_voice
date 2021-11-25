#!/bin/bash

tests="simple multitone"
headroom="2 4 8"
echos="short long decaying"
ref_type="discrete continuous single noise"

audio_dir="spec_audio"
out_dir="aec_out"
xc_dir="../test_wav_aec"

setup_env() {
    pushd ../../../infr_scripts_pl/Build/
    source SetupEnv
    popd
}

build() {
    pushd $xc_dir
    waf configure
    waf build --aec-config='3 1 1 16'
    popd
}

make_dirs() {
    mkdir -p $audio_dir
    mkdir -p $out_dir
}

generate_audio() {
    python testgen.py $audio_dir
}

test_audio() {
    in="$audio_dir/$1-AudioIn.wav"
    ref="$audio_dir/$1-AudioRef.wav"
    out="$out_dir/$1-Error.wav"

    echo "AudioIn: $in"
    echo "AudioRef: $ref"
    echo "AudioOut: $out"
    axe --args "$xc_dir/bin/test_wav_aec.xe" $ref $in $out
    echo "Saved: $out"
}

setup_env
build
make_dirs
echo "Generating audio..."
generate_audio

echo "Running tests..."
for t in $tests; do
    for h in $headroom; do
        for e in $echos; do
            for ref in $ref_type; do
                echo $t hr$h $e $ref
                test_audio "$t-$e-$ref-hr$h"
            done
        done
    done
done
echo "Done. Output audio in $out_dir"
