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

/**
 * @brief Read filtered color data (moving average - 4 samples)
 *
 * @param i2c     I2C instance
 * @param is_left True for left sensor (I2C0), false for right (I2C1)
 * @return Filtered ColorData with smoothed values
 */
ColorData tcs_read_filtered(i2c_inst_t *i2c, bool is_left);

/**
 * @brief Calibrate white reference for a sensor
 *
 * Call this with the sensor placed over a white surface.
 * The current reading will be stored as the reference for normalization.
 *
 * @param i2c     I2C instance
 * @param is_left True for left sensor, false for right
 */
void tcs_calibrate_white(i2c_inst_t *i2c, bool is_left);

/**
 * @brief Check if a sensor has been calibrated
 *
 * @param is_left True for left sensor, false for right
 * @return true if calibration data is available
 */
bool tcs_is_calibrated(bool is_left);

/**
 * @brief Identify color using calibration data
 *
 * @param data    Raw color data
 * @param is_left True for left sensor, false for right
 * @return Detected color type
 */
ColorType color_identify_calibrated(ColorData data, bool is_left);

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
