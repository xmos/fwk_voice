#!/bin/sh

if [ -e fat.fs ] ; then
    echo "fat.fs already exists!"
else
    # Create directory for intended files and Copy renamed files into directory
    tmp_dir=$(mktemp -d)
    cp "$WW_PATH/models/common/WR_250k.en-US.alexa.bin" $tmp_dir/250kenUS.bin
    cp "$WW_PATH/models/common/WS_50k.en-US.alexa.bin" $tmp_dir/50kenUS.bin

    # Create env var for path to fatfs_mkimage?
    FATFS_MKIMAGE_PATH=../../host/fatfs

    # Run fatfs_mkimage.exe on the directory to create filesystem file
    $FATFS_MKIMAGE_PATH/fatfs_mkimage --input=$tmp_dir --output=fat.fs
fi
