::*************************************************
:: This batch file creates an Avona fat filesystem.
::*************************************************

@echo off

:: Check for fat.fs already existing
if exist "%~dp0\fat.fs" (
    :: Exit with error
    echo.
    echo fat.fs already exists!
    pause
) else (
    :: Create directory for intended files and Copy renamed files into directory
    if exist "%temp%\fatmktmp\" (
        :: Exit with error
        echo.
        echo fatmktmp\ directory already exists at %temp%
        echo Please delete and retry.
        pause
    ) else (
        mkdir %temp%\fatmktmp
        cp "%WW_PATH%\models\common\WR_250k.en-US.alexa.bin" %temp%\fatmktmp\250kenUS.bin
        cp "%WW_PATH%\models\common\WS_50k.en-US.alexa.bin" %temp%\fatmktmp\50kenUS.bin

        :: Run fatfs_mkimage.exe on the directory to create filesystem file
        start ..\..\host\fatfs\fatfs_mkimage.exe --input=%temp%\fatmktmp --output=fat.fs

        echo Filesystem created. Deleting temp files . . .
        :: File fat.fs is also deleted in cleanup without this:
        sleep 1

        :: Cleanup
        rm -rf %temp%\fatmktmp
    )
)