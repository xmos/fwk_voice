// Copyright 2021 XMOS LIMITED.
// This Software is subject to the terms of the XMOS Public Licence: Version 1.

#include <platform.h>

#include "FreeRTOS.h"

#include "platform/app_pll_ctrl.h"
#include "gpio_test/gpio_test.h"

RTOS_GPIO_ISR_CALLBACK_ATTR
static void button_callback(rtos_gpio_t *ctx, void *app_data, rtos_gpio_port_id_t port_id, uint32_t value)
{
    TaskHandle_t task = app_data;
    BaseType_t yield_required = pdFALSE;

    value = (~value) & 0x3;

    xTaskNotifyFromISR(task, value, eSetValueWithOverwrite, &yield_required);

    portYIELD_FROM_ISR(yield_required);
}

static void gpio_handler(rtos_gpio_t *gpio_ctx)
{
    uint32_t value;
    uint32_t buttons_val;
    uint32_t buttonA;
    uint32_t buttonB;

    const rtos_gpio_port_id_t button_port = rtos_gpio_port(PORT_BUTTONS);
    const rtos_gpio_port_id_t led_port = rtos_gpio_port(PORT_LEDS);

    rtos_gpio_port_enable(gpio_ctx, led_port);
    rtos_gpio_port_enable(gpio_ctx, button_port);

    rtos_gpio_isr_callback_set(gpio_ctx, button_port, button_callback, xTaskGetCurrentTaskHandle());
    rtos_gpio_interrupt_enable(gpio_ctx, button_port);

    for (;;) {
        xTaskNotifyWait(
                0x00000000UL,    /* Don't clear notification bits on entry */
                0xFFFFFFFFUL,    /* Reset full notification value on exit */
                &value,          /* Pass out notification value into value */
                portMAX_DELAY ); /* Wait indefinitely until next notification */

        buttons_val = rtos_gpio_port_in(gpio_ctx, button_port);
        buttonA = ( buttons_val >> 0 ) & 0x01;
        buttonB = ( buttons_val >> 1 ) & 0x01;

        rtos_gpio_port_out(gpio_ctx, led_port, buttons_val);

        extern volatile int mic_from_usb;
        extern volatile int aec_ref_source;
        if (buttonA == 0) {
            rtos_printf("button a\n");
//            app_pll_nudge(APP_PLL_NUDGE_FASTER);
            mic_from_usb = !mic_from_usb;
        } else if (buttonB == 0) {
            rtos_printf("button b\n");
            aec_ref_source = !aec_ref_source;
//            app_pll_nudge(APP_PLL_NUDGE_SLOWER);
        }
    }
}

void gpio_test(rtos_gpio_t *gpio_ctx)
{
    xTaskCreate((TaskFunction_t) gpio_handler,
                "gpio_handler",
                RTOS_THREAD_STACK_SIZE(gpio_handler),
                gpio_ctx,
                configMAX_PRIORITIES-2,
                NULL);
}
