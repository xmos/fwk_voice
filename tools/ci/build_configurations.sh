#!/bin/bash
set -e

DISTDIR=dist

mkdir -p ${DISTDIR}

echo '******************************************************'
echo '* Building UA configuration'
echo '******************************************************'
#NOTE: The defines need to be separated by a colon and not whitespace
export APP_CONF_DEFINES="-DappconfI2S_ENABLED=1;-DappconfUSB_ENABLED=1;-DappconfUSB_AUDIO_MODE=0"
(make distclean all WW=0)
(cp bin/sw_xvf3652.xe ${DISTDIR}/sw_xvf3652_UA.xe)

echo '******************************************************'
echo '* Building INT configuration'
echo '******************************************************'
#NOTE: The defines need to be separated by a colon and not whitespace
export APP_CONF_DEFINES="-DappconfI2S_ENABLED=1;-DappconfUSB_ENABLED=0"
(make distclean all WW=0)
(cp bin/sw_xvf3652.xe ${DISTDIR}/sw_xvf3652_INT.xe)
