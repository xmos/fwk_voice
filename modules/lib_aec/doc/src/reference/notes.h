
// This file exists as a compatibility work-around between vanilla Doxygen and 
// sphinx+breathe+doxygen.
// If these notes are written in an .md file, sphinx can't interpret them. If they're 
// written in an .rst file, Doxygen can't interpret them and can't build link references 
// to them.

/**
 * @page note_implementation_details Note: Implementation Details
 * 
 * The lib_aec library provides functions that can be put together to perform Automatic Echo Cancellation on input
 * microphone data by using input reference data to model the echo characteristics of the room.
 *
 * AEC is performed on a frame by frame basis where the frame size is AEC_FRAME_ADVANCE samples.
 * Before starting frame processing, the user needs to call aec_init() to initialise the AEC for a desired
 * configuration.
 * 
 */
