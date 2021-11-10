// Copyright (c) 2017-2020, XMOS Ltd, All rights reserved
#ifndef _AGC_H_
#define _AGC_H_

#include <xccompat.h>
#include "dsp.h"
#include "vtb_q_format.h"
#include "agc_ch_state.h"


/**
 * Structure containing AGC state unique to each input channel.
 */
typedef struct {
    agc_ch_state_t ch_state[AGC_INPUT_CHANNELS]; ///< Channel states
} agc_state_t;


/**
 * Structure containing AGC configuration unique to each input channel.
 * Used to initialise AGC channel state.
 */
typedef struct {
    int adapt;                      ///< 0 for fixed gain, or 1 for AGC.
    vtb_uq16_16_t init_gain;        ///< Initial channel gain. Linear UQ16_16.
    vtb_uq16_16_t max_gain;         ///< Maximum channel gain. Linear UQ16_16.
    vtb_uq1_31_t upper_threshold;   ///< Upper threshold for desired output voice level [0, INT32_MAX].
    vtb_uq1_31_t lower_threshold;   ///< Lower threshold for desired output voice level [0, INT32_MAX].
    vtb_uq16_16_t gain_inc;         ///< Step value to increment the channel gain.
    vtb_uq16_16_t gain_dec;         ///< Step value to decrement the channel gain.
    int lc_enabled;
} agc_ch_init_config_t;

/**
 * Structure containing AGC configuration
 * Used to initialise AGC state.
 */
typedef struct {
    agc_ch_init_config_t ch_init_config[AGC_INPUT_CHANNELS];
} agc_init_config_t;

/** Initialise AGC state given an initial configuration.
 *
 * \param[out] agc          AGC state to be initialised.
 *
 * \param[in] config        Array containing AGC configuration for each channel.
 *                          Must be of length AGC_INPUT_CHANNELS.
 */
void agc_init(REFERENCE_PARAM(agc_state_t, agc), agc_init_config_t config);


/** Set AGC channel gain.
 *
 * \param[in,out] agc       AGC state.
 *
 * \param[in] ch_index      Channel index. Must be less than AGC_INPUT_CHANNELS.
 *
 * \param[in] gain          Gain value in linear UQ16_16 format.
 */
void agc_set_ch_gain(REFERENCE_PARAM(agc_state_t, agc), unsigned ch_index, vtb_uq16_16_t gain);


/** Get AGC channel gain.
 *
 * \param[in] agc           AGC state.
 *
 * \param[in] ch_index      Channel index. Must be less than AGC_INPUT_CHANNELS.
 *
 * \returns                 Channel gain in linear UQ16_16 format.
 */
vtb_uq16_16_t agc_get_ch_gain(agc_state_t agc, unsigned ch_index);


/** Set AGC channel gain increase value.
 *
 * \param[in,out] agc       AGC state.
 *
 * \param[in] ch_index      Channel index. Must be less than AGC_INPUT_CHANNELS.
 *
 * \param[in] gain_inc      Gain increase value in linear UQ16_16 format.
 *                          Must be greater than 1.
 */
void agc_set_ch_gain_inc(REFERENCE_PARAM(agc_state_t, agc), unsigned ch_index, vtb_uq16_16_t gain_inc);


/** Get AGC channel gain increase value.
 *
 * \param[in] agc           AGC state.
 *
 * \param[in] ch_index      Channel index. Must be less than AGC_INPUT_CHANNELS.
 *
 * \returns                 Channel gain_inc in linear UQ16_16 format.
 */
vtb_uq16_16_t agc_get_ch_gain_inc(agc_state_t agc, unsigned ch_index);


/** Set AGC channel gain decrease value.
 *
 * \param[in,out] agc       AGC state.
 *
 * \param[in] ch_index      Channel index. Must be less than AGC_INPUT_CHANNELS.
 *
 * \param[in] gain_dec      Gain decrease value in linear UQ16_16 format.
 *                          Must be between 0 and 1.
 */
void agc_set_ch_gain_dec(REFERENCE_PARAM(agc_state_t, agc), unsigned ch_index, vtb_uq16_16_t gain_dec);


/** Get AGC channel gain decrease value.
 *
 * \param[in] agc           AGC state.
 *
 * \param[in] ch_index      Channel index. Must be less than AGC_INPUT_CHANNELS.
 *
 * \returns                 Channel gain_dec in linear UQ16_16 format.
 */
vtb_uq16_16_t agc_get_ch_gain_dec(agc_state_t agc, unsigned ch_index);


/** Set AGC channel max gain.
 *
 * \param[in,out] agc       AGC state.
 *
 * \param[in] ch_index      Channel index. Must be less than AGC_INPUT_CHANNELS.
 *
 * \param[in] max_gain      Max gain value in linear UQ16_16 format.
 */
void agc_set_ch_max_gain(REFERENCE_PARAM(agc_state_t, agc), unsigned ch_index,
        vtb_uq16_16_t max_gain);


/** Get AGC channel max gain.
 *
 * \param[in] agc           AGC state.
 *
 * \param[in] ch_index      Channel index. Must be less than AGC_INPUT_CHANNELS.
 *
 * \returns                 Channel max gain in linear UQ16_16 format.
 */
vtb_uq16_16_t agc_get_ch_max_gain(agc_state_t agc, unsigned ch_index);


/** Set AGC channel adapt flag.
 *
 * \param[in,out] agc       AGC state.
 *
 * \param[in] ch_index      Channel index. Must be less than AGC_INPUT_CHANNELS.
 *
 * \param[in] adapt         AGC adapt flag: 0 for fixed gain, 1 for adapt.
 */
void agc_set_ch_adapt(REFERENCE_PARAM(agc_state_t, agc), unsigned ch_index,  uint32_t adapt);


/** Get AGC channel adapt flag.
 *
 * \param[in] agc           AGC state.
 *
 * \param[in] ch_index      Channel index. Must be less than AGC_INPUT_CHANNELS.
 *
 * \returns                 0 for fixed gain, 1 for adapt.
 */
int agc_get_ch_adapt(agc_state_t agc, unsigned ch_index);


/** Set AGC channel loss control enabled flag.
 *
 * \param[in,out] agc       AGC state.
 *
 * \param[in] ch_index      Channel index. Must be less than AGC_INPUT_CHANNELS.
 *
 * \param[in] enable        AGC LC enable flag: 0 for disabled, 1 for enabled.
 */
void agc_set_ch_lc_enable(REFERENCE_PARAM(agc_state_t, agc), unsigned ch_index,  uint32_t enable);


/** Get AGC channel loss control enabled flag.
 *
 * \param[in] agc           AGC state.
 *
 * \param[in] ch_index      Channel index. Must be less than AGC_INPUT_CHANNELS.
 *
 * \returns                 0 for disabled, 1 for enabled.
 */
int agc_get_ch_lc_enable(agc_state_t agc, unsigned ch_index);


/** Set upper threshold of desired output voice level for AGC channel.
 *
 * \param[in,out] agc           AGC state.
 *
 * \param[in] ch_index          Channel index. Must be less than
 *                              AGC_INPUT_CHANNELS.
 *
 * \param[in] upper_threshold   Upper threshold of desired output voice level
 *                              [0, INT32_MAX].
 */
void agc_set_ch_upper_threshold(REFERENCE_PARAM(agc_state_t, agc), unsigned ch_index,
        int32_t upper_threshold);


/** Set lower threshold of desired output voice level for AGC channel.
 *
 * \param[in,out] agc           AGC state.
 *
 * \param[in] ch_index          Channel index. Must be less than
 *                              AGC_INPUT_CHANNELS.
 *
 * \param[in] lower_threshold   Lower threshold of desired output voice level
 *                              [0, INT32_MAX].
 */
void agc_set_ch_lower_threshold(REFERENCE_PARAM(agc_state_t, agc), unsigned ch_index,
        int32_t lower_threshold);


/** Get upper threshold of desired output voice level for AGC channel.
 *
 * \param[in] agc           AGC state.
 *
 * \param[in] ch_index      Channel index. Must be less than AGC_INPUT_CHANNELS.
 *
 * \returns                 Lower threshold of esired output voice level for AGC channel.
 */
int32_t agc_get_ch_upper_threshold(agc_state_t agc, unsigned ch_index);


/** Get lower threshold of desired output voice level for AGC channel.
 *
 * \param[in] agc           AGC state.
 *
 * \param[in] ch_index      Channel index. Must be less than AGC_INPUT_CHANNELS.
 *
 * \returns                 Lower threshold of desired output voice level for AGC channel.
 */
int32_t agc_get_ch_lower_threshold(agc_state_t agc, unsigned ch_index);



/** Process a multi-channel frame of time-domain sample data.
 *
 * \param[in,out] agc           AGC state.
 *
 * \param[in,out] frame_in_out  On input this array contains the sample data.
 *                              On output this array contains the data with AGC
 *                              applied.
 *
 * \param[in] far_power         Frame power of reference audio.
 *
 * \param[in] vad_flag          VAD flag for input sample data. Non-zero indicates
 *                              that the sample data contains voice activity.
 *
 * \param[in] aec_corr          AEC correlation value. A value close to 1 indicates
 *                              that the mic energy is dominated by reference audio.
 */
void agc_process_frame(REFERENCE_PARAM(agc_state_t, agc),
        vtb_ch_pair_t frame_in_out[AGC_CHANNEL_PAIRS][AGC_PROC_FRAME_LENGTH],
        vtb_u32_float_t far_power,
        int vad_flag,
        vtb_uq0_32_t aec_corr);


#endif // _AGC_H_
