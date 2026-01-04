/**
 * @file motor.c
 * @brief Motor control implementation using PWM
 */

#include "motor.h"
#include "config.h"
#include "pico/stdlib.h"
#include "hardware/pwm.h"

/* --------------------------------------------------------------------------
 * Private Helper Functions
 * -------------------------------------------------------------------------- */

/**
 * @brief Configure a GPIO pin for PWM output
 */
static void pwm_pin_init(uint pin)
{
    gpio_set_function(pin, GPIO_FUNC_PWM);
    uint slice = pwm_gpio_to_slice_num(pin);
    pwm_set_wrap(slice, 65535);
    pwm_set_enabled(slice, true);
}

/* --------------------------------------------------------------------------
 * Public API
 * -------------------------------------------------------------------------- */

void motors_init(void)
{
    /* Initialize direction pins */
    gpio_init(PIN_MOTOR_L_FWD);
    gpio_set_dir(PIN_MOTOR_L_FWD, GPIO_OUT);

    gpio_init(PIN_MOTOR_L_BWD);
    gpio_set_dir(PIN_MOTOR_L_BWD, GPIO_OUT);

    gpio_init(PIN_MOTOR_R_FWD);
    gpio_set_dir(PIN_MOTOR_R_FWD, GPIO_OUT);

    gpio_init(PIN_MOTOR_R_BWD);
    gpio_set_dir(PIN_MOTOR_R_BWD, GPIO_OUT);

    /* Initialize standby pin (active high to enable driver) */
    gpio_init(PIN_MOTOR_STBY);
    gpio_set_dir(PIN_MOTOR_STBY, GPIO_OUT);
    gpio_put(PIN_MOTOR_STBY, 1);

    /* Initialize PWM pins */
    pwm_pin_init(PIN_MOTOR_L_PWM);
    pwm_pin_init(PIN_MOTOR_R_PWM);
}

void motors_stop(void)
{
    gpio_put(PIN_MOTOR_L_FWD, 0);
    gpio_put(PIN_MOTOR_L_BWD, 0);
    gpio_put(PIN_MOTOR_R_FWD, 0);
    gpio_put(PIN_MOTOR_R_BWD, 0);

    pwm_set_gpio_level(PIN_MOTOR_L_PWM, 0);
    pwm_set_gpio_level(PIN_MOTOR_R_PWM, 0);
}

void motors_forward(void)
{
    gpio_put(PIN_MOTOR_L_FWD, 1);
    gpio_put(PIN_MOTOR_L_BWD, 0);
    gpio_put(PIN_MOTOR_R_FWD, 1);
    gpio_put(PIN_MOTOR_R_BWD, 0);

    pwm_set_gpio_level(PIN_MOTOR_L_PWM, MOTOR_BASE_SPEED);
    pwm_set_gpio_level(PIN_MOTOR_R_PWM, MOTOR_BASE_SPEED);
}

void motors_spin_left(void)
{
    gpio_put(PIN_MOTOR_L_FWD, 0);
    gpio_put(PIN_MOTOR_L_BWD, 1);
    gpio_put(PIN_MOTOR_R_FWD, 1);
    gpio_put(PIN_MOTOR_R_BWD, 0);

    pwm_set_gpio_level(PIN_MOTOR_L_PWM, MOTOR_BASE_SPEED);
    pwm_set_gpio_level(PIN_MOTOR_R_PWM, MOTOR_SPIN_SPEED);
}

void motors_spin_right(void)
{
    gpio_put(PIN_MOTOR_L_FWD, 1);
    gpio_put(PIN_MOTOR_L_BWD, 0);
    gpio_put(PIN_MOTOR_R_FWD, 0);
    gpio_put(PIN_MOTOR_R_BWD, 1);

    pwm_set_gpio_level(PIN_MOTOR_L_PWM, MOTOR_BASE_SPEED);
    pwm_set_gpio_level(PIN_MOTOR_R_PWM, MOTOR_SPIN_SPEED);
}

void motors_spin_obstacle(void)
{
    /* Spin in place: both motors at same speed, opposite directions */
    gpio_put(PIN_MOTOR_L_FWD, 1);
    gpio_put(PIN_MOTOR_L_BWD, 0);
    gpio_put(PIN_MOTOR_R_FWD, 0);
    gpio_put(PIN_MOTOR_R_BWD, 1);

    pwm_set_gpio_level(PIN_MOTOR_L_PWM, MOTOR_SPIN_SPEED);
    pwm_set_gpio_level(PIN_MOTOR_R_PWM, MOTOR_SPIN_SPEED);

    sleep_ms(MOTOR_TURN_TIME_MS);
}
