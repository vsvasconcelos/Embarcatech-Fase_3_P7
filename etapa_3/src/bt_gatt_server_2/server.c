/**
 * server.c - Versão com Delay de Inicialização e Debug de Anúncio
 */
#include <stdio.h>
#include "btstack.h"
#include "pico/cyw43_arch.h"
#include "pico/stdlib.h"

// Header gerado pelo CMake
#include "temp_sensor.h"

// Definição dos dados do anúncio
// Flags + Nome "Pico"
static uint8_t adv_data[] = {
    // Flags general discoverable
    // APP_AD_FLAGS = 0x06
    0x02, BLUETOOTH_DATA_TYPE_FLAGS, 0x06,
    // Name: Pico 00:00:00:00:00:00 (22 chars) = 0x16
    0x17, BLUETOOTH_DATA_TYPE_COMPLETE_LOCAL_NAME, 'P', 'i', 'c', 'o', ' ', '0', '0', ':', '0', '0', ':', '0', '0', ':', '0', '0', ':', '0', '0', ':', '0', '0',
    // Custom Service UUID
    0x03, BLUETOOTH_DATA_TYPE_COMPLETE_LIST_OF_16_BIT_SERVICE_CLASS_UUIDS, 0x1a, 0x18,
};
static const uint8_t adv_data_len = sizeof(adv_data);
int le_notification_enabled;

// --- VARIÁVEIS ---
volatile int VERMELHO = 0;
volatile int VERDE    = 0;
volatile int AZUL     = 0;

volatile int DIREITA  = 0;
volatile int ESQUERDA = 0;
volatile int RETO     = 0;
volatile int PARE     = 1;

static hci_con_handle_t con_handle = HCI_CON_HANDLE_INVALID;

// --- CALLBACKS DE EVENTOS ---
static void packet_handler(uint8_t packet_type, uint16_t channel, uint8_t *packet, uint16_t size) {
    UNUSED(size);
    UNUSED(channel);
    bd_addr_t local_addr;
    if (packet_type != HCI_EVENT_PACKET) return;

    uint8_t event_type = hci_event_packet_get_type(packet);
    switch (event_type) {
        case BTSTACK_EVENT_STATE:
            if (btstack_event_state_get_state(packet) != HCI_STATE_WORKING) return;
            // O Bluetooth só está pronto quando o estado é HCI_STATE_WORKING
            printf("Status BTstack: WORKING via packet_handler.\n");
            printf("--> Configurando pacote de anuncio (Advertisement)...\n");
            gap_local_bd_addr(local_addr);
            printf("BTstack up and running on %s.\n", bd_addr_to_str(local_addr));
            // setup advertisements
            uint16_t adv_int_min = 800;
            uint16_t adv_int_max = 800;
            uint8_t adv_type = 0;
            bd_addr_t null_addr;
            memset(null_addr, 0, 6);
            gap_advertisements_set_params(adv_int_min, adv_int_max, adv_type, 0, null_addr, 0x07, 0x00);
            assert(adv_data_len <= 31); // ble limitation
            gap_advertisements_set_data(adv_data_len, (uint8_t*) adv_data);
            gap_advertisements_enable(1);
            printf("--> Anuncio ATIVADO. Procure por 'UCR' no celular.\n");
            break;

        case HCI_EVENT_DISCONNECTION_COMPLETE:
            PARE = 1;
            le_notification_enabled = 0;
            con_handle = HCI_CON_HANDLE_INVALID;
            printf("Desconectado. Reiniciando anuncio...\n");
            break;

        case ATT_EVENT_MTU_EXCHANGE_COMPLETE:
            con_handle = att_event_mtu_exchange_complete_get_handle(packet);
            printf("Conectado! Handle: 0x%04x\n", con_handle);
            break;
        default:
            break;
    }
}

// --- CALLBACKS ATT (Mantidos iguais) ---
uint16_t att_read_callback(hci_con_handle_t connection_handle, uint16_t att_handle, uint16_t offset, uint8_t * buffer, uint16_t buffer_size) {
    if (att_handle == ATT_CHARACTERISTIC_0000FF11_0000_1000_8000_00805F9B34FB_01_VALUE_HANDLE) {
        if (buffer) {
            if (VERMELHO) buffer[0] = 0x01;
            else if (VERDE) buffer[0] = 0x02;
            else if (AZUL) buffer[0] = 0x03;
            else buffer[0] = 0;
        }
        return 1;
    }
    return 0;
}

int att_write_callback(hci_con_handle_t connection_handle, uint16_t att_handle, uint16_t transaction_mode, uint16_t offset, uint8_t *buffer, uint16_t buffer_size) {
    if (att_handle == ATT_CHARACTERISTIC_0000FF12_0000_1000_8000_00805F9B34FB_01_VALUE_HANDLE) {
        if (buffer_size >= 1) {
            uint8_t cmd = buffer[0];
            printf("Comando recebido: %d\n", cmd);
            // Adicione sua lógica de hardware aqui (ex: gpio_put)
        }
    }
    return 0;
}

// --- HEARTBEAT ---
static btstack_timer_source_t heartbeat;
static void heartbeat_handler(struct btstack_timer_source *ts) {
    static int cor_idx = 0;
    cor_idx = (cor_idx % 3) + 1;

    // Prints de debug que você já viu funcionando
    if (cor_idx == 1) printf("[ROBO] Cor Alvo: VERMELHO\n");
    if (cor_idx == 2) printf("[ROBO] Cor Alvo: VERDE\n");
    if (cor_idx == 3) printf("[ROBO] Cor Alvo: AZUL\n");

    // Pisca LED da placa (Confirmação Visual)
    static int led = 0;
    cyw43_arch_gpio_put(CYW43_WL_GPIO_LED_PIN, led);
    led = !led;

    btstack_run_loop_set_timer(ts, 2000);
    btstack_run_loop_add_timer(ts);
}

// --- MAIN ---
int main() {
    stdio_init_all();

    // 1. DELAY DE SEGURANÇA (5 SEGUNDOS)
    // Dá tempo de abrir o serial e estabilizar a energia
    sleep_ms(5000);
    printf("\n\n--- INICIANDO SISTEMA BITDOGLAB ---\n");

    if (cyw43_arch_init()) {
        printf("ERRO: Falha ao iniciar CYW43\n");
        return -1;
    }

    l2cap_init();
    sm_init();

    att_server_init(profile_data, att_read_callback, att_write_callback);

    static btstack_packet_callback_registration_t hci_callback_registration;
    hci_callback_registration.callback = &packet_handler;
    hci_add_event_handler(&hci_callback_registration);

    att_server_register_packet_handler(packet_handler);

    heartbeat.process = &heartbeat_handler;
    btstack_run_loop_set_timer(&heartbeat, 2000);
    btstack_run_loop_add_timer(&heartbeat);

    hci_power_control(HCI_POWER_ON);

    printf("Loop principal iniciado. Aguardando Bluetooth...\n");
    btstack_run_loop_execute();
    return 0;
}
