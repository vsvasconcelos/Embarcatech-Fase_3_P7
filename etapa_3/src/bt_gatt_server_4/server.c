/**
 * server.c - Versão V8 (Log de Desconexão com MAC)
 * - Funcionalidade V7 mantida.
 * - Log "[SIS] Desconectado" agora mostra quem saiu.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "btstack.h"
#include "pico/cyw43_arch.h"
#include "pico/stdlib.h"
#include "hardware/pio.h"
#include "hardware/clocks.h"
#include "ucr.h"
#include "ws2812.pio.h"

// --- HARDWARE ---
#define LED_PIN_R 13
#define LED_PIN_G 11
#define LED_PIN_B 12
#define NEOPIXEL_PIN 7
#define NEOPIXEL_COUNT 25
#define IS_RGBW false

// --- BLE CONFIG ---
#define MAX_CONEXOES 3

typedef struct {
    hci_con_handle_t handle;
    bd_addr_t address;
} client_info_t;

static client_info_t clientes[MAX_CONEXOES];
static btstack_timer_source_t timer_anuncio;

static uint8_t adv_data[] = {
    0x02, BLUETOOTH_DATA_TYPE_FLAGS, 0x06,
    0x0E, BLUETOOTH_DATA_TYPE_COMPLETE_LOCAL_NAME, 'B', 'i', 't', 'D', 'o', 'g', 'L', 'a', 'b', '_', 'U', 'C', 'R',
    0x03, BLUETOOTH_DATA_TYPE_COMPLETE_LIST_OF_16_BIT_SERVICE_CLASS_UUIDS, 0x1a, 0x18,
};
static const uint8_t adv_data_len = sizeof(adv_data);

// --- ESTADO ---
volatile int COR_ATUAL = 0;
#define COR_OFF      0
#define COR_VERMELHO 1
#define COR_VERDE    2
#define COR_AZUL     3

#define CMD_PARE     0x00
#define CMD_RETO     0x01
#define CMD_ESQUERDA 0x02
#define CMD_DIREITA  0x03

// --- HELPERS DE LOG ---
const char* get_cor_nome(uint8_t cor) {
    switch(cor) {
        case COR_VERMELHO: return "VERMELHO";
        case COR_VERDE:    return "VERDE";
        case COR_AZUL:     return "AZUL";
        case COR_OFF:      return "OFF";
        default:           return "DESCONHECIDO";
    }
}

const char* get_comando_nome(uint8_t cmd) {
    switch(cmd) {
        case CMD_PARE:     return "PARE";
        case CMD_RETO:     return "RETO";
        case CMD_ESQUERDA: return "ESQUERDA";
        case CMD_DIREITA:  return "DIREITA";
        default:           return "DESCONHECIDO";
    }
}

// --- PIO ---
static PIO np_pio = pio0;
static uint np_sm = 0;

void init_neopixel() {
    uint offset = pio_add_program(np_pio, &ws2812_program);
    ws2812_program_init(np_pio, np_sm, offset, NEOPIXEL_PIN, 800000, IS_RGBW);
}

void put_pixel(uint32_t pixel_grb) {
    pio_sm_put_blocking(np_pio, np_sm, pixel_grb << 8u);
}

uint32_t urgb_u32(uint8_t r, uint8_t g, uint8_t b) {
    return ((uint32_t)(r) << 8) | ((uint32_t)(g) << 16) | (uint32_t)(b);
}

// --- PROTOTIPOS ---
void clear_matrix(void);
void update_matrix(uint8_t comando);
void set_rgb_color(int codigo);

void clear_matrix() {
    for (int i = 0; i < NEOPIXEL_COUNT; i++) put_pixel(0);
}

void update_matrix(uint8_t comando) {
    uint8_t r = 0, g = 0, b = 0;
    if (COR_ATUAL == COR_VERMELHO) r = 50;
    else if (COR_ATUAL == COR_VERDE) g = 50;
    else if (COR_ATUAL == COR_AZUL) b = 50;

    int leds[5][5] = {0};
    switch (comando) {
        case CMD_PARE:     r=50; g=50; b=50; leds[2][2]=1; break;
        case CMD_RETO:     leds[4][2] = 1; leds[3][1] = 1; leds[3][3] = 1; leds[2][0] = 1; leds[2][4] = 1; break;
        case CMD_ESQUERDA: leds[2][0] = 1; leds[3][1] = 1; leds[1][1] = 1; leds[0][2] = 1; leds[4][2] = 1; break;
        case CMD_DIREITA:  leds[0][2] = 1; leds[4][2] = 1; leds[1][3] = 1; leds[3][3] = 1; leds[2][4] = 1; break;
    }

    uint32_t buffer[25] = {0};
    for (int y = 0; y < 5; y++) {
        for (int x = 0; x < 5; x++) {
            if (leds[y][x]) {
                int idx;
                if (y % 2 == 0) idx = (4 - x) + (y * 5);
                else            idx = x + (y * 5);
                buffer[idx] = urgb_u32(r, g, b);
            }
        }
    }
    for (int i = 0; i < NEOPIXEL_COUNT; i++) put_pixel(buffer[i]);
}

void init_rgb_led() {
    gpio_init(LED_PIN_R); gpio_set_dir(LED_PIN_R, GPIO_OUT);
    gpio_init(LED_PIN_G); gpio_set_dir(LED_PIN_G, GPIO_OUT);
    gpio_init(LED_PIN_B); gpio_set_dir(LED_PIN_B, GPIO_OUT);
    gpio_put(LED_PIN_R, 0); gpio_put(LED_PIN_G, 0); gpio_put(LED_PIN_B, 0);
}

void set_rgb_color(int codigo) {
    gpio_put(LED_PIN_R, (codigo == COR_VERMELHO));
    gpio_put(LED_PIN_G, (codigo == COR_VERDE));
    gpio_put(LED_PIN_B, (codigo == COR_AZUL));
}

// --- BLE HELPER ---
void init_conexoes() {
    for (int i = 0; i < MAX_CONEXOES; i++) clientes[i].handle = HCI_CON_HANDLE_INVALID;
}

int contar_conexoes() {
    int qtd = 0;
    for (int i = 0; i < MAX_CONEXOES; i++) if (clientes[i].handle != HCI_CON_HANDLE_INVALID) qtd++;
    return qtd;
}

int adicionar_conexao(hci_con_handle_t handle, bd_addr_t address) {
    for (int i = 0; i < MAX_CONEXOES; i++) if (clientes[i].handle == handle) return 0;
    for (int i = 0; i < MAX_CONEXOES; i++) {
        if (clientes[i].handle == HCI_CON_HANDLE_INVALID) {
            clientes[i].handle = handle;
            memcpy(clientes[i].address, address, 6);
            printf("[SIS] Conectado: %s\n", bd_addr_to_str(address));
            return 1;
        }
    }
    return 0;
}

void remover_conexao(hci_con_handle_t handle) {
    for (int i = 0; i < MAX_CONEXOES; i++) {
        if (clientes[i].handle == handle) {
            // [ALTERADO] Imprime o MAC antes de invalidar o slot
            printf("[SIS] Desconectado: %s\n", bd_addr_to_str(clientes[i].address));

            clientes[i].handle = HCI_CON_HANDLE_INVALID;
            return;
        }
    }
}

bd_addr_t* get_client_address(hci_con_handle_t handle) {
    for (int i = 0; i < MAX_CONEXOES; i++) {
        if (clientes[i].handle == handle) return &clientes[i].address;
    }
    return NULL;
}

static void reativar_anuncio_task(struct btstack_timer_source *ts) {
    if (contar_conexoes() < MAX_CONEXOES) {
        hci_send_cmd(&hci_le_set_advertise_enable, 1);
    }
}

// --- LOGICA ---
void processar_comando_direcao(hci_con_handle_t handle, uint8_t comando) {
    bd_addr_t* addr = get_client_address(handle);
    printf("[CMD] %s (0x%02X) de %s\n",
           get_comando_nome(comando),
           comando,
           addr ? bd_addr_to_str(*addr) : "Desc.");

    update_matrix(comando);
}

void processar_mudanca_cor(hci_con_handle_t handle, uint8_t nova_cor) {
    bd_addr_t* addr = get_client_address(handle);
    printf("[APP] Cor %d (%s) de %s\n",
           nova_cor,
           get_cor_nome(nova_cor),
           addr ? bd_addr_to_str(*addr) : "Desc.");

    COR_ATUAL = (int)nova_cor;
    set_rgb_color(COR_ATUAL);

    for (int i = 0; i < MAX_CONEXOES; i++) {
        if (clientes[i].handle != HCI_CON_HANDLE_INVALID) {
            att_server_notify(clientes[i].handle,
                              ATT_CHARACTERISTIC_0000FF11_0000_1000_8000_00805F9B34FB_01_VALUE_HANDLE,
                              &nova_cor, 1);
        }
    }
    update_matrix(CMD_PARE);
}

int att_write_callback(hci_con_handle_t connection_handle, uint16_t att_handle, uint16_t transaction_mode, uint16_t offset, uint8_t *buffer, uint16_t buffer_size) {
    if (buffer_size < 1) return 0;

    if (att_handle == ATT_CHARACTERISTIC_0000FF11_0000_1000_8000_00805F9B34FB_01_VALUE_HANDLE) {
        processar_mudanca_cor(connection_handle, buffer[0]);
    }
    else if (att_handle == ATT_CHARACTERISTIC_0000FF12_0000_1000_8000_00805F9B34FB_01_VALUE_HANDLE || att_handle == 0x0009) {
        processar_comando_direcao(connection_handle, buffer[0]);
    }
    return 0;
}

uint16_t att_read_callback(hci_con_handle_t connection_handle, uint16_t att_handle, uint16_t offset, uint8_t * buffer, uint16_t buffer_size) {
    if (att_handle == ATT_CHARACTERISTIC_0000FF11_0000_1000_8000_00805F9B34FB_01_VALUE_HANDLE) {
        if (buffer) buffer[0] = (uint8_t)COR_ATUAL;
        return 1;
    }
    return 0;
}

static void packet_handler(uint8_t packet_type, uint16_t channel, uint8_t *packet, uint16_t size) {
    if (packet_type != HCI_EVENT_PACKET) return;
    uint8_t event_type = hci_event_packet_get_type(packet);

    switch (event_type) {
        case BTSTACK_EVENT_STATE:
            if (btstack_event_state_get_state(packet) != HCI_STATE_WORKING) return;

            gap_advertisements_set_params(800, 800, 0, 0x07, NULL, 0x00, 0x00);
            gap_advertisements_set_data(adv_data_len, (uint8_t*) adv_data);
            gap_advertisements_enable(1);

            btstack_run_loop_remove_timer(&timer_anuncio);
            timer_anuncio.process = &reativar_anuncio_task;
            btstack_run_loop_set_timer(&timer_anuncio, 100);
            btstack_run_loop_add_timer(&timer_anuncio);
            break;

        case HCI_EVENT_LE_META:
            if (hci_event_le_meta_get_subevent_code(packet) == HCI_SUBEVENT_LE_CONNECTION_COMPLETE) {
                hci_con_handle_t h = hci_subevent_le_connection_complete_get_connection_handle(packet);
                bd_addr_t addr;
                hci_subevent_le_connection_complete_get_peer_address(packet, addr);

                if (adicionar_conexao(h, addr)) {
                    gap_request_connection_parameter_update(h, 80, 120, 0, 500);
                    btstack_run_loop_remove_timer(&timer_anuncio);
                    timer_anuncio.process = &reativar_anuncio_task;
                    btstack_run_loop_set_timer(&timer_anuncio, 1000);
                    btstack_run_loop_add_timer(&timer_anuncio);
                }
            }
            break;

        case HCI_EVENT_DISCONNECTION_COMPLETE:
            remover_conexao(hci_event_disconnection_complete_get_connection_handle(packet));
            btstack_run_loop_remove_timer(&timer_anuncio);
            timer_anuncio.process = &reativar_anuncio_task;
            btstack_run_loop_set_timer(&timer_anuncio, 50);
            btstack_run_loop_add_timer(&timer_anuncio);
            break;
    }
}

// --- HEARTBEAT ---
static btstack_timer_source_t heartbeat;
static void heartbeat_handler(struct btstack_timer_source *ts) {
    static int led = 0;
    cyw43_arch_gpio_put(CYW43_WL_GPIO_LED_PIN, led);
    led = !led;

    // Sincronizacao periodica
    if (contar_conexoes() > 0) {
        uint8_t cor_byte = (uint8_t)COR_ATUAL;
        for (int i = 0; i < MAX_CONEXOES; i++) {
            if (clientes[i].handle != HCI_CON_HANDLE_INVALID) {
                att_server_notify(clientes[i].handle,
                                  ATT_CHARACTERISTIC_0000FF11_0000_1000_8000_00805F9B34FB_01_VALUE_HANDLE,
                                  &cor_byte, 1);
            }
        }
    }
    btstack_run_loop_set_timer(ts, 2000);
    btstack_run_loop_add_timer(ts);
}

// --- MAIN ---
int main() {
    stdio_init_all();
    sleep_ms(2000);

    init_rgb_led();
    init_neopixel();

    set_rgb_color(COR_VERMELHO); sleep_ms(200);
    set_rgb_color(COR_OFF);
    clear_matrix();
    update_matrix(CMD_PARE);

    init_conexoes();
    if (cyw43_arch_init()) return -1;
    l2cap_init();
    sm_init();
    att_server_init(profile_data, att_read_callback, att_write_callback);

    static btstack_packet_callback_registration_t hci_cb;
    hci_cb.callback = &packet_handler;
    hci_add_event_handler(&hci_cb);
    att_server_register_packet_handler(packet_handler);

    heartbeat.process = &heartbeat_handler;
    btstack_run_loop_set_timer(&heartbeat, 2000);
    btstack_run_loop_add_timer(&heartbeat);

    hci_power_control(HCI_POWER_ON);
    printf("--- SERVER UCR V8 (MAC LOG) ---\n");
    btstack_run_loop_execute();
    return 0;
}
