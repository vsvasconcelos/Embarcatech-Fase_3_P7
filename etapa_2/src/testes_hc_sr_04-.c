//libraries we need
#include <stdio.h>
#include <math.h>
#include "pico/stdlib.h"
#include "hardware/gpio.h"
#include "hardware/timer.h"

volatile int start_ticks = 0;
volatile int end_ticks = 0;
volatile bool timer_fired = 0;

// Portas sensor ultrassom
#define TRIG_PIN 8
#define ECHO_PIN 9


// Tratamento da interrupção do GPIO
void gpio_callback(uint gpio, uint32_t events){
    if (events == 0x4){ // fall edge
        end_ticks = to_us_since_boot(get_absolute_time());
    } else if (events == 0x8){ // rise edge
        start_ticks = to_us_since_boot(get_absolute_time());
    }
}

// Tratamento da interrupção do alarm (timer)
int64_t alarm_callback(alarm_id_t id, void *user_data){
    timer_fired = 1;
    return 0;
}

// Função medição da distância
float mede_distancia() {
    // Seta o trig do sensor
    gpio_put(TRIG_PIN, 1);
    // Habilita alarme de 10 us
    alarm_id_t alarm = add_alarm_in_us(10, alarm_callback, NULL, false);
    // Verifica o termino da contagem de 10 us
    if (timer_fired == 1){
        gpio_put(TRIG_PIN, 0); // dispara interrupção
        timer_fired = 0;       // reseta a flag do alarme
    }
    // Calcula a diferença de tempo
    uint32_t elapsed_time_us = end_ticks - start_ticks;
    // Calcula distância conforme formula do datasheet
    float distance_cm = elapsed_time_us * 0.0343 / 2;
    // Zera variáveis
    start_ticks = 0;
    end_ticks = 0;
    return distance_cm;
}


int main() {
    stdio_init_all();
    sleep_ms(2000);  // Wait for sensor to stabilize

    // Configura HC-SR04
    gpio_init(ECHO_PIN);
    gpio_set_dir(ECHO_PIN, GPIO_IN);
    gpio_init(TRIG_PIN);
    gpio_set_dir(TRIG_PIN, GPIO_OUT);
    // Habilita interrupção no pino Eco
    gpio_set_irq_enabled_with_callback(ECHO_PIN,
            GPIO_IRQ_EDGE_RISE |
            GPIO_IRQ_EDGE_FALL,
            true,
            &gpio_callback);

    while (true) {
      float distance = mede_distancia();
      printf("distancia= %2f\n", distance);
      sleep_ms(1000);
    }
    return 0;
}
