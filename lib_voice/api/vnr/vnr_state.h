#ifndef __vnr_state_H__
#define __vnr_state_H__

#include "vnr_defines.h"
#include "xmath/xmath.h"

/**
 * @page page_vnr_state_h vnr_state.h
 * 
 * This header contains lib_vnr public data structure definitions 
 *
 * @ingroup vnr_header_file
 */

/**
 * @defgroup vnr_state   VNR data structure definitions
 */ 

/**
 * @brief VNR form_input state structure
 *
 * @ingroup vnr_state
 */
typedef struct {
    /** Previous frame time domain input samples which are combined with VNR_FRAME_ADVANCE new samples to form the VNR input frame. */
    int32_t DWORD_ALIGNED prev_input_samples[VNR_PROC_FRAME_LENGTH - VNR_FRAME_ADVANCE];
}vnr_input_state_t;

/**
 * @brief VNR feature extraction config structure
 *
 * @ingroup vnr_state
 */
typedef struct {
    /** Enable highpass filtering of VNR MEL filter output. Disabled by default*/
    int32_t enable_highpass;
}vnr_feature_config_t;

/**
 * @brief State structure used in VNR feature extraction
 *
 * @ingroup vnr_state
 */
typedef struct {
    /** Feature buffer containing the most recent VNR_MEL_FILTERS frames' MEL frequency spectrum. */
    int32_t DWORD_ALIGNED feature_buffers[VNR_PATCH_WIDTH][VNR_MEL_FILTERS];
    vnr_feature_config_t config;
}vnr_feature_state_t;

/**
 * @brief State structure used for the VNR
 *
 * @ingroup vnr_state
 */
typedef struct {
    /** VNR Input state */
    vnr_input_state_t input_state;
    /** VNR Feature state */
    vnr_feature_state_t feature_state;
} vnr_state_t;

#endif
