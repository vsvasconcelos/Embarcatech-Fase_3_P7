/**
 * server.c - Versão Final com Monitoramento Serial Detalhado
 * Projeto: bitdoglab (Robo Bluetooth)
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

// --- VARIÁVEIS GLOBAIS DE ESTADO ---
volatile int VERMELHO = 0;
volatile int VERDE    = 0;
volatile int AZUL     = 0;

volatile int DIREITA  = 0;
volatile int ESQUERDA = 0;
volatile int RETO     = 0;
volatile int PARE     = 1;

// Códigos do Protocolo
#define COR_VERMELHO 0x01
#define COR_VERDE    0x02
#define COR_AZUL     0x03

#define CMD_PARE     0x00
#define CMD_RETO     0x01
#define CMD_ESQUERDA 0x02
#define CMD_DIREITA  0x03

static hci_con_handle_t con_handle = HCI_CON_HANDLE_INVALID;

// --- LÓGICA DE CONTROLE E MONITORAMENTO (ATUALIZADA) ---

void processar_comando(uint8_t comando) {
    // 1. Monitoramento do Dado Bruto
    printf("\n=== [CLIENTE -> SERVIDOR] DADO RECEBIDO ===\n");
    printf("Valor Hex: 0x%02X\n", comando);

    // 2. Reset das variáveis (Exclusividade Mútua)
    DIREITA = 0; ESQUERDA = 0; RETO = 0; PARE = 0;

    const char* status_str = "DESCONHECIDO";

    // 3. Atualização de Estado
    switch (comando) {
        case CMD_RETO:
            RETO = 1;
            status_str = "SEGUIR RETO";
            break;
        case CMD_ESQUERDA:
            ESQUERDA = 1;
            status_str = "VIRAR ESQUERDA";
            break;
        case CMD_DIREITA:
            DIREITA = 1;
            status_str = "VIRAR DIREITA";
            break;
        case CMD_PARE:
        default:
            PARE = 1;
            status_str = "PARAR";
            break;
    }

    // 4. Exibição Detalhada no Terminal
    printf("Acao Interpretada: %s\n", status_str);
    printf("--- ESTADO DAS VARIAVEIS ---\n");
    printf("  [RETO]:     %d\n", RETO);
    printf("  [ESQUERDA]: %d\n", ESQUERDA);
    printf("  [DIREITA]:  %d\n", DIREITA);
    printf("  [PARE]:     %d\n", PARE);
    printf("==========================================\n");
}

void atualizar_cor_alvo(int codigo) {
    VERMELHO = 0; VERDE = 0; AZUL = 0;
    uint8_t valor = 0;

    switch (codigo) {
        case COR_VERMELHO: VERMELHO = 1; valor = COR_VERMELHO; break;
        case COR_VERDE:    VERDE = 1;    valor = COR_VERDE;    break;
        case COR_AZUL:     AZUL = 1;     valor = COR_AZUL;     break;
    }

    if (con_handle != HCI_CON_HANDLE_INVALID) {
        att_server_notify(con_handle, ATT_CHARACTERISTIC_0000FF11_0000_1000_8000_00805F9B34FB_01_VALUE_HANDLE, &valor, 1);
    }
}

// --- CALLBACKS ATT ---

int att_write_callback(hci_con_handle_t connection_handle, uint16_t att_handle, uint16_t transaction_mode, uint16_t offset, uint8_t *buffer, uint16_t buffer_size) {

    // Verifica se a escrita foi na característica de COMANDO (FF12)
    if (att_handle == ATT_CHARACTERISTIC_0000FF12_0000_1000_8000_00805F9B34FB_01_VALUE_HANDLE) {
        if (buffer_size >= 1) {
            // Chama a função de processamento com o dado recebido
            processar_comando(buffer[0]);
        }
    }
    return 0;
}

uint16_t att_read_callback(hci_con_handle_t connection_handle, uint16_t att_handle, uint16_t offset, uint8_t * buffer, uint16_t buffer_size) {
    if (att_handle == ATT_CHARACTERISTIC_0000FF11_0000_1000_8000_00805F9B34FB_01_VALUE_HANDLE) {
        if (buffer) {
            if (VERMELHO) buffer[0] = COR_VERMELHO;
            else if (VERDE) buffer[0] = COR_VERDE;
            else if (AZUL) buffer[0] = COR_AZUL;
            else buffer[0] = 0;
        }
        return 1;
    }
    return 0;
}

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

static btstack_timer_source_t heartbeat;
static void heartbeat_handler(struct btstack_timer_source *ts) {
    static int cor_idx = 0;
    cor_idx = (cor_idx % 3) + 1;

    // Simulação: Apenas mostra o log da cor, não afeta a recepção de dados
    if (cor_idx == 1) printf("[SERVIDOR -> CLIENTE] Notificando Cor: VERMELHO\n");
    // atualizar_cor_alvo(cor_idx); // Descomente para enviar notificação automática

    static int led = 0;
    cyw43_arch_gpio_put(CYW43_WL_GPIO_LED_PIN, led);
    led = !led;

    btstack_run_loop_set_timer(ts, 2000);
    btstack_run_loop_add_timer(ts);
}

// --- MAIN ---
int main() {
    stdio_init_all();
    sleep_ms(5000); // Tempo para abrir o monitor serial

    printf("\n\n--- INICIANDO MONITOR DO BITDOGLAB ---\n");

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

    printf("Aguardando conexao Bluetooth...\n");
    btstack_run_loop_execute();
    return 0;
}
