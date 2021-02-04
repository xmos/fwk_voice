// Copyright (c) 2018-2020, XMOS Ltd, All rights reserved
#ifndef VTB_TX_RX_H_
#define VTB_TX_RX_H_

#include <xccompat.h>
#include <stdint.h>
#include "vtb_ch_pair.h"
#include "vtb_float.h"

// Used internally.
#define VTB_STATE_DELAY_START  4

#define VTB_TX_RX_CHUNK_SIZE 32

#define VTB_TX_RX_DONE 1
#define VTB_TX_RX_BUSY 0

#ifndef VTB_MD_SIZE
#define VTB_MD_SIZE 4
#endif

#ifdef __XC__
extern "C" {
#endif

typedef struct {
    // Chunked state
    unsigned chunk_size;
    unsigned sample_index;
    uint8_t transfer_status;

    // Parameters
    unsigned frame_advance;
    unsigned num_ch;
} vtb_tx_state_t;

typedef struct {
    // Chunked state
    unsigned chunk_size;
    unsigned sample_index;
    uint8_t transfer_status;

    // Buffers
    vtb_ch_pair_t * input_frame; // FRAME_ADVANCE
    vtb_ch_pair_t * prev_frame;  // PROC_FRAME_LENGTH - FRAME_ADVANCE
    int32_t * delay_buffer;      // sum(delays)

    // Parameters
    unsigned frame_advance;
    unsigned proc_frame_length;
    unsigned num_ch;
    unsigned * delays; // Array[num_ch] = {delay in samples for each channel}
    unsigned delayed; // Set to 1 in form_rx_state if sum(delays) != 0
} vtb_rx_state_t;

#ifdef __XC__
}
#endif


/** Macro that calculates the required size (one dimension) of the rx state array.
 *
 * \param[in]      num_ch       No. of channels (not channel pairs) in
 *                              the data to be received.
 *
 * \param[in]     frame_size    The number of samples that are processed by this
 *                              DSP block. The rx function will group the samples
 *                              together in a [channels][frame_size] array.
 *                              This must be a multiple of four.
 *
 * \param[in]     advance       The number of samples that the transmitter sends
 *                              on every iteration. Must be smaller than frame_size.
 *                              This must be a multiple of four.
 *
 * \param[in]     total_delay   The total delay, summed across all channels.
 */
#define VTB_RX_STATE_UINT64_SIZE(num_ch, frame_size, advance, total_delay) \
            ((((num_ch+1)/2)*2 * (frame_size))/2 \
            + ((total_delay)+1)/2 \
            + (((num_ch+1)/2)*2)*2 + VTB_STATE_DELAY_START/2) + 1 // +1 for alignment padding


/** Struct for custom metadata. To be used when tx/rx a channel pair frame.
 */
typedef struct {
    uint32_t data[VTB_MD_SIZE]; ///< Metadata. Contents defined by application
} vtb_md_t;


/** Copy the contents of a metadata frame.
 *
 * \param[in]      dest                 Destination metadata.
 *
 * \param[in]      source               Source metadata.
 */
void vtb_md_copy(REFERENCE_PARAM(vtb_md_t, dest), vtb_md_t source);


/** Function that receives pointers to input_frame and prev_frame and combines them to
 * create a processing frame. It also updates the prev_frame buffer.
 *
 * \param[out]     proc_frame           Pointer to the full combined frame of length proc_frame_length.
 *
 * \param[in]      input_frame          Pointer to the new data of length frame_advance.
 *
 * \param[in,out]  prev_frame           Pointer to buffer of previous frame data.
 *
 * \param[in]      frame_advance        No. of elements per channel pair in input_frame.
 *
 * \param[in]      proc_frame_length    No. of elements per channel pair in proc_frame.
 *
 * \param[in]      num_ch               No. of channels (not channel pairs) (proc_frame, input_frame and buffer should have the same no. of channels).
 */
void vtb_form_processing_frame(vtb_ch_pair_t *proc_frame,
        vtb_ch_pair_t *input_frame,
        vtb_ch_pair_t *prev_frame,
        const unsigned frame_advance,
        const unsigned proc_frame_length,
        const unsigned num_ch);


/** Function that receives pointers to input_frame, prev_frame, and delay_buffer
 * and combines them to create a processing frame. It also updates the prev_frame
 * and delay buffers.
 *
 * This function is slower than vtb_form_processing_frame so it is only used
 * when delay is required.
 *
 * \param[out]     proc_frame           Pointer to the full combined frame of length proc_frame_length.
 *
 * \param[in]      input_frame          Pointer to the new data of length frame_advance.
 *
 * \param[in,out]  prev_frame           Pointer to buffer of previous frame data of length proc_frame_length - frame_advance.
 *
 * \param[in,out]  delay_buffer         Pointer to buffer of delayed frame data of length sum(delays).
 *
 * \param[in]      frame_advance        No. of elements per channel pair in input_frame.
 *
 * \param[in]      proc_frame_length    No. of elements per channel pair in proc_frame.
 *
 * \param[in]      num_ch               No. of channels (not channel pairs) (proc_frame, input_frame and buffer should have the same no. of channels).
 *
 * \param[in]      delays               Amount of delay in samples to apply to each channel.
 */
void vtb_form_processing_frame_delayed(
        vtb_ch_pair_t *proc_frame, // PROC_FRAME_LENGTH
        vtb_ch_pair_t *input_frame, // FRAME_ADVANCE
        vtb_ch_pair_t *prev_frame, // PROC_FRAME_LENGTH - FRAME_ADVANCE
        int32_t *delay_buffer, // Variable length
        const unsigned frame_advance,
        const unsigned proc_frame_length,
        const unsigned num_ch,
        const unsigned *delays);


/** Initialises metadata.
 *
 * \param[out]     md    Metadata to be initialised.
 */
void vtb_md_init(REFERENCE_PARAM(vtb_md_t, md));


/** Function that forms the state for transmitting data from a DSP block.
 * This function is called once, before the vtb_tx functions are called in
 * a loop.
 *
 * \param[in]     frame_advance     The number of samples that the transmitter sends
 *                            on every iteration. Must be smaller than frame_size.
 *                            This must be a multiple of four.
 *
 * \param[in]     num_ch      The number of channels that the transmitter sends
 *                            on every iteration. Must be <= CH_PAIRS used for
 *                            size of the input arrays.
 *
 * \return        tx_state    A structure of type vtb_tx_state_t.
 */
vtb_tx_state_t vtb_form_tx_state(
        unsigned frame_advance,
        unsigned num_ch);


/** Function that forms the state for receiving data into a DSP block.
 * This function is called once, before the vtb_rx functions are called in
 * a loop.
 *
 * The functions requires pointers to three arrays of type vtb_ch_pair_t::
 *
 *     vtb_ch_pair_t [[aligned(8)]] input_frame[CH_PAIRS][FRAME_ADVANCE];
 *     vtb_ch_pair_t [[aligned(8)]] proc_frame[CH_PAIRS][PROC_FRAME_LENGTH];
 *     vtb_ch_pair_t [[aligned(8)]] prev_frame[CH_PAIRS][PROC_FRAME_LENGTH - FRAME_ADVANCE];
 *
 * The prev_frame array should be memset to 0.
 *
 * If any of the channels need to be delayed, a delay_buffer and array of delays
 * is required::
 *
 *     int32_t delay_buffer[TOTAL_DELAY];
 *     unsigned delays[NUM_CH_PAIRS * 2];
 *
 * The delays array must be of size NUM_CH_PAIRS * 2 and the delay_buffer size
 * must equal the sum of delays across all channels.
 *
 * \param[in]     input_frame Array as described above.
 *
 * \param[in]     prev_frame  Array as described above.
 *
 * \param[in]     delay_buffer  Array as described above.
 *
 * \param[in]     frame_advance     The number of samples that the transmitter sends
 *                            on every iteration. Must be smaller than frame_size.
 *                            This must be a multiple of four.
 *
 * \param[in]     proc_frame_length  The number of samples that are processed by this
 *                            DSP block. The rx function will group the samples
 *                            together in a [ch_pairs][proc_frame_length] array.
 *                            This must be a multiple of four.
 *
 * \param[in]     num_ch      The number of channels that the transmitter sends
 *                            on every iteration. Must be <= CH_PAIRS used for
 *                            size of the input arrays.
 *
 * \param[in]     delays      An array that specifies a delay for each channel.
 *                            Can be null. If null, no delays are applied (the
 *                            typical case). Each delay must be a multiple of four.
 *
 * \return        rx_state    A structure of type vtb_rx_state_t.
 */
vtb_rx_state_t vtb_form_rx_state(
        vtb_ch_pair_t input_frame[],
        NULLABLE_ARRAY_OF(vtb_ch_pair_t, prev_frame),
        NULLABLE_ARRAY_OF(int32_t, delay_buffer),
        unsigned frame_advance,
        unsigned proc_frame_length,
        unsigned num_ch,
        NULLABLE_ARRAY_OF(unsigned, delays));

/** Function to transmit a frame of size state.frame_advance.
 *  Used in vtb_tx. If called outside of vtb_tx, it allows
 *  other events to be serviced between transfer of data chunks.
 *
 *  This function returns VTB_TX_TX_WAIT if the transfer is incomplete.
 *  It returns VTB_TX_TX_DONE once the transfer has completed.
 *
 * \param         c_out    Channel end to send the data to.
 *
 * \param[in]     state    VTB TX state. Must have been initialised.
 *
 * \param[in]     frame    Array containing channel pair data of size
 *                         `state.frame_advance`
 *
 * \param[in]     md       Metadata.
 *
 * \return        Transfer status: VTB_TX_RX_BUSY or VTB_TX_RX_DONE
 *
 */
int vtb_tx_chunked(chanend c_out,
        REFERENCE_PARAM(vtb_tx_state_t, state),
        vtb_ch_pair_t data[],
        REFERENCE_PARAM(vtb_md_t, md));


/** Function to recieve a frame of size state.frame_advance.
 *  Used in vtb_rx. If called outside of vtb_rx, it allows
 *  other events to be serviced between transfer of data chunks.
 *
 *  This function returns VTB_TX_TX_WAIT if the transfer is incomplete.
 *  It returns VTB_TX_TX_DONE once the transfer has completed.
 *
 * \param         c_out    Channel end to send the data to.
 *
 * \param[in]     state    VTB TX state. Must have been initialised.
 *
 * \param[in]     frame    Array containing channel pair data of size
 *                         `state.frame_advance`
 *
 * \param[in]     md       Metadata.
 *
 * \return        Transfer status: VTB_TX_RX_BUSY or VTB_TX_RX_DONE
 *
 */
int vtb_rx_chunked(chanend c_in,
        REFERENCE_PARAM(vtb_rx_state_t, state),
        REFERENCE_PARAM(vtb_md_t, md));


/** Transmit a notification token over channel
 *
 * \param         c_out    Channel end to send the notification to.
 * \param         state    VTB TX state. Must have been initialised.
 */
void vtb_tx_notification(chanend c_out, REFERENCE_PARAM(vtb_tx_state_t, state));

//TODO - support with lib_xcore
#ifdef __XC__
/** Receive a notification token over channel
 *
 * \param         c_in     Channel end to receive notification from.
 * \param         state    VTB RX state. Must have been initialised.
 */
#pragma select handler
void vtb_rx_notification(chanend c_in, vtb_rx_state_t &state);
#endif // __XC__

/** Transmit a notification and then a frame of channel pairs to the next DSP block.
 *
 * Equivalent to vtb_tx_notification() followed by calling vtb_tx_chunked() in a loop
 * until completed.
 *
 * \param         c_out    Channel end to send the data to.
 *
 * \param[in]     state    VTB TX state. Must have been initialised.
 *
 * \param[in]     frame    Array containing channel pair data of size
 *                         `state.frame_advance`
 *
 * \param[in]     md       Metadata.
 */
void vtb_tx(chanend c_out,
            REFERENCE_PARAM(vtb_tx_state_t, state),
            vtb_ch_pair_t frame[],
            NULLABLE_REFERENCE_PARAM(vtb_md_t, md));


/** Receive a notification and then a frame of channel pairs from the prior DSP
 * block and update the processing frame.
 *
 * Equivalent to vtb_rx_notification() followed by calling vtb_rx_chunked() in
 * a loop until completed, then calling vtb_form_processing_frame or
 * vtb_form_processing_frame_delayed depending on whether the RX state
 * specifies that any of the channels should be delayed.
 *
 * \param         c_in     Channel end to receive data from.
 *
 * \param[in]     state    VTB RX state. Must have been initialised.
 *
 * \param[in]     frame    Array containing channel pair data of size
 *                         `state.proc_frame_length`
 *
 * \param[out]    md       Metadata.
 */
#ifdef __XC__
#pragma select handler
select vtb_rx(chanend c_in,
              REFERENCE_PARAM(vtb_rx_state_t, state),
              NULLABLE_ARRAY_OF(vtb_ch_pair_t, frame),
              NULLABLE_REFERENCE_PARAM(vtb_md_t, md));
#endif // __XC__


/** Transmit a frame of channel pairs to the next DSP block.
 *
 * Equivalent to calling vtb_tx_chunked() in a loop until completed.
 *
 * \param         c_in     Channel end to receive data from.
 *
 * \param[in]     state    VTB RX state. Must have been initialised.
 *
 * \param[in]     frame    Array containing channel pair data of size
 *                         `state.proc_frame_length`
 *
 * \param[out]    md       Metadata.
 */
void vtb_tx_without_notification(chanend c_out,
            REFERENCE_PARAM(vtb_tx_state_t, state),
            vtb_ch_pair_t frame[],
            NULLABLE_REFERENCE_PARAM(vtb_md_t, md));


/** Receive a frame of channel pairs from the prior DSP block and update the
 * processing frame.
 *
 * Equivalent to calling vtb_rx_chunked() in a loop until completed, then
 * calling vtb_form_processing_frame or vtb_form_processing_frame_delayed
 * depending on whether the RX state specifies that any of the channels should
 * be delayed.
 *
 * \param         c_in     Channel end to receive data from.
 *
 * \param[in]     state    VTB RX state. Must have been initialised.
 *
 * \param[in]     frame    Array containing channel pair data of size
 *                         `state.proc_frame_length`
 *
 * \param[out]    md       Metadata.
 */
void vtb_rx_without_notification(chanend c_in,
            REFERENCE_PARAM(vtb_rx_state_t, state),
            NULLABLE_ARRAY_OF(vtb_ch_pair_t, frame),
            NULLABLE_REFERENCE_PARAM(vtb_md_t, md));



/*
 *                      DEPRECATED FUNCTIONS BELOW
 *
 * Equivalent functions in new API:
 *
 * vtb_rx_state_init                ->      vtb_form_rx_state
 * vtb_tx_notification_and_data           ->      vtb_tx
 * vtb_rx_notification_and_data     ->      vtb_rx
 * vtb_tx_data                      ->      vtb_tx_without_notification
 * vtb_rx_data                      ->      vtb_rx_without_notification
 *
 */

//No plan to support deprecated functions in C
#ifdef __XC__

/** Function that initialises the state for receiving data into a DSP block.
 * This function is called once, before the vtb_rx functions are called in
 * a loop.
 *
 * The state must have been declared as a::
 *    uint64_t state[VTB_RX_STATE_UINT64_SIZE(channels,
 *                                                  td_frame_size,
 *                                                  advance,
 *                                                  totaldelay)];
 *
 * with the appropriate number of channels, time domain frame size and
 * advance. The totaldelay parameter is normally set to 0, but if one or
 * more channels are to be delayed (see later on), then this parameter must
 * be the sum of all delays.
 *
 * \param[in,out] state      Declared as shown above.
 *
 * \param[in]     num_ch    The number of channels to be transported.
 *
 * \param[in]     frame_size The number of samples that are processed by this
 *                           DSP block. The rx function will group the samples
 *                           together in a [channels][frame_size] array.
 *                           This must be a multiple of four.
 *
 * \param[in]     advance    The number of samples that the transmitter sends
 *                           on every iteration. Must be smaller than frame_size.
 *                           This must be a multiple of four.
 *
 * \param[in]     delays     An array that specifies a delay for each channel.
 *                           Can be null. If null, no delays are applied (the
 *                           typical case). Otherwise, some channels can be delayed.
 *                           The sum of all values of this parameter must have
 *                           been used in the declaration of state as the last
 *                           argument of VTB_RX_DELAY_STATE_UINT64_SIZE().
 *                           Each delay must be a multiple of four.
 *
 * \param[in]     size       The number of elements in the state array, the
 *                           value of VTB_RX_DELAY_STATE_UINT64_SIZE().
 */
// THIS FUNCTION IS DEPRECATED.
// See notice above for the equivalent function in the new API.
void vtb_rx_state_init(uint64_t state[],
        const uint32_t num_ch,
        const uint32_t frame_size,
        const uint32_t advance,
        uint32_t (&?delays)[],
        const uint32_t size);


/** Transmit a nofication and then a frame of channel pairs to the next DSP block.
 * Equivalent to vtb_tx_notification() followed by vtb_tx_data().
 *
 * \param         c_out    Channel end to send the data to.
 *
 * \param[in]     data     Array containing channel pair data.
 *
 * \param[in]     num_ch   Number of channels in the data.
 *
 * \param[in]     advance  Number of new elements in the data.
 *
 * \param[in]     md       Metadata.
 */
// THIS FUNCTION IS DEPRECATED.
// See notice above for the equivalent function in the new API.
void vtb_tx_notification_and_data(chanend c_out,
        vtb_ch_pair_t data[],
        const uint32_t num_ch,
        const uint32_t advance,
        vtb_md_t &md);


/** Receive a notifaction and then a frame of channel pairs from the prior DSP block
 * Equivalent to vtb_rx_notification() followed by vtb_rx_data().
 *
 * \param         c_in     Channel end to receive data from.
 *
 * \param[in]     state    State for receiving data. Must have been initialised.
 *
 * \param[in,out] target   Array receiving channel pair data.
 *
 * \param[out]    md       Metadata.
 */
// THIS FUNCTION IS DEPRECATED.
// See notice above for the equivalent function in the new API.
void vtb_rx_notification_and_data(chanend c_in,
        uint64_t state[],
        vtb_ch_pair_t target[],
        vtb_md_t &md);


/** Transmit a frame of channel pairs to the next DSP block
 *
 * \param         c_out    Channel end to send the data to.
 *
 * \param[in]     data     Array containing channel pair data.
 *                         Length must equal advance * (num_ch/2).
 *
 * \param[in]     num_ch   Number of channels in the data.
 *
 * \param[in]     advance  Number of new elements in the data.
 *
 * \param[in]     md       Metadata.
 */
// THIS FUNCTION IS DEPRECATED.
// See notice above for the equivalent function in the new API.
void vtb_tx_data(chanend c_out,
        vtb_ch_pair_t data[],
        const uint32_t num_ch,
        const uint32_t advance,
        vtb_md_t &md);


/** Receive a frame of channel pairs from the prior DSP block
 *
 * \param         c_in     Channel end to receive data from.
 *
 * \param[in]     state    State for receiving data. Must have been initialised.
 *
 * \param[in,out] target   Array receiving channel pair data.
 *
 * \param[out]    md       Metadata.
 */
// THIS FUNCTION IS DEPRECATED.
// See notice above for the equivalent function in the new API.
void vtb_rx_data(chanend c_in,
        uint64_t state[],
        vtb_ch_pair_t target[],
        vtb_md_t &md);

#endif // __XC__

#endif /* VTB_TX_RX_H_ */
