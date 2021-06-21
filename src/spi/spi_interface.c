// Copyright (c) 2020 XMOS LIMITED. This Software is subject to the terms of the
// XMOS Public License: Version 1

#include "FreeRTOS.h"
#include "stream_buffer.h"

#include "app_conf.h"
#include "platform/driver_instances.h"

static StreamBufferHandle_t samples_to_host_stream_buf;

typedef int16_t samp_t;

void spi_audio_send(rtos_intertile_t *intertile_ctx,
                    size_t frame_count,
                    int32_t (*processed_audio_frame)[2])
{
    samp_t spi_audio_in_frame[appconfAUDIO_PIPELINE_FRAME_ADVANCE][2];


    const int src_32_shift = 16;

    xassert(frame_count == appconfAUDIO_PIPELINE_FRAME_ADVANCE);

    for (int i = 0; i < appconfAUDIO_PIPELINE_FRAME_ADVANCE; i++) {
        spi_audio_in_frame[i][0] = processed_audio_frame[i][0] >> src_32_shift;
        spi_audio_in_frame[i][1] = processed_audio_frame[i][1] >> src_32_shift;
    }

//    rtos_printf("%02x %02x %02x %02x\n", spi_audio_in_frame[0][0], spi_audio_in_frame[0][1],
//                spi_audio_in_frame[1][0], spi_audio_in_frame[1][1]);

    rtos_intertile_tx(intertile_ctx,
                      appconfSPI_AUDIO_PORT,
                      spi_audio_in_frame,
                      sizeof(spi_audio_in_frame));
}

static void spi_audio_in_task(void *arg)
{
    for (;;) {
        samp_t spi_audio_in_frame[appconfAUDIO_PIPELINE_FRAME_ADVANCE][2];
        size_t frame_length;

        frame_length = rtos_intertile_rx_len(
                intertile_ctx,
                appconfSPI_AUDIO_PORT,
                portMAX_DELAY);

        xassert(frame_length == sizeof(spi_audio_in_frame));

        rtos_intertile_rx_data(
                intertile_ctx,
                spi_audio_in_frame,
                frame_length);

        if (xStreamBufferSend(samples_to_host_stream_buf, spi_audio_in_frame, sizeof(spi_audio_in_frame), 0) != sizeof(spi_audio_in_frame)) {
            //rtos_printf("lost VFE output samples\n");
        }
    }
}

RTOS_SPI_SLAVE_CALLBACK_ATTR
void spi_slave_start_cb(rtos_spi_slave_t *ctx, void *app_data)
{
    static samp_t tx_buf[appconfAUDIO_PIPELINE_FRAME_ADVANCE][2];

    rtos_printf("SPI SLAVE STARTING!\n");

    samples_to_host_stream_buf = xStreamBufferCreate(3 * appconfAUDIO_PIPELINE_FRAME_ADVANCE * 2 * sizeof(int16_t), 0);

    xTaskCreate((TaskFunction_t) spi_audio_in_task, "spi_audio_in_task", portTASK_STACK_DEPTH(spi_audio_in_task), NULL, 16, NULL);

    spi_slave_xfer_prepare(ctx, NULL, 0, tx_buf, sizeof(tx_buf));
}

RTOS_SPI_SLAVE_CALLBACK_ATTR
void spi_slave_xfer_done_cb(rtos_spi_slave_t *ctx, void *app_data)
{
    uint8_t *tx_buf;
    uint8_t *rx_buf;
    size_t rx_len;
    size_t tx_len;

    if (spi_slave_xfer_complete(ctx, &rx_buf, &rx_len, &tx_buf, &tx_len, 0) == 0) {

        xStreamBufferReceive(samples_to_host_stream_buf, tx_buf, tx_len, 0);
//        tx_buf[0] = 0x42; tx_buf[1] = 0x99;

        rtos_printf("SPI slave xfer complete\n");
        rtos_printf("%d bytes sent, %d bytes received\n", tx_len, rx_len);
//        rtos_printf("TX: ");
//        for (int i = 0; i < 32; i++) {
//            rtos_printf("%02x ", tx_buf[i]);
//        }
//        rtos_printf("\n");
//        rtos_printf("RX: ");
//        for (int i = 0; i < 32; i++) {
//            rtos_printf("%02x ", rx_buf[i]);
//        }
//        rtos_printf("\n");
    }
}
