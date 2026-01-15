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

/** @brief Time domain samples block length used internally in VNR DFT computation. 
 * NOT USER MODIFIABLE.
 *
 * @ingroup vnr_defines
 */
#define VNR_PROC_FRAME_LENGTH (512)

/** @brief VNR new samples frame size
 * This is the number of samples of new data that the VNR processes every frame. 240 samples at 16kHz is 15msec.
 * NOT USER MODIFIABLE.
 *
 * @ingroup vnr_defines
 */
#define VNR_FRAME_ADVANCE (240)

/** Number of bins of spectrum data computed when doing a DFT of a VNR_PROC_FRAME_LENGTH length time domain vector. The
 * VNR_FD_FRAME_LENGTH spectrum values represent the bins from DC to Nyquist. NOT USER MODIFIABLE.
 *
 * @ingroup vnr_defines
 */   
#define VNR_FD_FRAME_LENGTH ((VNR_PROC_FRAME_LENGTH/2)+1)

/** Number of filters in the MEL filterbank used in the VNR feature extraction.
 * @ingroup vnr_defines
 */
#define VNR_MEL_FILTERS (24)

/** Number of frames that make up a full set of features for the inference to run on.
 * @ingroup vnr_defines
 */
#define VNR_PATCH_WIDTH (4)

#endif

