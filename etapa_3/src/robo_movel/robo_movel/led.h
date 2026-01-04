/**
 * @file led.h
 * @brief LED control interface for indicator LEDs
 */

#ifndef LED_H
#define LED_H

#include <stdbool.h>

/**
 * @brief Initialize all LED GPIOs
 */
void leds_init(void);

/**
 * @brief Control the RGB indicator LEDs
 *
 * @param green  Green LED state (true = on)
 * @param blue   Blue LED state (true = on)
 * @param red    Red LED state (true = on)
 */
void leds_set(bool green, bool blue, bool red);

/**
 * @brief Control the onboard LED
 *
 * @param state  LED state (true = on)
 */
void led_onboard_set(bool state);

#endif /* LED_H */
