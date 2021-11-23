#!/bin/bash

# Include test utils
. utils.sh

test_dirs="$(read_config in_dir)"
test_dirs="$test_dirs $(read_config out_dir)"
test_dirs="$test_dirs $(read_config results_dir)"
test_dirs="$test_dirs $(read_config results_dir)/plots/fail"
test_dirs="$test_dirs $(read_config results_dir)/logs/fail"

for dir in $test_dirs; do
    echo $dir
    mkdir -p $dir
    if grep -q "clean" <<< $1; then
        rm -r $dir/*
    fi
done

