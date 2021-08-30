#!/bin/bash

echo "creating file system..."
./create_fs.sh
echo "flashing firmware and filesystem..."
xflash --quad-spi-clock 50MHz --factory ../bin/sw_xvf3652.xe --boot-partition-size 0x100000 --data ./fat.fs
echo "xflash complete!"

