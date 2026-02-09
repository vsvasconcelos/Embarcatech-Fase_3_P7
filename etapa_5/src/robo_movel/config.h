/**
 * @file config.h
 * @brief Hardware configuration and global definitions for the color line follower robot
 *
 * This header contains all pin definitions, constants, and shared types used
 * across the application modules.
 */

#ifndef CONFIG_H
#define CONFIG_H

#include <stdint.h>
#include <stdbool.h>
#include "hardware/i2c.h"

/* ==========================================================================
 * PIN DEFINITIONS
 * ========================================================================== */

/* Onboard LED */
#define PIN_LED_ONBOARD     25

/* RGB LED Indicator */
#define PIN_LED_GREEN       11
#define PIN_LED_BLUE        12
#define PIN_LED_RED         13

/* Buttons */
#define PIN_BTN_START       5   /* Button A - Start/Stop */
#define PIN_BTN_CALIBRATE   6   /* Button B - White calibration */

/* Motor Driver (TB6612) */
#define PIN_MOTOR_L_FWD     9   /* Left motor forward */
#define PIN_MOTOR_L_BWD     4   /* Left motor backward */
#define PIN_MOTOR_L_PWM     8   /* Left motor PWM */
#define PIN_MOTOR_R_FWD     18  /* Right motor forward */
#define PIN_MOTOR_R_BWD     19  /* Right motor backward */
#define PIN_MOTOR_R_PWM     16  /* Right motor PWM */
#define PIN_MOTOR_STBY      20  /* Standby pin */

/* I2C Bus 0 - Left color sensor */
#define PIN_I2C0_SDA        0
#define PIN_I2C0_SCL        1

/* I2C Bus 1 - Right color sensor + Ultrasonic */
#define PIN_I2C1_SDA        2
#define PIN_I2C1_SCL        3

/* ==========================================================================
 * MOTOR CONFIGURATION
 * ========================================================================== */

#define DEFAULT_MOTOR_BASE_SPEED    16000   /* Base PWM value for straight movement */
#define DEFAULT_MOTOR_SPIN_SPEED    15000   /* PWM value for spin/turn maneuvers */
#define DEFAULT_MOTOR_KICK_PWM      30000   /* Maximum power for startup kick to overcome inertia */
#define DEFAULT_MOTOR_KICK_TIME_MS  60      /* Duration of startup kick (milliseconds) */
#define DEFAULT_MOTOR_TURN_TIME_MS  1000     /* Time for 90-degree turn (milliseconds) */

/* ==========================================================================
 * SENSOR CONFIGURATION
 * ========================================================================== */

/* I2C */
#define I2C_BAUDRATE        100000  /* 100kHz for SR04 compatibility */

/* TCS34725 Color Sensor */
#define TCS_I2C_ADDR        0x29
#define TCS_INTEGRATION     0xFE    /* Integration time setting (2 cycles = 4.8ms) */
#define TCS_GAIN            0x01    /* 4x gain */

/* SR04 Ultrasonic Sensor (I2C module) */
#define SR04_I2C_ADDR       0x57
#define SR04_TRIGGER_CMD    0x01

/* ==========================================================================
 * COLOR DETECTION THRESHOLDS
 * ========================================================================== */

/**
 * Black detection threshold.
 * If the "Clear" channel value is below this, the surface is considered black.
 * - Increase if robot doesn't stop on black
 * - Decrease if robot stops on colored lines
 */
#define COLOR_BLACK_THRESHOLD   40

/* Obstacle detection distance (millimeters) */
#define OBSTACLE_DISTANCE_MM    200

/* Priority lock duration (milliseconds) */
#define COLOR_LOCK_TIME_MS      500

/* ==========================================================================
 * SHARED TYPES
 * ========================================================================== */

/**
 * @brief Raw color data from TCS34725 sensor
 */
typedef struct {
    uint16_t r;     /* Red channel */
    uint16_t g;     /* Green channel */
    uint16_t b;     /* Blue channel */
    uint16_t c;     /* Clear channel (luminosity) */
    bool valid;     /* True if reading was successful */
} ColorData;

/**
 * @brief Detected color type with priority ordering
 *
 * Values are ordered by priority: higher value = higher priority
 */
typedef enum {
    COLOR_NONE   = 0,
    COLOR_BLUE   = 1,
    COLOR_RED    = 2,
    COLOR_YELLOW = 3,
    COLOR_BLACK  = 4   /* Special case: triggers stop */
} ColorType;

#endif /* CONFIG_H */
