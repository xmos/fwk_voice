#ifndef __VNR_DEFINES_H__
#define __VNR_DEFINES_H__

/**
 * @page page_vnr_defines_h vnr_defines.h
 * 
 * This header contains the lib_vnr public #defines that are common to both feature extraction and inference.
 *
 * @ingroup vnr_header_file
 */

/**
 * @defgroup vnr_defines   VNR #define constants common to both feature extraction and inference
 */ 

/** Number of filters in the MEL filterbank used in the VNR feature extraction.
 * @ingroup vnr_defines
 */
#define VNR_MEL_FILTERS (24)

/** Number of frames that make up a full set of features for the inference to run on.
 * @ingroup vnr_defines
 */
#define VNR_PATCH_WIDTH (4)

#endif

