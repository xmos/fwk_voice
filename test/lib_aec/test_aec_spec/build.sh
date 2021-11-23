#!/bin/bash

# Include test utils
. utils.sh

build() {
    pushd $(read_config aec_xc_dir)
    threads=$(read_config threads)
    x_channel_count=$(read_config x_channel_count)
    y_channel_count=$(read_config y_channel_count)
    phase_count=$(read_config phases)
    sf_phase_count=$(read_config shadow_filter_phases)
    echo "$y_channel_count"
    echo "$phase_count"
    echo "$sf_phase_count"
    waf configure clean build --aec-config="$threads $x_channel_count $y_channel_count $phase_count $sf_phase_count"
    build_error=$?
    popd
    return $build_error
}

#setup_env
build
build_error=$?
exit $build_error
