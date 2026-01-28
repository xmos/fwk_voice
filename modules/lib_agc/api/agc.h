// Copyright 2022 XMOS LIMITED.
// This Software is subject to the terms of the XMOS Public Licence: Version 1.
#ifndef AGC_API_H
#define AGC_API_H

#include "xmath/xmath.h"
#include <agc_profiles.h>

/**
 * @page page_agc_h agc.h
 *
 * This header should be included in application source code to gain access to the
 * lib_agc public functions API.
 */

/**
 * @defgroup agc_func   AGC API functions
 * @defgroup agc_defs   AGC API structure definitions
 */

/**
 * @brief Length of the frame of data on which the AGC will operate.
 *
 * @ingroup agc_defs
 */
#define AGC_FRAME_ADVANCE 240u

/**
 * @brief AGC configuration structure
 *
 * This structure contains configuration settings that can be changed to alter the
 * behaviour of the AGC instance.
 *
 * Members with the "lc_" prefix are parameters for the Loss Control feature.
 *
 * @ingroup agc_defs
 */
typedef struct {
    /** Boolean to enable AGC adaption; if enabled, the gain to apply will adapt based on the
     *  peak of the input frame and the upper/lower threshold parameters. */
    int adapt;
    /** Boolean to enable adaption based on the VNR meta-data; if enabled, adaption will always
     *  be performed when voice activity is detected. This must be disabled if the application
     *  doesn't have a VNR. */
    int adapt_on_vnr;
    /** VNR threshold for voice activity detection. A higher value will
     *  only adapt the AGC on clean speech. A lower value will adapt the
     *  AGC on noisy speech, but may also adapt to more non-speech signals. */
    float_s32_t vnr_threshold;
    /** Boolean to enable soft-clipping of the output frame. */
    int soft_clipping;
    /** The current gain to be applied, not including loss control. When
     * `adapt` is false, this gain will be applied to every frame. When
     * `adapt` is true, the initial value of this gain will be applied to
     * the first frame and then it will be adapted on subsequent frames.*/
    float_s32_t gain;
    /** The maximum gain allowed when adaption is enabled. This can be
     *  used to prevent the AGC amplifying very quiet signals.*/
    float_s32_t max_gain;
    /** The minimum gain allowed when adaption is enabled. */
    float_s32_t min_gain;
    /** The target maximum peak level of the AGC output. If the AGC output
     *  goes above this level, the gain is reduced. */
    float_s32_t upper_threshold;
    /** The target minimum peak level of the AGC output. If the AGC output
     *  goes below this level, the gain is increased. */
    float_s32_t lower_threshold;
    /** Factor by which to increase the gain during adaption. */
    float_s32_t gain_inc;
    /** Factor by which to decrease the gain during adaption. */
    float_s32_t gain_dec;
    /** Number of frames to mute the output at startup. */
    uint32_t startup_delay;
    /** Boolean to enable loss control. The loss control applies additional
     *  attenuation when there is no near end speech. This must be disabled
     *  if the application doesn't have an AEC or VNR. */
    int lc_enabled;
    /** Number of frames required to consider far-end audio active. */
    int lc_n_frame_far;
    /** Number of frames required to consider near-end audio active. */
    int lc_n_frame_near;
    /** Threshold for far-end correlation above which to indicate far-end activity only. */
    float_s32_t lc_corr_threshold;
    /** Gamma coefficient for estimating the power of the far-end background noise. */
    float_s32_t lc_bg_power_gamma;
    /** Factor by which to increase the loss control gain when less than target value. */
    float_s32_t lc_gamma_inc;
    /** Factor by which to decrease the loss control gain when greater than target value. */
    float_s32_t lc_gamma_dec;
    /** Delta multiplier used when only far-end activity is detected. */
    float_s32_t lc_far_delta;
    /** Delta multiplier used when only near-end activity is detected.
     *  How many times louder the near-end signal must be than the background
     *  noise when there is no far-end playback. If the
     *  near end speech is not heard during silence, reduce this value. If
     *  too much non-speech background noise is heard, increase this value. */
    float_s32_t lc_near_delta;
    /** Delta multiplier used when both near-end and far-end activity is
     *  detected. How many times louder the near end signal must be
     *  above the residual far-end speech (after the AEC) to be detected
     *  during double talk. If the near end speech is not heard during
     *  double talk, reduce this value. If there is too much breakthrough
     *  of residual far-end echo when there is no near-end speech present,
     *  increase this value. */
    float_s32_t lc_near_delta_far_active;
    /** Loss control gain to apply when near-end activity only is detected. */
    float_s32_t lc_gain_max;
    /** Loss control gain to apply when double-talk is detected. Reducing this
     *  value will reduce the level of the near-end speech during double-talk,
     *  but may help to reduce the level of residual far-end echo that is heard. */
    float_s32_t lc_gain_double_talk;
    /** Loss control gain to apply when silence is detected. */
    float_s32_t lc_gain_silence;
    /** Loss control gain to apply when far-end activity only is detected. */
    float_s32_t lc_gain_min;
    /** Low VNR threshold for background estimation. */
    float_s32_t lc_vnr_low;
    /** Frame count limit for low VNR detection. */
    int lc_vnr_low_count_limit;

} agc_config_t;

/**
 * @brief AGC state structure
 *
 * This structure holds the current state of the AGC instance and members are updated each
 * time that `agc_process_frame()` runs. Many of these members are exponentially-weighted
 * moving averages (EWMA) which influence the adaption of the AGC gain or the loss control
 * feature. The user should not directly modify any of these members, except the config.
 *
 * @ingroup agc_defs
 */
typedef struct {
    /** The current configuration of the AGC. Any member of this configuration structure can
     * be modified and that change will take effect on the next run of `agc_process_frame()`. */
    agc_config_t config;
    /** EWMA of the frame peak, which is used to identify the overall trend of a rise or fall
     * in the input signal. */
    float_s32_t x_slow;
    /** EWMA of the frame peak, which is used to identify a rise or fall in the peak of frame. */
    float_s32_t x_fast;
    /** EWMA of `x_fast`, which is used when adapting to the `agc_config_t::upper_threshold`. */
    float_s32_t x_peak;
    /** Timer counting down until enough frames with far-end activity have been processed. */
    int lc_t_far;
    /** Timer counting down until enough frames with near-end activity have been processed. */
    int lc_t_near;
    /** EWMA of estimates of the near-end power. */
    float_s32_t lc_near_power_est;
    /** EWMA of estimates of the far-end power. */
    float_s32_t lc_far_power_est;
    /** EWMA of estimates of the power of near-end background noise. */
    float_s32_t lc_near_bg_power_est;
    /** Loss control gain applied on top of the AGC gain in `agc_config_t`. */
    float_s32_t lc_gain;
    /** EWMA of estimates of the power of far-end background noise. */
    float_s32_t lc_far_bg_power_est;
    /** EWMA of the far-end correlation for detecting double-talk. */
    float_s32_t lc_corr_val;
    /** Counter of how many frames the VNR has been low for. */
    int lc_vnr_low_count;
    /** Frame counter since initialisation, used for startup delay */
    uint32_t frame_count;
} agc_state_t;

/**
 * @brief Initialise the AGC
 *
 * This function initialises the AGC state with the provided configuration. It must be called
 * at startup to initialise the AGC before processing any frames, and can be called at any time
 * after that to reset the AGC instance, returning the internal AGC state to its defaults.
 *
 * @param[out] agc       AGC state structure
 * @param[in]  config    Initial configuration values
 *
 * @par Example with an unmodified profile
 * @code{.c}
 *      agc_state_t agc;
        agc_init(&agc, &AGC_PROFILE_ASR);
 * @endcode
 *
 * @par Example with modification to the profile
 * @code{.c}
 *      agc_config_t conf = AGC_PROFILE_FIXED_GAIN;
        conf.gain = f32_to_float_s32(100);
        agc_state_t agc;
        agc_init(&agc, &conf);
 * @endcode
 *
 * @ingroup agc_func
 */
void agc_init(agc_state_t *agc, agc_config_t *config);

/**
 * @brief AGC meta data structure
 *
 * This structure holds meta-data about the current frame to be processed, and must be updated
 * to reflect the current frame before calling `agc_process_frame()`.
 *
 * @ingroup agc_defs
 */
typedef struct {
    /** Estimated voice-to-noise ratio in the current frame. */
    float_s32_t vnr_flag;
    /** The power of the most powerful reference channel. */
    float_s32_t aec_ref_power;
    /** Correlation factor between the microphone input and the AEC's estimated microphone
     *  signal. */
    float_s32_t aec_corr_factor;
    /** Flag to indicate if the reference signal is currently active */
    int32_t ref_active_flag;
} agc_meta_data_t;


/**
 * @brief Initialise AGC meta data structure
 * 
 * Set the AGC meta data values to indicate no VNR, no AEC and no reference signal.
 * 
 * @ingroup agc_func
 */
agc_meta_data_t agc_meta_data_init();

/**
 * If the application has no VNR, `adapt_on_vnr` must be disabled in the configuration. This
 * pre-processor definition can be assigned to the `vnr_flag` in `agc_meta_data_t` in that
 * situation to make it clear in the code that there is no VNR.
 *
 * @ingroup agc_defs
 */
#define AGC_META_DATA_NO_VNR (float_s32_t){0, 0}

/**
 * If the application has VNR, `adapt_on_vnr` can be enabled in the configuration. This
 * define is used to covert VNR value from uint8_t to boolean.
 *
 */
#define AGC_VNR_THRESHOLD 205

/**
 * If the application has no AEC, `lc_enabled` must be disabled in the configuration. This
 * pre-processor definition can be assigned to the `aec_ref_power` and `aec_corr_factor` in
 * `agc_meta_data_t` in that situation to make it clear in the code that there is no AEC.
 *
 * @ingroup agc_defs
 */
#define AGC_META_DATA_NO_AEC (float_s32_t){0, 0}


/**
 * If the application has no reference signal, this pre-processor definition can be assigned to the
 * `ref_active_flag` in `agc_meta_data_t` to make it clear in the code that there is no reference
 * signal.
 *
 * @ingroup agc_defs
 */
#define AGC_META_DATA_NO_REF 0

/**
 * @brief Perform AGC processing on a frame of input data
 *
 * This function updates the AGC's internal state based on the input frame and meta-data, and
 * returns an output containing the result of the AGC algorithm applied to the input.
 *
 * The `input` and `output` pointers can be equal to perform the processing in-place.
 *
 * @param[inout] agc      AGC state structure
 * @param[out] output     Array to return the resulting frame of data
 * @param[in] input       Array of frame data on which to perform the AGC
 * @param[in] meta_data   Meta-data structure with VNR/AEC data
 *
 * @par Example
 * @code{.c}
 *      int32_t input[AGC_FRAME_ADVANCE];
        int32_t output[AGC_FRAME_ADVANCE];
        agc_meta_data_t md = agc_meta_data_init();
        agc_process_frame(&agc, output, input, &md);
 * @endcode
 *
 * @ingroup agc_func
 */
void agc_process_frame(agc_state_t *agc,
                       int32_t output[AGC_FRAME_ADVANCE],
                       const int32_t input[AGC_FRAME_ADVANCE],
                       agc_meta_data_t *meta_data);

#endif
