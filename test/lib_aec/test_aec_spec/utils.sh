#!/bin/bash

config_filename="$(pwd)/parameters.cfg"

read_config() {
    line=$(cat $config_filename | grep "\b$1\b")
    echo $(echo $line | sed "s/.* *= *//g")
}

setup_env() {
    pushd ../../../infr_scripts_pl/Build/
    source SetupEnv
    popd
}
