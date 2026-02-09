/**
 * @file hal_error.h
 * @brief Hardware Abstraction Layer - Error codes and status types
 *
 * Provides standardized error handling across all drivers.
 */

#ifndef HAL_ERROR_H
#define HAL_ERROR_H

/**
 * @brief HAL status/error codes
 */
typedef enum {
    HAL_OK = 0,                 /**< Operation completed successfully */
    HAL_ERROR_TIMEOUT,          /**< Operation timed out */
    HAL_ERROR_I2C_NACK,         /**< I2C device did not acknowledge */
    HAL_ERROR_I2C_WRITE,        /**< I2C write operation failed */
    HAL_ERROR_I2C_READ,         /**< I2C read operation failed */
    HAL_ERROR_SENSOR_INVALID,   /**< Sensor returned invalid data */
    HAL_ERROR_PARAM,            /**< Invalid parameter */
} hal_status_t;

/**
 * @brief Check if status indicates success
 */
#define HAL_IS_OK(status) ((status) == HAL_OK)

/**
 * @brief Check if status indicates error
 */
#define HAL_IS_ERROR(status) ((status) != HAL_OK)

#endif /* HAL_ERROR_H */
