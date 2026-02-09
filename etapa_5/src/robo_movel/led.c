/**
 * @file led.c
 * @brief LED control implementation
 */

#include "led.h"
#include "config.h"
#include "pico/stdlib.h"

void leds_init(void)
{
    gpio_init(PIN_LED_ONBOARD);
    gpio_set_dir(PIN_LED_ONBOARD, GPIO_OUT);

    gpio_init(PIN_LED_GREEN);
    gpio_set_dir(PIN_LED_GREEN, GPIO_OUT);

    gpio_init(PIN_LED_BLUE);
    gpio_set_dir(PIN_LED_BLUE, GPIO_OUT);

    gpio_init(PIN_LED_RED);
    gpio_set_dir(PIN_LED_RED, GPIO_OUT);
}

void leds_set(bool green, bool blue, bool red)
{
    /* Atomic write using mask - all LEDs change simultaneously */
    uint32_t mask = (1u << PIN_LED_GREEN) | (1u << PIN_LED_BLUE) | (1u << PIN_LED_RED);
    uint32_t value = ((uint32_t)green << PIN_LED_GREEN) | 
                     ((uint32_t)blue << PIN_LED_BLUE) | 
                     ((uint32_t)red << PIN_LED_RED);
    gpio_put_masked(mask, value);
}

void led_onboard_set(bool state)
{
    gpio_put(PIN_LED_ONBOARD, state);
}
