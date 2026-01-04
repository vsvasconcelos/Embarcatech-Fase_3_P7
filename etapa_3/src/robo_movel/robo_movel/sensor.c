/**
 * @file sensor.c
 * @brief Sensor driver implementations
 */

#include "sensor.h"
#include "pico/stdlib.h"

/* --------------------------------------------------------------------------
 * Private Helper Functions
 * -------------------------------------------------------------------------- */

/**
 * @brief Write a single byte to an I2C register
 */
static void i2c_write_register(i2c_inst_t *i2c, uint8_t addr, uint8_t reg, uint8_t val)
{
    uint8_t buf[2] = {reg, val};
    i2c_write_blocking(i2c, addr, buf, 2, false);
}

/* --------------------------------------------------------------------------
 * TCS34725 Color Sensor Implementation
 * -------------------------------------------------------------------------- */

/* TCS34725 Register Addresses */
#define TCS_REG_ENABLE      (0x80 | 0x00)
#define TCS_REG_ATIME       (0x80 | 0x01)
#define TCS_REG_CONTROL     (0x80 | 0x0F)
#define TCS_REG_CDATA       (0x80 | 0x14)

/* Enable register bits */
#define TCS_ENABLE_PON      0x01    /* Power on */
#define TCS_ENABLE_AEN      0x02    /* ADC enable */

void tcs_init(i2c_inst_t *i2c)
{
    /* Set integration time */
    i2c_write_register(i2c, TCS_I2C_ADDR, TCS_REG_ATIME, TCS_INTEGRATION);

    /* Set gain to 4x */
    i2c_write_register(i2c, TCS_I2C_ADDR, TCS_REG_CONTROL, TCS_GAIN);

    /* Power on */
    i2c_write_register(i2c, TCS_I2C_ADDR, TCS_REG_ENABLE, TCS_ENABLE_PON);
    sleep_ms(3);

    /* Enable ADC */
    i2c_write_register(i2c, TCS_I2C_ADDR, TCS_REG_ENABLE, TCS_ENABLE_PON | TCS_ENABLE_AEN);
}

ColorData tcs_read(i2c_inst_t *i2c)
{
    ColorData data;
    data.valid = true;

    uint8_t cmd = TCS_REG_CDATA;
    uint8_t buf[8];

    int write_result = i2c_write_blocking(i2c, TCS_I2C_ADDR, &cmd, 1, true);
    int read_result = i2c_read_blocking(i2c, TCS_I2C_ADDR, buf, 8, false);

    if (write_result < 0 || read_result < 0) {
        data.valid = false;
        return data;
    }

    /* Parse 16-bit values (little-endian) */
    data.c = (buf[1] << 8) | buf[0];
    data.r = (buf[3] << 8) | buf[2];
    data.g = (buf[5] << 8) | buf[4];
    data.b = (buf[7] << 8) | buf[6];

    return data;
}

ColorType color_identify(ColorData data)
{
    /* Check for BLACK (very low luminosity) */
    if (data.c < COLOR_BLACK_THRESHOLD) {
        return COLOR_BLACK;
    }

    /* Check for YELLOW (high red AND green, low blue) */
    if (data.r > data.b * 1.5 && data.g > data.b * 1.5) {
        return COLOR_YELLOW;
    }

    /* Check for RED (high red, low green and blue) */
    if (data.r > data.g * 1.5 && data.r > data.b * 1.5) {
        return COLOR_RED;
    }

    /* Check for BLUE (high blue, low red) */
    if (data.b > data.r * 1.4) {
        return COLOR_BLUE;
    }

    return COLOR_NONE;
}

/* --------------------------------------------------------------------------
 * SR04 Ultrasonic Sensor Implementation
 * -------------------------------------------------------------------------- */

void sr04_trigger(i2c_inst_t *i2c)
{
    uint8_t cmd = SR04_TRIGGER_CMD;
    i2c_write_blocking(i2c, SR04_I2C_ADDR, &cmd, 1, false);
}

uint16_t sr04_read(i2c_inst_t *i2c)
{
    uint8_t buf[3];

    int result = i2c_read_blocking(i2c, SR04_I2C_ADDR, buf, 3, false);

    if (result == 3) {
        /* Convert to millimeters */
        uint32_t raw = ((uint32_t)buf[0] << 16) | ((uint32_t)buf[1] << 8) | buf[2];
        return (uint16_t)(raw / 1000);
    }

    return 9999; /* Error value */
}
