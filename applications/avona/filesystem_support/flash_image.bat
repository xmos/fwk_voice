::*************************************************************************
:: This batch file creates an Avona fat filesystem and flashes the hardware
:: using xflash.
::*************************************************************************

@echo off

echo Creating filesystem . . .
call create_fs.bat

echo Flashing firmware and filesystem . . .
xflash --quad-spi-clock 50MHz --factory ../bin/sw_avona.xe --boot-partition-size 0x100000 --data ./fat.fs

echo.
echo xflash complete!