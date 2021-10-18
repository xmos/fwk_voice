#!/bin/bash
set -e

DISTDIR=dist

mkdir -p ${DISTDIR}

echo '******************************************************'
echo '* Building USB Mics configuration'
echo '******************************************************'
#NOTE: The defines need to be separated by a colon and not whitespace
(make distclean)
(make -j WW=amazon APP_CONF_DEFINES='-DappconfUSB_ENABLED=1\;-DappconfUSB_AUDIO_MODE=appconfUSB_AUDIO_TESTING\;-DappconfMIC_SRC_DEFAULT=appconfMIC_SRC_USB')
(cp bin/sw_avona.xe ${DISTDIR}/sw_avona_TEST_USB_MICS.xe)

