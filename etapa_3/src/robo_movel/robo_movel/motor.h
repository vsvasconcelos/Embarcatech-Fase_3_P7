/**
 * @file motor.h
 * @brief Motor control interface for dual DC motors with PWM
 */

#ifndef MOTOR_H
#define MOTOR_H

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

#endif /* MOTOR_H */
