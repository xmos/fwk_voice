./create_fs.sh
xflash --quad-spi-clock 50MHz --factory ../bin/sw_xvf3652.xe --boot-partition-size 0x100000 --data ./fat.fs
