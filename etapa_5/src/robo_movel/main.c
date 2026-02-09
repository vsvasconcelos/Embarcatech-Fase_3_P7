/**
 * @file main.c
 * @brief Color line follower robot with BLE control
 *
 * This application implements a line-following robot that:
 * - Follows colored lines controlled via BLE
 * - Stops when detecting black
 * - Avoids obstacles using ultrasonic sensor
 *
 * BLE Commands:
 * - "start" / "stop" - Control robot running state
 * - "azul" / "vermelho" / "amarelo" / "auto" - Color mode selection
 *
 * Button A (GPIO 5): White calibration
 */

#include <stdio.h>
#include "pico/stdlib.h"
#include "pico/cyw43_arch.h"
#include "hardware/i2c.h"
#include "hardware/timer.h"

#include "config.h"
#include "led.h"
#include "motor.h"
#include "sensor.h"
#include "ble_server.h"

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

/* Calibration request flag */
static volatile bool g_calibration_requested = false;
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
    (void)t;

    /* Read left color sensor with moving average filter (I2C0) */
    ColorData data_left = tcs_read_filtered(i2c0, true);
    if (data_left.valid) {
        g_color_left = color_identify_calibrated(data_left, true);
    }

    /* Read right color sensor with moving average filter (I2C1) */
    ColorData data_right = tcs_read_filtered(i2c1, false);
    if (data_right.valid) {
        g_color_right = color_identify_calibrated(data_right, false);
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
 * Calibration Button (Button A - GPIO 5)
 * ========================================================================== */

static void calibration_button_callback(uint gpio, uint32_t events)
{
    (void)events;
    
    if (gpio == PIN_BTN_START) {
        uint64_t now = time_us_64();
        if (now - g_last_button_time > BUTTON_DEBOUNCE_US) {
            g_calibration_requested = true;
            g_last_button_time = now;
        }
    }
}

static void button_init(void)
{
    /* Button A - Calibration */
    gpio_init(PIN_BTN_START);
    gpio_set_dir(PIN_BTN_START, GPIO_IN);
    gpio_pull_up(PIN_BTN_START);
    
    /* Enable interrupt */
    gpio_set_irq_enabled_with_callback(PIN_BTN_START, 
        GPIO_IRQ_EDGE_FALL, true, calibration_button_callback);
    
    printf("Button A (GPIO%d) = White Calibration\n", PIN_BTN_START);
}

/* ==========================================================================
 * Color Mode Filtering
 * ========================================================================== */

/**
 * @brief Apply color mode filter from BLE
 */
static ColorType apply_color_mode_filter(ColorType detected, ColorMode mode)
{
    if (mode == COLOR_MODE_AUTO) {
        return detected;  /* No filtering in auto mode */
    }
    
    /* In specific mode, only pass the selected color */
    switch (mode) {
        case COLOR_MODE_BLUE:
            return (detected == COLOR_BLUE) ? COLOR_BLUE : COLOR_NONE;
        case COLOR_MODE_RED:
            return (detected == COLOR_RED) ? COLOR_RED : COLOR_NONE;
        case COLOR_MODE_YELLOW:
            return (detected == COLOR_YELLOW) ? COLOR_YELLOW : COLOR_NONE;
        default:
            return detected;
    }
}

/**
 * @brief Apply priority filter (for auto mode)
 */
static ColorType apply_priority_filter(ColorType detected, bool lock_active, ColorType locked)
{
    if (detected == COLOR_YELLOW) {
        return COLOR_YELLOW;
    }
    if (detected == COLOR_RED && (!lock_active || locked <= COLOR_RED)) {
        return COLOR_RED;
    }
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
    
    printf("\n=== RoboMovel BLE Line Follower ===\n");

    /* Initialize CYW43 (WiFi/BT chip) */
    if (cyw43_arch_init()) {
        printf("ERROR: Failed to initialize CYW43\n");
        return -1;
    }
    printf("CYW43 initialized\n");

    /* Initialize hardware modules */
    leds_init();
    button_init();
    i2c_buses_init();
    motors_init();

    /* Initialize sensors */
    tcs_init(i2c0);
    tcs_init(i2c1);

    /* Initialize BLE server */
    ble_server_init();

    /* Start sensor reading timer (5ms interval) */
    struct repeating_timer timer;
    add_repeating_timer_ms(5, timer_callback, NULL, &timer);

    printf("Commands via BLE: start, stop, azul, vermelho, amarelo, auto\n");
    printf("Button A: White calibration\n");
    printf("Ready!\n");

    /* Get robot state from BLE module */
    RobotState* robot = ble_get_robot_state();

    /* ---------- Main Loop ---------- */
    while (true) {
        
        /* ----- Handle Calibration Request ----- */
        /* Polling fallback for button */
        if (!gpio_get(PIN_BTN_START)) {
            uint64_t now = time_us_64();
            if (now - g_last_button_time > BUTTON_DEBOUNCE_US) {
                g_calibration_requested = true;
                g_last_button_time = now;
            }
        }
        
        if (g_calibration_requested) {
            g_calibration_requested = false;
            
            /* Visual feedback */
            leds_set(false, true, false); /* Blue */
            
            /* Calibrate both sensors */
            tcs_calibrate_white(i2c0, true);
            tcs_calibrate_white(i2c1, false);
            robot->calibrated = true;
            
            printf(">>> WHITE CALIBRATION COMPLETE <<<\n");
            ble_update_status("Calibrado");
            
            /* Flash green 3 times */
            for (int i = 0; i < 3; i++) {
                leds_set(false, true, false);
                sleep_ms(200);
                leds_set(false, false, false);
                sleep_ms(200);
            }
        }

        /* ----- Standby Mode (from BLE) ----- */
        if (!robot->running) {
            motors_stop();
            leds_set(true, true, true); /* White = waiting */
            sleep_ms(50);
            continue;
        }
        
        /* Sync Motor Parameters from BLE */
        motors_set_params(
            robot->motor_base_speed,
            robot->motor_spin_speed,
            robot->motor_kick_pwm,
            robot->motor_kick_time_ms,
            robot->motor_turn_time_ms
        );

        /* Wait for new sensor reading */
        if (!g_new_reading) {
            sleep_ms(1);
            continue;
        }
        g_new_reading = false;

        /* Get current readings */
        uint16_t distance = g_distance_mm;
        ColorType color_left = g_color_left;
        ColorType color_right = g_color_right;

        /* ----- Black Detection: Emergency Stop ----- */
        if (color_left == COLOR_BLACK || color_right == COLOR_BLACK) {
            printf(">>> BLACK DETECTED - STOPPING <<<\n");
            motors_stop();
            leds_set(true, false, true); /* Magenta */
            robot->running = false;
            ble_update_status("Parou: Preto");
            sleep_ms(2000);
            continue;
        }

        /* ----- Obstacle Detection ----- */
        if (distance < OBSTACLE_DISTANCE_MM) {
            leds_set(false, false, true); /* Red */
            motors_spin_obstacle();
            continue;
        }

        /* ----- Apply Color Mode Filter (from BLE) ----- */
        ColorType filtered_left = apply_color_mode_filter(color_left, robot->color_mode);
        ColorType filtered_right = apply_color_mode_filter(color_right, robot->color_mode);

        /* ----- Color Priority Logic (for auto mode) ----- */
        ColorType best_left, best_right;
        
        if (robot->color_mode == COLOR_MODE_AUTO) {
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

            best_left = apply_priority_filter(filtered_left, lock_active, g_locked_priority);
            best_right = apply_priority_filter(filtered_right, lock_active, g_locked_priority);

            /* Update lock */
            ColorType highest = (best_left > best_right) ? best_left : best_right;
            if (highest > g_locked_priority) {
                g_locked_priority = highest;
                g_lock_start_time = get_absolute_time();
            }
        } else {
            /* In specific color mode, just use filtered values */
            best_left = filtered_left;
            best_right = filtered_right;
        }

        /* ----- Movement Decision ----- */
        if (best_left > best_right) {
            leds_set(false, true, false); /* Green */
            motors_spin_left();
        } else if (best_right > best_left) {
            leds_set(false, true, false); /* Green */
            motors_spin_right();
        } else {
            leds_set(false, false, false); /* Off */
            motors_forward();
        }

        sleep_ms(1);
    }

    return 0;
}