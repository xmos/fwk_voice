/*=========================================================================
| (c) 2003-2007  Total Phase, Inc.
|--------------------------------------------------------------------------
| Project : Aardvark Sample Code
| File    : aadetect.c
|--------------------------------------------------------------------------
| Auto-detection test routine
|--------------------------------------------------------------------------
| Redistribution and use of this file in source and binary forms, with
| or without modification, are permitted.
|
| THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
| "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
| LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
| FOR A PARTICULAR PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE
| COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
| INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
| BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
| LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
| CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
| LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN
| ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
| POSSIBILITY OF SUCH DAMAGE.
 ========================================================================*/

//=========================================================================
// INCLUDES
//=========================================================================
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define USE_AARDVARK_IRQ 0
typedef int16_t samp_t;
#define SPI_CHANNELS 6

#define USE_STDOUT 0
#define LENGTH 4000/15

#if USE_AARDVARK_IRQ
#include "aardvark.h"
#endif
#include "cheetah.h"

//=========================================================================
// MAIN PROGRAM ENTRY POINT
//=========================================================================
int main (int argc, char *argv[])
{
    int nelem = 1;
    int count;

    u16 ch_ports[1];
    Cheetah cheetah;

#if USE_AARDVARK_IRQ
    u16 aa_ports[1];
    Aardvark aardvark;

    count = aa_find_devices(nelem, aa_ports);
    if (count <= 0) {
        printf("Aardvark device not found\n");
        return 1;
    }
    if (aa_ports[0] & AA_PORT_NOT_FREE) {
        printf("Aardvark device is already in use\n");
        return 1;
    }
#endif
    
    count = ch_find_devices(nelem, ch_ports);
    if (count <= 0) {
        printf("Cheetah device not found\n");
        return 1;
    }
    if (ch_ports[0] & CH_PORT_NOT_FREE) {
        printf("Cheetah device is already in use\n");
        return 1;
    }

    cheetah = ch_open(ch_ports[0]);
    if (cheetah <= 0) {
        printf("Unable to open Cheetah device on port %d\n", ch_ports[0]);
        printf("Error code = %d\n", cheetah);
        return 1;
    }

#if USE_AARDVARK_IRQ
    aardvark = aa_open(aa_ports[0]);
    if (aardvark <= 0) {
        printf("Unable to open Aardvark device on port %d\n", aa_ports[0]);
        printf("Error code = %d\n", aardvark);
        ch_close(cheetah);
        return 1;
    }
#endif

#if USE_AARDVARK_IRQ
    aa_configure(aardvark, AA_CONFIG_SPI_GPIO);
#endif

    ch_spi_configure(cheetah, CH_SPI_POL_FALLING_RISING, CH_SPI_PHASE_SETUP_SAMPLE, CH_SPI_BITORDER_MSB, 0);

#if USE_AARDVARK_IRQ
    int r = ch_spi_bitrate(cheetah, 8000);
#else
    int r = ch_spi_bitrate(cheetah, 2 * SPI_CHANNELS*8*sizeof(samp_t) * 16);
#endif

    printf("Actual SPI rate is %d khz\n", r);
    ch_spi_queue_oe(cheetah, 1);

    samp_t tx_buf[240][SPI_CHANNELS];
    samp_t rx_buf[240][SPI_CHANNELS];
    samp_t zeros[240][SPI_CHANNELS];
    
    memset(zeros, 0, sizeof(zeros));
    
    ch_spi_queue_ss(cheetah, 1);
    ch_spi_queue_array(cheetah, sizeof(tx_buf), (u08 *) tx_buf);
    ch_spi_queue_ss(cheetah, 0);

    FILE *f = USE_STDOUT ? stdout : fopen("audio.dat", "wb");
    for (int i = 0; i < LENGTH; i++) {

#if USE_AARDVARK_IRQ
        if ((aa_gpio_get(aardvark) & AA_GPIO_SCL) == 0) {
            while ((aa_gpio_change(aardvark, 4000) & AA_GPIO_SCL) == 0);
        }
#endif
    
        ch_spi_batch_shift(cheetah, sizeof(rx_buf), (u08 *) rx_buf);
        if (memcmp(rx_buf, zeros, sizeof(rx_buf)) != 0) {
            fwrite(rx_buf, 1, sizeof(rx_buf), f);
        } else {
            i--;
        }
    }

    if (!USE_STDOUT) {
        fclose(f);
    }

#if USE_AARDVARK_IRQ
    aa_close(aardvark);
#endif
    ch_close(cheetah);

    return 0;
}

