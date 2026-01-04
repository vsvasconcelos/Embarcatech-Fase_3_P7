/**
 * @file sensor.h
 * @brief Sensor drivers for TCS34725 color sensor and SR04 ultrasonic
 */

#ifndef SENSOR_H
#define SENSOR_H

#include "config.h"
#include "hardware/i2c.h"

/* --------------------------------------------------------------------------
 * TCS34725 Color Sensor
 * -------------------------------------------------------------------------- */

/**
 * @brief Initialize TCS34725 color sensor
 *
 * @param i2c  I2C instance (i2c0 or i2c1)
 */
void tcs_init(i2c_inst_t *i2c);

/**
 * @brief Read raw color data from TCS34725
 *
 * @param i2c  I2C instance
 * @return ColorData with RGBC values, check .valid before using
 */
ColorData tcs_read(i2c_inst_t *i2c);

/**
 * @brief Identify color from raw sensor data
 *
 * @param data  Raw color data from tcs_read()
 * @return Detected color type
 */
ColorType color_identify(ColorData data);

/* --------------------------------------------------------------------------
 * SR04 Ultrasonic Sensor (I2C Module)
 * -------------------------------------------------------------------------- */

/**
 * @brief Trigger ultrasonic measurement
 *
 * Call this, then wait ~100ms before reading.
 *
 * @param i2c  I2C instance
 */
void sr04_trigger(i2c_inst_t *i2c);

/**
 * @brief Read distance from ultrasonic sensor
 *
 * @param i2c  I2C instance
 * @return Distance in millimeters, or 9999 if read failed
 */
uint16_t sr04_read(i2c_inst_t *i2c);

#endif /* SENSOR_H */
