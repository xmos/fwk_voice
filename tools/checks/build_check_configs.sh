#!/bin/bash
set -e

AVONA_DIR=applications/avona
DIST_DIR=applications/dist

mkdir -p ${DIST_DIR}

echo '******************************************************'
echo '* Building USB Mics configuration'
echo '******************************************************'
#NOTE: The defines need to be separated by a colon and not whitespace
(cd ${AVONA_DIR}; make distclean)
(cd ${AVONA_DIR}; make -j WW=amazon APP_CONF_DEFINES='-DappconfUSB_ENABLED=1\;-DappconfUSB_AUDIO_MODE=appconfUSB_AUDIO_TESTING\;-DappconfMIC_SRC_DEFAULT=appconfMIC_SRC_USB')
(mv ${AVONA_DIR}/bin/sw_avona.xe ${DIST_DIR}/sw_avona_TEST_USB_MICS.xe)

