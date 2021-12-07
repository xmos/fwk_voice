#!/bin/bash
set -e

AVONA_DIR=applications/avona
DIST_DIR=applications/dist

mkdir -p ${DIST_DIR}

echo '******************************************************'
echo '* Building Avona UA configuration'
echo '******************************************************'
#NOTE: The defines need to be separated by a colon and not whitespace
(cd ${AVONA_DIR}; make distclean)
(cd ${AVONA_DIR}; make -j WW=0 APP_CONF_DEFINES='-DappconfI2S_ENABLED=1\;-DappconfUSB_ENABLED=1\;-DappconfUSB_AUDIO_MODE=appconfUSB_AUDIO_RELEASE')
(mv ${AVONA_DIR}/bin/sw_avona.xe ${DIST_DIR}/sw_avona_UA.xe)

echo '******************************************************'
echo '* Building INT configuration'
echo '******************************************************'
#NOTE: The defines need to be separated by a colon and not whitespace
(cd ${AVONA_DIR}; make distclean)
(cd ${AVONA_DIR}; make -j WW=0 APP_CONF_DEFINES='-DappconfI2S_ENABLED=1\;-DappconfUSB_ENABLED=0')
(mv ${AVONA_DIR}/bin/sw_avona.xe ${DIST_DIR}/sw_avona_INT.xe)
