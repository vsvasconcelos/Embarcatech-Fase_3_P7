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
    gpio_put(PIN_LED_GREEN, green);
    gpio_put(PIN_LED_BLUE, blue);
    gpio_put(PIN_LED_RED, red);
}

void led_onboard_set(bool state)
{
    gpio_put(PIN_LED_ONBOARD, state);
}
