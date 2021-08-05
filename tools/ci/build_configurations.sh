#!/bin/bash
set -e

DISTDIR=dist

mkdir -p ${DISTDIR}

echo '******************************************************'
echo '* Building UA configuration'
echo '******************************************************'
(make distclean all WW=0 CMAKE_ARGS="-DappconfI2S_ENABLED=1 -DappconfUSB_ENABLED=1 -DappconfUSB_AUDIO_MODE=0 -DCMAKE_VERBOSE_MAKEFILE:BOOL=ON")
(cp bin/sw_xvf3652.xe ${DISTDIR}/sw_xvf3652_UA.xe)


echo '******************************************************'
echo '* Building INT configuration'
echo '******************************************************'
(make distclean all WW=0 CMAKE_ARGS="-DappconfI2S_ENABLED=1 -DappconfUSB_ENABLED=0 -DCMAKE_VERBOSE_MAKEFILE:BOOL=ON")
(cp bin/sw_xvf3652.xe ${DISTDIR}/sw_xvf3652_INT.xe)
