#include <stdio.h>
#include "pico/stdlib.h"
#include "hardware/pwm.h"
#include "hardware/gpio.h"
#include "hardware/i2c.h"
#include "hardware/timer.h" 

// ==========================================
// CONFIGURAÇÃO DE HARDWARE
// ==========================================
#define LED_PIN 25 

#define LEFT_FWD 4
#define LEFT_BWD 9
#define LEFT_PWM 8
#define RIGHT_FWD 18
#define RIGHT_BWD 19
#define RIGHT_PWM 16
#define STBY 20

#define BASE_SPEED 17000
#define SPIN_SPEED 15000

// ==========================================
// SENSORES
// ==========================================
#define TCS34725_ADDR 0x29
#define TCS34725_COMMAND_BIT 0x80
#define TCS34725_ENABLE 0x00
#define TCS34725_ATIME  0x01
#define TCS34725_CONTROL 0x0F
#define TCS34725_CDATAL 0x14
#define TCS34725_ENABLE_AEN 0x02
#define TCS34725_ENABLE_PON 0x01

#define I2C0_SDA_PIN 0
#define I2C0_SCL_PIN 1
#define I2C1_SDA_PIN 2
#define I2C1_SCL_PIN 3

typedef struct {
    uint16_t r, g, b, c;
    bool valid; 
} ColorData;

typedef enum {
    COR_NENHUMA = 0, 
    COR_AZUL    = 1, 
    COR_VERMELHA= 2, 
    COR_AMARELA = 3  
} TipoCor;

// Variáveis globais para leitura assíncrona
volatile TipoCor cor_esq_atual = COR_NENHUMA;
volatile TipoCor cor_dir_atual = COR_NENHUMA;
volatile bool nova_leitura_disponivel = false;
volatile bool ler_sensor_esquerdo = true;

// --- MOTORES ---
void pwm_setup(uint pin) {
    gpio_set_function(pin, GPIO_FUNC_PWM);
    uint slice = pwm_gpio_to_slice_num(pin);
    pwm_set_wrap(slice, 65535);
    pwm_set_enabled(slice, true);
}

void motors_init() {
    gpio_init(LEFT_FWD);  gpio_set_dir(LEFT_FWD, GPIO_OUT);
    gpio_init(LEFT_BWD);  gpio_set_dir(LEFT_BWD, GPIO_OUT);
    gpio_init(RIGHT_FWD); gpio_set_dir(RIGHT_FWD, GPIO_OUT);
    gpio_init(RIGHT_BWD); gpio_set_dir(RIGHT_BWD, GPIO_OUT);
    gpio_init(STBY);      gpio_set_dir(STBY, GPIO_OUT);
    gpio_put(STBY, 1); 
    pwm_setup(LEFT_PWM);
    pwm_setup(RIGHT_PWM);
}

void run_forward() {
    gpio_put(LEFT_FWD, 1);  gpio_put(LEFT_BWD, 0);
    gpio_put(RIGHT_FWD, 1); gpio_put(RIGHT_BWD, 0);
    pwm_set_gpio_level(LEFT_PWM, BASE_SPEED);
    pwm_set_gpio_level(RIGHT_PWM, BASE_SPEED);
}

void spin_right() {
    gpio_put(LEFT_FWD, 1);  gpio_put(LEFT_BWD, 0);
    gpio_put(RIGHT_FWD, 0); gpio_put(RIGHT_BWD, 1);
    pwm_set_gpio_level(LEFT_PWM, BASE_SPEED);
    pwm_set_gpio_level(RIGHT_PWM, SPIN_SPEED);
}

void spin_left() {
    gpio_put(LEFT_FWD, 0);  gpio_put(LEFT_BWD, 1);
    gpio_put(RIGHT_FWD, 1); gpio_put(RIGHT_BWD, 0);
    pwm_set_gpio_level(LEFT_PWM, BASE_SPEED);
    pwm_set_gpio_level(RIGHT_PWM, SPIN_SPEED);
}

// --- I2C / SENSOR ---
void tcs_write8(i2c_inst_t *i2c, uint8_t reg, uint8_t value) {
    uint8_t buf[2] = {TCS34725_COMMAND_BIT | reg, value};
    i2c_write_blocking(i2c, TCS34725_ADDR, buf, 2, false);
}

void tcs_init(i2c_inst_t *i2c) {
    tcs_write8(i2c, TCS34725_ATIME, 0xF6); // ~24ms (mais rápido)
    tcs_write8(i2c, TCS34725_CONTROL, 0x01); // 4x Gain
    tcs_write8(i2c, TCS34725_ENABLE, TCS34725_ENABLE_PON);
    sleep_ms(3);
    tcs_write8(i2c, TCS34725_ENABLE, TCS34725_ENABLE_PON | TCS34725_ENABLE_AEN);
}

ColorData read_color_fast(i2c_inst_t *i2c) {
    ColorData d;
    d.valid = true;

    uint8_t cmd = TCS34725_COMMAND_BIT | TCS34725_CDATAL;
    uint8_t buf[8];

    int ok1 = i2c_write_blocking(i2c, TCS34725_ADDR, &cmd, 1, true);
    int ok2 = i2c_read_blocking(i2c, TCS34725_ADDR, buf, 8, false);

    if (ok1 < 0 || ok2 < 0) {
        d.valid = false;
        return d;
    }

    d.c = (buf[1] << 8) | buf[0];
    d.r = (buf[3] << 8) | buf[2];
    d.g = (buf[5] << 8) | buf[4];
    d.b = (buf[7] << 8) | buf[6];

    return d;
}

TipoCor identificar_cor(ColorData d) {
    if (d.c < 50) return COR_NENHUMA;
    if (d.r > d.b * 1.5 && d.g > d.b * 1.5) return COR_AMARELA;
    if (d.r > d.g * 1.5 && d.r > d.b * 1.5) return COR_VERMELHA;
    if (d.b > d.r * 1.4) return COR_AZUL;
    return COR_NENHUMA;
}

// Callback do timer - lê sensores alternadamente
bool timer_callback(struct repeating_timer *t) {
    ColorData data;
    
    if (ler_sensor_esquerdo) {
        data = read_color_fast(i2c1);
        if (data.valid) {
            cor_esq_atual = identificar_cor(data);
        }
    } else {
        data = read_color_fast(i2c0);
        if (data.valid) {
            cor_dir_atual = identificar_cor(data);
        }
    }
    
    ler_sensor_esquerdo = !ler_sensor_esquerdo;
    nova_leitura_disponivel = true;
    
    return true; // Continua repetindo
}

// ================= MAIN =================
int main() {
    stdio_init_all();
    
    gpio_init(LED_PIN);
    gpio_set_dir(LED_PIN, GPIO_OUT);
    for(int i=0; i<3; i++) { 
        gpio_put(LED_PIN, 1); sleep_ms(100); 
        gpio_put(LED_PIN, 0); sleep_ms(100);
    }

    sleep_ms(1000);
    printf("--- Leitura otimizada com timer ---\n");

    // I2C em modo rápido (400kHz)
    i2c_init(i2c0, 400 * 1000); 
    gpio_set_function(I2C0_SDA_PIN, GPIO_FUNC_I2C); 
    gpio_set_function(I2C0_SCL_PIN, GPIO_FUNC_I2C);
    gpio_pull_up(I2C0_SDA_PIN); 
    gpio_pull_up(I2C0_SCL_PIN);

    i2c_init(i2c1, 400 * 1000);
    gpio_set_function(I2C1_SDA_PIN, GPIO_FUNC_I2C); 
    gpio_set_function(I2C1_SCL_PIN, GPIO_FUNC_I2C);
    gpio_pull_up(I2C1_SDA_PIN); 
    gpio_pull_up(I2C1_SCL_PIN);

    motors_init();
    tcs_init(i2c0);
    tcs_init(i2c1);

    // Configura timer para ler sensores a cada 30ms alternadamente
    struct repeating_timer timer;
    add_repeating_timer_ms(30, timer_callback, NULL, &timer);

    TipoCor prioridade_ativa = COR_NENHUMA;
    uint32_t fim_do_bloqueio = 0;

    while (true) {
        // Espera nova leitura estar disponível
        if (!nova_leitura_disponivel) {
            sleep_ms(1);
            continue;
        }
        
        nova_leitura_disponivel = false;
        
        // Copia valores voláteis localmente
        TipoCor cor_esq_real = cor_esq_atual;
        TipoCor cor_dir_real = cor_dir_atual;

        // Lógica de prioridade
        TipoCor maior_cor_agora = (cor_dir_real > cor_esq_real) ? cor_dir_real : cor_esq_real;
        uint32_t tempo_agora = to_ms_since_boot(get_absolute_time());

        if (maior_cor_agora > prioridade_ativa) {
            prioridade_ativa = maior_cor_agora;
            fim_do_bloqueio = tempo_agora + 1500;
            printf("Prioridade travada em: %d\n", prioridade_ativa);
        }

        if (tempo_agora > fim_do_bloqueio) {
            prioridade_ativa = COR_NENHUMA;
        }

        // Aplica filtro de bloqueio
        TipoCor cor_esq_final = cor_esq_real;
        TipoCor cor_dir_final = cor_dir_real;

        if (cor_esq_real < prioridade_ativa) cor_esq_final = COR_NENHUMA;
        if (cor_dir_real < prioridade_ativa) cor_dir_final = COR_NENHUMA;

        // Decisão de movimento
        if (cor_dir_final > cor_esq_final) {
            spin_right();
        }
        else if (cor_esq_final > cor_dir_final) {
            spin_left();
        }
        else {
            run_forward();
        }
    }
}