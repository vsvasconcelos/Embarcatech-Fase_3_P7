/**
 * @file ble_server.h
 * @brief BLE GATT Server for Robot Control
 */

#ifndef BLE_SERVER_H
#define BLE_SERVER_H

#include <stdbool.h>
#include <stdint.h>

/**
 * @brief Color mode for the robot
 */
typedef enum {
    COLOR_MODE_AUTO = 0,    /**< Automatic priority mode */
    COLOR_MODE_BLUE,        /**< Follow only blue */
    COLOR_MODE_RED,         /**< Follow only red */
    COLOR_MODE_YELLOW,      /**< Follow only yellow */
} ColorMode;

/**
 * @brief Robot state structure
 */
typedef struct {
    bool running;           /**< Robot is running (not in standby) */
    ColorMode color_mode;   /**< Current color following mode */
    bool calibrated;        /**< White calibration done */
    
    /* Configurable parameters */
    uint16_t motor_base_speed;
    uint16_t motor_spin_speed;
    uint16_t motor_kick_pwm;
    uint16_t motor_kick_time_ms;
    uint16_t motor_turn_time_ms;
} RobotState;

/**
 * @brief Initialize BLE server
 * 
 * Initializes BTstack and starts advertising.
 * Must be called after cyw43_arch_init().
 */
void ble_server_init(void);

/**
 * @brief Process BLE events
 * 
 * Call this periodically in the main loop.
 */
void ble_server_process(void);

/**
 * @brief Get current robot state from BLE commands
 * 
 * @return Pointer to robot state structure
 */
RobotState* ble_get_robot_state(void);

/**
 * @brief Update robot status for BLE notification
 * 
 * @param status Status string to send to connected client
 */
void ble_update_status(const char* status);

#endif /* BLE_SERVER_H */
