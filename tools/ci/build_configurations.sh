#!/bin/bash
set -e

DISTDIR=dist

mkdir -p ${DISTDIR}

echo '******************************************************'
echo '* Building UA configuration'
echo '******************************************************'
(make distclean all WW=0 appconfI2S_ENABLED=1 appconfUSB_ENABLED=1)
(cp bin/sw_xvf3652.xe ${DISTDIR}/sw_xvf3652_UA.xe)


echo '******************************************************'
echo '* Building INT configuration'
echo '******************************************************'
(make distclean all WW=0 appconfI2S_ENABLED=1 appconfUSB_ENABLED=0 appconfUSB_AUDIO_MODE=0)
(cp bin/sw_xvf3652.xe ${DISTDIR}/sw_xvf3652_INT.xe)
