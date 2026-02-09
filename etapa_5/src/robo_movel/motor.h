/**
 * @file motor.h
 * @brief Motor control interface for dual DC motors with PWM
 */

#ifndef MOTOR_H
#define MOTOR_H

#include <stdint.h>

/**
 * @brief Initialize motor driver GPIOs and PWM
 */
void motors_init(void);

/**
 * @brief Stop both motors immediately
 */
void motors_stop(void);

/**
 * @brief Move forward at base speed
 */
void motors_forward(void);

/**
 * @brief Spin left in place (left backward, right forward)
 */
void motors_spin_left(void);

/**
 * @brief Spin right in place (left forward, right backward)
 */
void motors_spin_right(void);

/**
 * @brief Execute 90-degree obstacle avoidance turn
 *
 * This is a blocking function that performs a timed spin.
 */
void motors_spin_obstacle(void);

/**
 * @brief Update motor configuration parameters
 */
void motors_set_params(uint16_t base_speed, uint16_t spin_speed, 
                       uint16_t kick_pwm, uint16_t kick_time_ms, 
                       uint16_t turn_time_ms);


#endif /* MOTOR_H */
