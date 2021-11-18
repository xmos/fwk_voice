#!/bin/sh

if [ -e fat.fs ] ; then
    echo "fat.fs already exists!"
else
    if [ -e fat_mnt ] ; then
        echo "fat_mnt directory already exists!"
    else
        # Create directory for intended files and Copy renamed files into directory
        sudo mkdir -p fat_mnt/ww
        sudo cp "$WW_PATH/models/common/WR_250k.en-US.alexa.bin" fat_mnt/ww/250kenUS.bin
        sudo cp "$WW_PATH/models/common/WS_50k.en-US.alexa.bin" fat_mnt/ww/50kenUS.bin

        # Create env var for path to fatfs_mkimage?
        FATFS_MKIMAGE_PATH=../../host/fatfs

        # Run fatfs_mkimage.exe on the directory to create filesystem file
        sudo $FATFS_MKIMAGE_PATH/fatfs_mkimage --input=fat_mnt/ww --output=fat.fs
    fi
fi
