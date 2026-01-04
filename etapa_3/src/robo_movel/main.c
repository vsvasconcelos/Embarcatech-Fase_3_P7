/**
 * @file main.c
 * @brief Color line follower robot - Main application
 *
 * This application implements a line-following robot that:
 * - Follows colored lines with priority: Yellow > Red > Blue
 * - Stops when detecting black
 * - Avoids obstacles using ultrasonic sensor
 *
 * Hardware:
 * - Raspberry Pi Pico
 * - 2x TCS34725 color sensors (left/right)
 * - 1x SR04 ultrasonic sensor (I2C module)
 * - TB6612 motor driver with 2x DC motors
 * - RGB LED for status indication
 */

#include <stdio.h>
#include "pico/stdlib.h"
#include "hardware/i2c.h"
#include "hardware/timer.h"

#include "config.h"
#include "led.h"
#include "motor.h"
#include "sensor.h"

/* ==========================================================================
 * Global State Variables
 * ========================================================================== */

/* Current sensor readings (updated by timer callback) */
static volatile ColorType g_color_left = COLOR_NONE;
static volatile ColorType g_color_right = COLOR_NONE;
static volatile uint16_t g_distance_mm = 9999;
static volatile bool g_new_reading = false;

/* Ultrasonic sensor timing */
static volatile int g_ultrasonic_counter = 0;

/* Application mode */
static volatile bool g_standby_mode = true;

/* Button interrupt debounce */
static volatile uint64_t g_last_button_time = 0;
#define BUTTON_DEBOUNCE_US 200000  /* 200ms debounce */

/* Color priority lock (prevents oscillation between colors) */
static ColorType g_locked_priority = COLOR_NONE;
static absolute_time_t g_lock_start_time;

/* ==========================================================================
 * Timer Callback - Sensor Reading (10ms interval)
 * ========================================================================== */

static bool timer_callback(struct repeating_timer *t)
{
    (void)t; /* Unused parameter */

    /* Read left color sensor with moving average filter (I2C0) */
    ColorData data_left = tcs_read_filtered(i2c0, true);
    if (data_left.valid) {
        g_color_left = color_identify(data_left);
    }

    /* Read right color sensor with moving average filter (I2C1) */
    ColorData data_right = tcs_read_filtered(i2c1, false);
    if (data_right.valid) {
        g_color_right = color_identify(data_right);
    }

    /* Ultrasonic sensor reading (every 100ms) */
    g_ultrasonic_counter++;
    if (g_ultrasonic_counter == 1) {
        sr04_trigger(i2c1);
    } else if (g_ultrasonic_counter >= 10) {
        uint16_t distance = sr04_read(i2c1);
        if (distance != 9999) {
            g_distance_mm = distance;
        }
        g_ultrasonic_counter = 0;
    }

    g_new_reading = true;
    return true;
}

/* ==========================================================================
 * I2C Initialization
 * ========================================================================== */

static void i2c_buses_init(void)
{
    /* I2C0: Left color sensor */
    i2c_init(i2c0, I2C_BAUDRATE);
    gpio_set_function(PIN_I2C0_SDA, GPIO_FUNC_I2C);
    gpio_set_function(PIN_I2C0_SCL, GPIO_FUNC_I2C);
    gpio_pull_up(PIN_I2C0_SDA);
    gpio_pull_up(PIN_I2C0_SCL);

    /* I2C1: Right color sensor + Ultrasonic */
    i2c_init(i2c1, I2C_BAUDRATE);
    gpio_set_function(PIN_I2C1_SDA, GPIO_FUNC_I2C);
    gpio_set_function(PIN_I2C1_SCL, GPIO_FUNC_I2C);
    gpio_pull_up(PIN_I2C1_SDA);
    gpio_pull_up(PIN_I2C1_SCL);
}

/* ==========================================================================
 * Button GPIO Interrupt Callback
 * ========================================================================== */

/**
 * @brief GPIO interrupt callback for button with debounce
 */
static void button_callback(uint gpio, uint32_t events)
{
    (void)events;
    
    if (gpio == PIN_BTN_START) {
        uint64_t now = time_us_64();
        if (now - g_last_button_time > BUTTON_DEBOUNCE_US) {
            g_standby_mode = !g_standby_mode;  /* Toggle mode */
            g_last_button_time = now;
        }
    }
}

/* ==========================================================================
 * Button Initialization with Interrupt
 * ========================================================================== */

static void button_init(void)
{
    gpio_init(PIN_BTN_START);
    gpio_set_dir(PIN_BTN_START, GPIO_IN);
    gpio_pull_up(PIN_BTN_START);
    
    /* Enable interrupt on falling edge with debounce callback */
    gpio_set_irq_enabled_with_callback(PIN_BTN_START, 
        GPIO_IRQ_EDGE_FALL, true, button_callback);
}

/* ==========================================================================
 * Color Priority Logic
 * ========================================================================== */

/**
 * @brief Determine the best color considering priority lock
 *
 * @param detected  The currently detected color
 * @param lock_active  Whether a priority lock is active
 * @param locked  The currently locked priority
 * @return The color to act upon (may be COLOR_NONE if blocked)
 */
static ColorType apply_priority_filter(ColorType detected, bool lock_active, ColorType locked)
{
    /* Yellow always passes (highest priority) */
    if (detected == COLOR_YELLOW) {
        return COLOR_YELLOW;
    }

    /* Red passes if not blocked by higher priority */
    if (detected == COLOR_RED && (!lock_active || locked <= COLOR_RED)) {
        return COLOR_RED;
    }

    /* Blue passes if not blocked by higher priority */
    if (detected == COLOR_BLUE && (!lock_active || locked <= COLOR_BLUE)) {
        return COLOR_BLUE;
    }

    return COLOR_NONE;
}

/* ==========================================================================
 * Main Application
 * ========================================================================== */

int main(void)
{
    /* Initialize stdio for debug output */
    stdio_init_all();

    /* Initialize hardware modules */
    leds_init();
    button_init();
    i2c_buses_init();
    motors_init();

    /* Initialize sensors */
    tcs_init(i2c0);
    tcs_init(i2c1);

    /* Start sensor reading timer (10ms interval) */
    struct repeating_timer timer;
    add_repeating_timer_ms(10, timer_callback, NULL, &timer);

    printf("=== Color Line Follower Ready ===\n");
    printf("Press button A (GPIO %d) to start\n", PIN_BTN_START);

    /* ---------- Main Loop ---------- */
    while (true) {

        /* ----- Standby Mode ----- */
        if (g_standby_mode) {
            motors_stop();
            leds_set(true, true, true); /* All LEDs on = waiting */
            
            /* Button handled by interrupt - just wait */
            sleep_ms(50);
            continue;
        }

        /* Wait for new sensor reading */
        if (!g_new_reading) {
            continue;
        }
        g_new_reading = false;

        /* Get current readings (copy from volatile) */
        uint16_t distance = g_distance_mm;
        ColorType color_left = g_color_left;
        ColorType color_right = g_color_right;

        /* ----- Black Detection: Emergency Stop ----- */
        if (color_left == COLOR_BLACK || color_right == COLOR_BLACK) {
            printf(">>> BLACK DETECTED - STOPPING <<<\n");
            motors_stop();
            leds_set(true, false, true); /* Magenta */
            sleep_ms(2000);
            g_standby_mode = true;
            continue;
        }

        /* ----- Obstacle Detection ----- */
        if (distance < OBSTACLE_DISTANCE_MM) {
            leds_set(false, false, true); /* Red */
            motors_spin_obstacle();
            continue;
        }

        /* ----- Color Priority Logic ----- */

        /* Check if priority lock is still active */
        bool lock_active = false;
        if (g_locked_priority != COLOR_NONE) {
            int64_t elapsed_us = absolute_time_diff_us(g_lock_start_time, get_absolute_time());
            if (elapsed_us < (COLOR_LOCK_TIME_MS * 1000)) {
                lock_active = true;
            } else {
                g_locked_priority = COLOR_NONE;
            }
        }

        /* Apply priority filter to both sensors */
        ColorType best_left = apply_priority_filter(color_left, lock_active, g_locked_priority);
        ColorType best_right = apply_priority_filter(color_right, lock_active, g_locked_priority);

        /* Determine highest priority currently detected */
        ColorType highest = (best_left > best_right) ? best_left : best_right;

        /* Update lock if we detected a higher priority color */
        if (highest > g_locked_priority) {
            g_locked_priority = highest;
            g_lock_start_time = get_absolute_time();
        }

        /* ----- Movement Decision ----- */
        if (best_left > best_right) {
            /* Higher priority on left -> turn left */
            leds_set(false, true, false); /* Green */
            motors_spin_left();
        } else if (best_right > best_left) {
            /* Higher priority on right -> turn right */
            leds_set(false, true, false); /* Green */
            motors_spin_right();
        } else {
            /* Equal priority (or no color) -> go straight */
            leds_set(false, false, false); /* LEDs off */
            motors_forward();
        }

        sleep_ms(1);
    }

    return 0;
}