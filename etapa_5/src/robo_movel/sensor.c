/**
 * @file sensor.c
 * @brief Sensor driver implementations with filtering and timeout
 *
 * Optimizations:
 * - Moving average filter for color sensor readings
 * - I2C timeout to prevent blocking
 * - Error handling with HAL status codes
 */

#include "sensor.h"
#include "hal_error.h"
#include "pico/stdlib.h"
#include "hardware/timer.h"

/* --------------------------------------------------------------------------
 * Filter Configuration
 * -------------------------------------------------------------------------- */

#define FILTER_SIZE         4       /**< Moving average window size */
#define I2C_TIMEOUT_US      5000    /**< I2C operation timeout (5ms) */

/* Filter buffers for each sensor */
static ColorData g_filter_left[FILTER_SIZE];
static ColorData g_filter_right[FILTER_SIZE];
static uint8_t g_filter_idx_left = 0;
static uint8_t g_filter_idx_right = 0;
static bool g_filter_initialized = false;

/* --------------------------------------------------------------------------
 * Private Helper Functions
 * -------------------------------------------------------------------------- */

/**
 * @brief Write a single byte to an I2C register with timeout
 * @return HAL_OK on success, error code on failure
 */
static hal_status_t i2c_write_register(i2c_inst_t *i2c, uint8_t addr, uint8_t reg, uint8_t val)
{
    uint8_t buf[2] = {reg, val};
    absolute_time_t timeout = make_timeout_time_us(I2C_TIMEOUT_US);
    int result = i2c_write_blocking_until(i2c, addr, buf, 2, false, timeout);
    return (result == 2) ? HAL_OK : HAL_ERROR_I2C_WRITE;
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
    ColorData data = {0};
    data.valid = false;

    uint8_t cmd = TCS_REG_CDATA;
    uint8_t buf[8];
    
    absolute_time_t timeout = make_timeout_time_us(I2C_TIMEOUT_US);

    int write_result = i2c_write_blocking_until(i2c, TCS_I2C_ADDR, &cmd, 1, true, timeout);
    if (write_result < 0) {
        return data;
    }

    timeout = make_timeout_time_us(I2C_TIMEOUT_US);
    int read_result = i2c_read_blocking_until(i2c, TCS_I2C_ADDR, buf, 8, false, timeout);
    if (read_result < 0) {
        return data;
    }

    /* Parse 16-bit values (little-endian) */
    data.c = (buf[1] << 8) | buf[0];
    data.r = (buf[3] << 8) | buf[2];
    data.g = (buf[5] << 8) | buf[4];
    data.b = (buf[7] << 8) | buf[6];
    data.valid = true;

    return data;
}

/**
 * @brief Read filtered color data (moving average)
 *
 * @param i2c     I2C instance
 * @param is_left True for left sensor, false for right
 * @return Filtered ColorData
 */
ColorData tcs_read_filtered(i2c_inst_t *i2c, bool is_left)
{
    ColorData raw = tcs_read(i2c);
    
    if (!raw.valid) {
        /* On error, return last valid average */
        ColorData *buffer = is_left ? g_filter_left : g_filter_right;
        ColorData avg = {0};
        uint8_t count = 0;
        
        for (int i = 0; i < FILTER_SIZE; i++) {
            if (buffer[i].valid) {
                avg.r += buffer[i].r;
                avg.g += buffer[i].g;
                avg.b += buffer[i].b;
                avg.c += buffer[i].c;
                count++;
            }
        }
        
        if (count > 0) {
            avg.r /= count;
            avg.g /= count;
            avg.b /= count;
            avg.c /= count;
            avg.valid = true;
        }
        return avg;
    }
    
    /* Store in circular buffer */
    if (is_left) {
        g_filter_left[g_filter_idx_left] = raw;
        g_filter_idx_left = (g_filter_idx_left + 1) % FILTER_SIZE;
    } else {
        g_filter_right[g_filter_idx_right] = raw;
        g_filter_idx_right = (g_filter_idx_right + 1) % FILTER_SIZE;
    }
    
    /* Calculate moving average */
    ColorData *buffer = is_left ? g_filter_left : g_filter_right;
    ColorData avg = {0};
    uint8_t valid_count = 0;
    
    for (int i = 0; i < FILTER_SIZE; i++) {
        if (buffer[i].valid) {
            avg.r += buffer[i].r;
            avg.g += buffer[i].g;
            avg.b += buffer[i].b;
            avg.c += buffer[i].c;
            valid_count++;
        }
    }
    
    if (valid_count > 0) {
        avg.r /= valid_count;
        avg.g /= valid_count;
        avg.b /= valid_count;
        avg.c /= valid_count;
        avg.valid = true;
    }
    
    return avg;
}

/* --------------------------------------------------------------------------
 * White Calibration
 * -------------------------------------------------------------------------- */

/* Calibration reference values (white surface) */
static ColorData g_calibration_left = {0, 0, 0, 0, false};
static ColorData g_calibration_right = {0, 0, 0, 0, false};

void tcs_calibrate_white(i2c_inst_t *i2c, bool is_left)
{
    /* Read current color as white reference */
    ColorData raw = tcs_read(i2c);
    
    if (raw.valid) {
        if (is_left) {
            g_calibration_left = raw;
            g_calibration_left.valid = true;
        } else {
            g_calibration_right = raw;
            g_calibration_right.valid = true;
        }
    }
}

bool tcs_is_calibrated(bool is_left)
{
    if (is_left) {
        return g_calibration_left.valid;
    }
    return g_calibration_right.valid;
}

/**
 * @brief Normalize color data using white calibration
 */
static ColorData normalize_color(ColorData raw, ColorData white_ref)
{
    ColorData normalized = raw;
    
    if (!white_ref.valid || white_ref.r == 0 || white_ref.g == 0 || white_ref.b == 0) {
        return raw; /* No calibration, return raw */
    }
    
    /* Normalize each channel: (raw / white) * 255 */
    normalized.r = (uint16_t)((raw.r * 255) / white_ref.r);
    normalized.g = (uint16_t)((raw.g * 255) / white_ref.g);
    normalized.b = (uint16_t)((raw.b * 255) / white_ref.b);
    normalized.c = (uint16_t)((raw.c * 255) / white_ref.c);
    
    /* Clamp to 255 */
    if (normalized.r > 255) normalized.r = 255;
    if (normalized.g > 255) normalized.g = 255;
    if (normalized.b > 255) normalized.b = 255;
    if (normalized.c > 255) normalized.c = 255;
    
    return normalized;
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

ColorType color_identify_calibrated(ColorData data, bool is_left)
{
    ColorData white_ref = is_left ? g_calibration_left : g_calibration_right;
    
    /* If calibrated, normalize the data first */
    if (white_ref.valid) {
        data = normalize_color(data, white_ref);
    }
    
    return color_identify(data);
}

/* --------------------------------------------------------------------------
 * SR04 Ultrasonic Sensor Implementation
 * -------------------------------------------------------------------------- */

void sr04_trigger(i2c_inst_t *i2c)
{
    uint8_t cmd = SR04_TRIGGER_CMD;
    absolute_time_t timeout = make_timeout_time_us(I2C_TIMEOUT_US);
    i2c_write_blocking_until(i2c, SR04_I2C_ADDR, &cmd, 1, false, timeout);
}

uint16_t sr04_read(i2c_inst_t *i2c)
{
    uint8_t buf[3];
    absolute_time_t timeout = make_timeout_time_us(I2C_TIMEOUT_US);

    int result = i2c_read_blocking_until(i2c, SR04_I2C_ADDR, buf, 3, false, timeout);

    if (result == 3) {
        /* Convert to millimeters */
        uint32_t raw = ((uint32_t)buf[0] << 16) | ((uint32_t)buf[1] << 8) | buf[2];
        return (uint16_t)(raw / 1000);
    }

    return 9999; /* Error value */
}
