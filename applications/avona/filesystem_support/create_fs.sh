#!/bin/sh

# Create directory for intended files and Copy renamed files into directory
tmp_dir=$(mktemp -d)
ww_dir=$tmp_dir/ww
mkdir -p $ww_dir
cp "$WW_PATH/models/common/WR_250k.en-US.alexa.bin" $ww_dir/250kenUS.bin
cp "$WW_PATH/models/common/WS_50k.en-US.alexa.bin" $ww_dir/50kenUS.bin

# Create env var for path to fatfs_mkimage?
FATFS_MKIMAGE_PATH=/opt/xmos/SDK/0.11.1/bin/

# Run fatfs_mkimage.exe on the directory to create filesystem file
$FATFS_MKIMAGE_PATH/fatfs_mkimage --input=$tmp_dir --output=fat.fs
