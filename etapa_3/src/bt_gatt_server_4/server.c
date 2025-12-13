/**
 * server.c - BitDogLab UCR (Multi-Client + RGB + Neopixel)
 *
 */

// Cabeçalhos padrão C para I/O, utilitários gerais e manipulação de strings.
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
// Cabeçalho principal da pilha BTstack
#include "btstack.h"
// Cabeçalhos do Pico SDK para interação com o hardware CYW43 (Wi-Fi/BT) e funções de utilidade padrão.
#include "pico/cyw43_arch.h"
#include "pico/stdlib.h"
#include "hardware/pio.h"
#include "hardware/clocks.h"
// Arquivo de cabeçalho gerado automaticamente pela ferramenta BTstack GATT a partir do `ucr.gatt`.
// Ele contém as definições dos handles dos atributos
// Ex:, `ATT_CHARACTERISTIC_0000FF11_0000_1000_8000_00805F9B34FB_01_VALUE_HANDLE`
#include "ucr.h"
// Cabeçalho para controle de LEDs NeoPixel via PIO
#include "ws2812.pio.h"

// --- HARDWARE BITDOGLAB ---
// Led RGB
#define LED_PIN_R 13
#define LED_PIN_G 11
#define LED_PIN_B 12
// Matriz de leds Neopixel
#define NEOPIXEL_PIN 7
#define NEOPIXEL_COUNT 25
#define IS_RGBW false

// --- CONFIGURAÇÕES MULTI-CLIENTE ---
// Define o nº max de conexões BLE que o servidor pode gerenciar, ecoando `MAX_NR_HCI_CONNECTIONS`
// e `MAX_NR_GATT_CLIENTS` do `CMakeLists.txt` e `btstack_config.h'.
// É importante que esses valores estejam alinhados.
#define MAX_CONEXOES 3

// Estrutura para armazenar informações de cada cliente conectado, incluindo o `handle` da conexão HCI e
// o `address` MAC do cliente. A estratégia de cache mencionada no cabeçalho se refere a isso: guardar o
// MAC na conexão para futuras referências.
typedef struct {
    hci_con_handle_t handle;
    bd_addr_t address; // Guarda o MAC Address (6 bytes)
} client_info_t;

// Array de clientes para gerenciar múltiplos clientes.
static client_info_t clientes[MAX_CONEXOES];
// Timer da pilha BTstack para gerenciar a reativação do anúncio BLE.
static btstack_timer_source_t timer_anuncio;

// --- DADOS DO ANÚNCIO ---
// Array de bytes que define os dados do pacote de anúncio BLE.
static uint8_t adv_data[] = {
    // Indica o modo de descoberta, geralmente `0x06` para "LE General Discoverable Mode"
    0x02, BLUETOOTH_DATA_TYPE_FLAGS, 0x06,
    // O nome do dispositivo, neste caso: "Pico".
    //0x05, BLUETOOTH_DATA_TYPE_COMPLETE_LOCAL_NAME, 'P', 'i', 'c', 'o',
    0x0E, BLUETOOTH_DATA_TYPE_COMPLETE_LOCAL_NAME, 'B', 'i', 't', 'D', 'o', 'g', 'L', 'a', 'b', '_', 'U', 'C', 'R',
    // Lista de UUIDs de serviço de 16 bits. `0x1a, 0x18` são para o serviço de informações
    // da bateria e ambiente, respectivamente, que podem ser padrões do BTstack ou de outros exemplos.
    // O importante é que os serviços relevantes estejam aqui.
    0x03, BLUETOOTH_DATA_TYPE_COMPLETE_LIST_OF_16_BIT_SERVICE_CLASS_UUIDS, 0x1a, 0x18,
};
static const uint8_t adv_data_len = sizeof(adv_data);

// --- VARIÁVEIS GLOBAIS VOLÁTEIS PARA ESTADO DO ROBÔ ---
// Estas variáveis `volatile` representam o estado atual do robô. `volatile` é crucial aqui,
// pois essas variáveis podem ser modificadas por interrupções ou outros threads (o BTstack
// opera em um loop de eventos, mas o compilador precisa saber que o valor pode mudar a qualquer momento).
volatile int COR_ATUAL = 0;
// volatile int VERMELHO = 0;
// volatile int VERDE    = 0;
// volatile int AZUL     = 0;
// volatile int DIREITA  = 0;
// volatile int ESQUERDA = 0;
// volatile int RETO     = 0;
// volatile int PARE     = 1;

//  Enums simples para representar os comandos de cor e direção, tornando o código mais legível.
#define COR_OFF      0
#define COR_VERMELHO 1
#define COR_VERDE    2
#define COR_AZUL     3

// #define COR_VERMELHO 0x01
// #define COR_VERDE    0x02
// #define COR_AZUL     0x03
#define CMD_PARE     0x00
#define CMD_RETO     0x01
#define CMD_ESQUERDA 0x02
#define CMD_DIREITA  0x03

// --- CONTROLE NEOPIXEL (PIO) ---
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

// Limpa Matriz
void clear_matrix() {
    for (int i = 0; i < NEOPIXEL_COUNT; i++) put_pixel(0);
}

// Atualiza Matriz de Neopixel com base no comando recebido
void update_matrix(uint8_t comando) {
    uint8_t r = 0, g = 0, b = 0;

    // Define cor base
    if (COR_ATUAL == COR_VERMELHO) r = 50;
    else if (COR_ATUAL == COR_VERDE) g = 50;
    else if (COR_ATUAL == COR_AZUL) b = 50;

    int leds[5][5] = {0}; // Mapa x,y (0-4)

    switch (comando) {
        case CMD_PARE:
            r = 50; g = 50; b = 50; // Branco
            leds[2][2] = 1;
            break;
        case CMD_RETO: // (0,2), (1,3), (2,4), (3,3), (4,2)
            //leds[2][0] = 1; leds[3][1] = 1; leds[4][2] = 1; leds[3][3] = 1; leds[2][4] = 1; // Ajuste x/y fisico
            leds[4][2] = 1; leds[3][1] = 1; leds[3][3] = 1; leds[2][0] = 1; leds[2][4] = 1; // Ajuste x/y fisico
            // Nota: O mapeamento físico x,y pode variar. Ajustando para visual "Seta Frente":
            break;
        case CMD_ESQUERDA: // (0,2), (1,3), (1,1), (2,0), (2,4) ?? Padrão estranho, seguindo pedido:
            leds[2][0] = 1; leds[3][1] = 1; leds[1][1] = 1; leds[0][2] = 1; leds[4][2] = 1;
            break;
        case CMD_DIREITA: // (2,0), (2,4), (3,1), (3,3), (4,2)
            leds[0][2] = 1; leds[4][2] = 1; leds[1][3] = 1; leds[3][3] = 1; leds[2][4] = 1;
            break;
    }

    // Renderiza (Mapeamento ZigZag BitDogLab)
    uint32_t buffer[25] = {0};
    for (int y = 0; y < 5; y++) {
        for (int x = 0; x < 5; x++) {
            if (leds[y][x]) {
                // Cálculo de índice ZigZag: Linhas pares (0,2,4) invertidas?
                // BitDogLab Padrao: Linha 0 (leds 0-4) normal, Linha 1 (leds 9-5) invertida
                // Ajuste este bloco se a matriz estiver rotacionada
                int idx;
                if (y % 2 == 0) idx = (4 - x) + (y * 5); // Invertido (4,3,2,1,0)
                else            idx = x + (y * 5);       // Normal (5,6,7,8,9)

                buffer[idx] = urgb_u32(r, g, b);
            }
        }
    }

    for (int i = 0; i < NEOPIXEL_COUNT; i++) put_pixel(buffer[i]);
}

// --- CONTROLE RGB (BitDogLab) ---
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

// --- FUNÇÕES AUXILIARES ---
//Inicializa o array de clientes, marcando todos os handles como inválidos (`HCI_CON_HANDLE_INVALID`).
void init_conexoes() {
    for (int i = 0; i < MAX_CONEXOES; i++) {
        clientes[i].handle = HCI_CON_HANDLE_INVALID;
    }
}
// Retorna o número de conexões ativas.
int contar_conexoes() {
    int ativas = 0;
    for (int i = 0; i < MAX_CONEXOES; i++) {
        if (clientes[i].handle != HCI_CON_HANDLE_INVALID) ativas++;
    }
    return ativas;
}

// Adiciona um novo cliente ao array `clientes` quando uma conexão é estabelecida,
// armazenando o handle da conexão e o endereço MAC.
// Fundamental para a "estratégia de cache" de MACs.
int adicionar_conexao(hci_con_handle_t handle, bd_addr_t address) {
    // Verifica duplicidade
    for (int i = 0; i < MAX_CONEXOES; i++) {
        if (clientes[i].handle == handle) return 0;
    }

    // Adiciona no slot vazio
    for (int i = 0; i < MAX_CONEXOES; i++) {
        if (clientes[i].handle == HCI_CON_HANDLE_INVALID) {
            clientes[i].handle = handle;
            // Copia o endereço MAC para a estrutura
            memcpy(clientes[i].address, address, 6);

            printf("[SISTEMA] Cliente armazenado no slot %d [%s]\n", i, bd_addr_to_str(address));
            return 1;
        }
    }
    return 0;
}

// Remove um cliente do array quando a conexão é perdida.
void remover_conexao(hci_con_handle_t handle) {
    for (int i = 0; i < MAX_CONEXOES; i++) {
        if (clientes[i].handle == handle) {
            clientes[i].handle = HCI_CON_HANDLE_INVALID;
            printf("[SISTEMA] Cliente removido do slot %d\n", i);
            return;
        }
    }
}

// --- TIMER: REATIVAR ANÚNCIO ---
// Função de callback para o timer que reativa os anúncios BLE (HCI FORCE)
// se houver slots // de conexão disponíveis. Isso é uma otimização: se o
// servidor estiver cheio, // ele para de anunciar para não aceitar mais
// conexões até que uma se desconecte.
static void reativar_anuncio_task(struct btstack_timer_source *ts) {
    int qtd = contar_conexoes();
    if (qtd < MAX_CONEXOES) {
        // printf("[ANUNCIO] Timer: Forcando via HCI (Conexoes: %d/%d)...\n", qtd, MAX_CONEXOES);
        // https://forums.raspberrypi.com/viewtopic.php?t=381611
        hci_send_cmd(&hci_le_set_advertise_enable, 1);
    }
}

// --- LÓGICA DE EVENTOS ---
// Chamado quando a Pi Zero escreve na característica de COMANDO
void processar_comando_direcao(uint8_t comando) {
    printf("[CMD] Direcao Recebida: 0x%02X\n", comando);
    update_matrix(comando);
}

// Chamado quando o App escreve na característica de COR
/*
void processar_mudanca_cor(uint8_t nova_cor) {
    printf("[APP] Nova Cor Recebida: %d\n", nova_cor);
    COR_ATUAL = nova_cor;

    // 1. Atualiza LED RGB Físico
    set_rgb_color(COR_ATUAL);

    // 2. Notifica a Pi Zero (e outros clientes)
    // A Pi Zero deve ter se inscrito (subscribe) nesta característica
    for (int i = 0; i < MAX_CONEXOES; i++) {
        if (clientes[i].handle != HCI_CON_HANDLE_INVALID) {
            att_server_notify(clientes[i].handle, ATT_CHARACTERISTIC_0000FF11_0000_1000_8000_00805F9B34FB_01_VALUE_HANDLE, &nova_cor, 1);
        }
    }

    // Atualiza a matriz imediatamente para refletir a nova cor (mantendo o ultimo comando ou limpando)
    // Por padrão, se mudar a cor, podemos apenas atualizar a cor do LED RGB, a matriz muda no próximo comando de direção.
}
*/
void processar_mudanca_cor(uint8_t nova_cor) {
    printf("[APP] Nova Cor: %d\n", nova_cor);
    COR_ATUAL = nova_cor;
    set_rgb_color(COR_ATUAL);

    // Notifica clientes (ex: Pi Zero)
    for (int i = 0; i < MAX_CONEXOES; i++) {
        if (clientes[i].handle != HCI_CON_HANDLE_INVALID) {
            att_server_notify(clientes[i].handle, ATT_CHARACTERISTIC_0000FF11_0000_1000_8000_00805F9B34FB_01_VALUE_HANDLE, &nova_cor, 1);
        }
    }
    // Atualiza matriz com nova cor (mantendo ultimo desenho ou resetando)
    update_matrix(CMD_PARE);
}

// --- CALLBACKS ATT ---
// Função de callback principal para quando um cliente escreve em uma característica GATT.
// Verifica se o `att_handle` corresponde à característica de "Comando de Direção" (FF12) O
// número mágico `0x0009` é um atalho ou um handle de teste para essa mesma característica.
int att_write_callback(hci_con_handle_t connection_handle, uint16_t att_handle, uint16_t transaction_mode, uint16_t offset, uint8_t *buffer, uint16_t buffer_size) {
    if (buffer_size < 1) return 0;

    // App Escrevendo Cor (FF11)
    if (att_handle == ATT_CHARACTERISTIC_0000FF11_0000_1000_8000_00805F9B34FB_01_VALUE_HANDLE) {
        processar_mudanca_cor(buffer[0]);
    }
    // Pi Zero Escrevendo Comando (FF12)
    // OBS: Verifique o ID real gerado no ucr.h se necessario
    else if (att_handle == ATT_CHARACTERISTIC_0000FF12_0000_1000_8000_00805F9B34FB_01_VALUE_HANDLE || att_handle == 0x0009) {
        processar_comando_direcao(buffer[0]);
    }
    return 0;
}


// Função de callback principal para quando um cliente lê de uma característica GATT
// Verifica se o `att_handle` corresponde à característica de "Cor do Alvo" (FF11).
// Preenche o buffer com o código da cor atual baseado nas variáveis globais `VERMELHO`, `VERDE`, `AZUL`.
uint16_t att_read_callback(hci_con_handle_t connection_handle, uint16_t att_handle, uint16_t offset, uint8_t * buffer, uint16_t buffer_size) {
    if (att_handle == ATT_CHARACTERISTIC_0000FF11_0000_1000_8000_00805F9B34FB_01_VALUE_HANDLE) {
        if (buffer) buffer[0] = COR_ATUAL;
        return 1;
    }
    return 0;
}

// --- PACKET HANDLER ---
// Função de callback central para todos os eventos da pilha HCI (Bluetooth Host Controller Interface).
// É aqui que o servidor reage a eventos como o estado da pilha, novas conexões e desconexões.
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
                    printf("Conectado: %s\n", bd_addr_to_str(addr));
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

// --- MAIN ---
int main() {
    // Inicializa todas as funcionalidades de I/O padrão (USB serial, etc.)
    stdio_init_all();
    // Um atraso inicial para dar tempo ao usuário de abrir um terminal serial.
    sleep_ms(5000);

    init_rgb_led();
    init_neopixel();

    // Check de Hardware
    set_rgb_color(COR_VERMELHO); sleep_ms(200);
    set_rgb_color(COR_OFF);
    clear_matrix();
    update_matrix(CMD_PARE);

    init_conexoes();
    printf("\n\n------- SERVER ROBO Seguidor de Faixa ------\n");
    printf("-------- Aguardando conexões dos clientes GATT--------\n");
    if (cyw43_arch_init()){
        printf("ERRO: Falha ao iniciar CYW43\n");
        return -1;
    }
    l2cap_init();
    sm_init();
    att_server_init(profile_data, att_read_callback, att_write_callback);

    static btstack_packet_callback_registration_t hci_cb;
    hci_cb.callback = &packet_handler;
    hci_add_event_handler(&hci_cb);
    att_server_register_packet_handler(packet_handler);

    hci_power_control(HCI_POWER_ON);
    btstack_run_loop_execute();
    return 0;
}
