/**
 * @file motor.c
 * @brief Motor control implementation using PWM
 */

#include "motor.h"
#include "config.h"
#include "pico/stdlib.h"
#include "hardware/pwm.h"

/* --------------------------------------------------------------------------
 * State Tracking
 * -------------------------------------------------------------------------- */

typedef enum {
    MOTOR_STATE_STOP,
    MOTOR_STATE_FORWARD,
    MOTOR_STATE_LEFT,
    MOTOR_STATE_RIGHT
} MotorState;

static MotorState current_state = MOTOR_STATE_STOP;

/* --------------------------------------------------------------------------
 * Configuration Variables
 * -------------------------------------------------------------------------- */

static uint16_t g_motor_base_speed = DEFAULT_MOTOR_BASE_SPEED;
static uint16_t g_motor_spin_speed = DEFAULT_MOTOR_SPIN_SPEED;
static uint16_t g_motor_kick_pwm = DEFAULT_MOTOR_KICK_PWM;
static uint16_t g_motor_kick_time_ms = DEFAULT_MOTOR_KICK_TIME_MS;
static uint16_t g_motor_turn_time_ms = DEFAULT_MOTOR_TURN_TIME_MS;

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

    current_state = MOTOR_STATE_STOP;
}

void motors_set_params(uint16_t base_speed, uint16_t spin_speed, 
                       uint16_t kick_pwm, uint16_t kick_time_ms, 
                       uint16_t turn_time_ms)
{
    g_motor_base_speed = base_speed;
    g_motor_spin_speed = spin_speed;
    g_motor_kick_pwm = kick_pwm;
    g_motor_kick_time_ms = kick_time_ms;
    g_motor_turn_time_ms = turn_time_ms;
}

void motors_stop(void)
{
    gpio_put(PIN_MOTOR_L_FWD, 0);
    gpio_put(PIN_MOTOR_L_BWD, 0);
    gpio_put(PIN_MOTOR_R_FWD, 0);
    gpio_put(PIN_MOTOR_R_BWD, 0);

    pwm_set_gpio_level(PIN_MOTOR_L_PWM, 0);
    pwm_set_gpio_level(PIN_MOTOR_R_PWM, 0);

    current_state = MOTOR_STATE_STOP;
}

void motors_forward(void)
{
    gpio_put(PIN_MOTOR_L_FWD, 1);
    gpio_put(PIN_MOTOR_L_BWD, 0);
    gpio_put(PIN_MOTOR_R_FWD, 1);
    gpio_put(PIN_MOTOR_R_BWD, 0);

    if (current_state != MOTOR_STATE_FORWARD) {
        pwm_set_gpio_level(PIN_MOTOR_L_PWM, g_motor_kick_pwm);
        pwm_set_gpio_level(PIN_MOTOR_R_PWM, g_motor_kick_pwm);
        sleep_ms(g_motor_kick_time_ms);
    }

    pwm_set_gpio_level(PIN_MOTOR_L_PWM, g_motor_base_speed);
    pwm_set_gpio_level(PIN_MOTOR_R_PWM, g_motor_base_speed);
    
    current_state = MOTOR_STATE_FORWARD;
}

void motors_spin_left(void)
{
    gpio_put(PIN_MOTOR_L_FWD, 0);
    gpio_put(PIN_MOTOR_L_BWD, 1);
    gpio_put(PIN_MOTOR_R_FWD, 1);
    gpio_put(PIN_MOTOR_R_BWD, 0);

    if (current_state != MOTOR_STATE_LEFT) {
        pwm_set_gpio_level(PIN_MOTOR_L_PWM, g_motor_kick_pwm);
        pwm_set_gpio_level(PIN_MOTOR_R_PWM, g_motor_kick_pwm);
        sleep_ms(g_motor_kick_time_ms);
    }

    pwm_set_gpio_level(PIN_MOTOR_L_PWM, g_motor_base_speed);
    pwm_set_gpio_level(PIN_MOTOR_R_PWM, g_motor_spin_speed);

    current_state = MOTOR_STATE_LEFT;
}

void motors_spin_right(void)
{
    gpio_put(PIN_MOTOR_L_FWD, 1);
    gpio_put(PIN_MOTOR_L_BWD, 0);
    gpio_put(PIN_MOTOR_R_FWD, 0);
    gpio_put(PIN_MOTOR_R_BWD, 1);

    if (current_state != MOTOR_STATE_RIGHT) {
        pwm_set_gpio_level(PIN_MOTOR_L_PWM, g_motor_kick_pwm);
        pwm_set_gpio_level(PIN_MOTOR_R_PWM, g_motor_kick_pwm);
        sleep_ms(g_motor_kick_time_ms);
    }

    pwm_set_gpio_level(PIN_MOTOR_L_PWM, g_motor_base_speed);
    pwm_set_gpio_level(PIN_MOTOR_R_PWM, g_motor_spin_speed);

    current_state = MOTOR_STATE_RIGHT;
}

void motors_spin_obstacle(void)
{
    /* Spin in place: both motors at same speed, opposite directions */
    gpio_put(PIN_MOTOR_L_FWD, 1);
    gpio_put(PIN_MOTOR_L_BWD, 0);
    gpio_put(PIN_MOTOR_R_FWD, 0);
    gpio_put(PIN_MOTOR_R_BWD, 1);

    /* Apply kick since this is usually a start from stop or sudden change */
    pwm_set_gpio_level(PIN_MOTOR_L_PWM, g_motor_kick_pwm);
    pwm_set_gpio_level(PIN_MOTOR_R_PWM, g_motor_kick_pwm);
    sleep_ms(g_motor_kick_time_ms);

    pwm_set_gpio_level(PIN_MOTOR_L_PWM, g_motor_spin_speed);
    pwm_set_gpio_level(PIN_MOTOR_R_PWM, g_motor_spin_speed);
    
    current_state = MOTOR_STATE_RIGHT; /* Same as spin right */

    sleep_ms(g_motor_turn_time_ms);
}
