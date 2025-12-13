/**
 * server.c - Versão FINAL com Identificação de MAC (Estratégia de Cache)
 * Resolve o problema de dependência do gap.h armazenando o endereço na conexão.
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
// Arquivo de cabeçalho gerado automaticamente pela ferramenta BTstack GATT a partir do `ucr.gatt`.
// Ele contém as definições dos handles dos atributos
// Ex:, `ATT_CHARACTERISTIC_0000FF11_0000_1000_8000_00805F9B34FB_01_VALUE_HANDLE`
#include "ucr.h"

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
    0x05, BLUETOOTH_DATA_TYPE_COMPLETE_LOCAL_NAME, 'P', 'i', 'c', 'o',
    // Lista de UUIDs de serviço de 16 bits. `0x1a, 0x18` são para o serviço de informações
    // da bateria e ambiente, respectivamente, que podem ser padrões do BTstack ou de outros exemplos.
    // O importante é que os serviços relevantes estejam aqui.

    //0x03, BLUETOOTH_DATA_TYPE_COMPLETE_LIST_OF_16_BIT_SERVICE_CLASS_UUIDS, 0x1a, 0x18,
};
static const uint8_t adv_data_len = sizeof(adv_data);

// --- VARIÁVEIS GLOBAIS VOLÁTEIS PARA ESTADO DO ROBÔ ---
// Estas variáveis `volatile` representam o estado atual do robô. `volatile` é crucial aqui,
// pois essas variáveis podem ser modificadas por interrupções ou outros threads (o BTstack
// opera em um loop de eventos, mas o compilador precisa saber que o valor pode mudar a qualquer momento).
volatile int VERMELHO = 0;
volatile int VERDE    = 0;
volatile int AZUL     = 0;
volatile int DIREITA  = 0;
volatile int ESQUERDA = 0;
volatile int RETO     = 0;
volatile int PARE     = 1;

//  Enums simples para representar os comandos de cor e direção, tornando o código mais legível.
#define COR_VERMELHO 0x01
#define COR_VERDE    0x02
#define COR_AZUL     0x03
#define CMD_PARE     0x00
#define CMD_RETO     0x01
#define CMD_ESQUERDA 0x02
#define CMD_DIREITA  0x03

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

// --- LÓGICA DE CONTROLE ---
// Recebe um comando de direção (0x00, 0x01, etc.) e o handle da conexão de origem.
// Atualiza as variáveis globais de direção (`DIREITA`, `ESQUERDA`, etc.).
// A capacidade de identificar a origem do comando pelo MAC address permite
// logs mais detalhados ou até mesmo políticas de acesso por cliente
void processar_comando(uint8_t comando, hci_con_handle_t handle_origem) {
    // Busca quem enviou o comando na nossa lista
    char origem_str[30] = "DESCONHECIDO";

    for (int i = 0; i < MAX_CONEXOES; i++) {
        if (clientes[i].handle == handle_origem) {
            sprintf(origem_str, "%s", bd_addr_to_str(clientes[i].address));
            break;
        }
    }
    const char* acao_str = "DESCONHECIDO";
    DIREITA = 0; ESQUERDA = 0; RETO = 0; PARE = 0;

    switch (comando) {
        case CMD_RETO:     RETO = 1;     acao_str = "SEGUIR RETO"; break;
        case CMD_ESQUERDA: ESQUERDA = 1; acao_str = "VIRAR ESQUERDA"; break;
        case CMD_DIREITA:  DIREITA = 1;  acao_str = "VIRAR DIREITA"; break;
        case CMD_PARE:     PARE = 1;     acao_str = "PARAR"; break;
        default:           PARE = 1;     acao_str = "PARAR (Cmd Invalido)"; break;
    }
    // LOG Formatado com Origem
    printf("\n>>> [COMANDO] Origem: %s | Acao: %s (0x%02X)\n", origem_str, acao_str, comando);
}

// ------------- Comunicação Server-to-Client ---------------------------------
// Recebe um código de cor e atualiza as variáveis globais de cor (`VERMELHO`, `VERDE`, `AZUL`).
// E ainda envia uma notificação BLE para todos os clientes conectados sobre a nova cor alvo usando
// 'att_server_notify()`.
void atualizar_cor_alvo(int codigo) {
    VERMELHO = 0; VERDE = 0; AZUL = 0;
    uint8_t valor = 0;
    const char* cor_str = "NENHUMA";

    switch (codigo) {
        case COR_VERMELHO: VERMELHO = 1; valor = COR_VERMELHO; cor_str = "VERMELHO"; break;
        case COR_VERDE:    VERDE = 1;    valor = COR_VERDE;    cor_str = "VERDE";    break;
        case COR_AZUL:     AZUL = 1;     valor = COR_AZUL;     cor_str = "AZUL";     break;
    }

    if (contar_conexoes() > 0) {
        printf("[NOTIFICACAO] Enviando COR: %s\n", cor_str);
    }

    for (int i = 0; i < MAX_CONEXOES; i++) {
        if (clientes[i].handle != HCI_CON_HANDLE_INVALID) {
            att_server_notify(clientes[i].handle, ATT_CHARACTERISTIC_0000FF11_0000_1000_8000_00805F9B34FB_01_VALUE_HANDLE, &valor, 1);
        }
    }
}

// --- CALLBACKS ATT ---
// Função de callback principal para quando um cliente escreve em uma característica GATT.
// Verifica se o `att_handle` corresponde à característica de "Comando de Direção" (FF12) O
// número mágico `0x0009` é um atalho ou um handle de teste para essa mesma característica.
int att_write_callback(hci_con_handle_t connection_handle,
    uint16_t att_handle,
    uint16_t transaction_mode,
    uint16_t offset,
    uint8_t *buffer,
    uint16_t buffer_size) {
    if (buffer_size > 0) {
        // printf("[DEBUG WRITE] Handle: 0x%04X | Valor: 0x%02X\n", att_handle, buffer[0]);
    }

    if (att_handle == ATT_CHARACTERISTIC_0000FF12_0000_1000_8000_00805F9B34FB_01_VALUE_HANDLE || att_handle == 0x0009) {
        if (buffer_size >= 1) {
            // ------------- Comunicação Client-to-Server ---------------------------------
            // Passamos o handle da conexão para identificar a origem Com o valor recebido e
            // o `connection_handle` para que o comando seja executado.
            processar_comando(buffer[0], connection_handle);
        }
    }
    return 0;
}
// Função de callback principal para quando um cliente lê de uma característica GATT
// Verifica se o `att_handle` corresponde à característica de "Cor do Alvo" (FF11).
// Preenche o buffer com o código da cor atual baseado nas variáveis globais `VERMELHO`, `VERDE`, `AZUL`.
uint16_t att_read_callback(hci_con_handle_t connection_handle,
    uint16_t att_handle,
    uint16_t offset,
    uint8_t * buffer,
    uint16_t buffer_size) {
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

// --- PACKET HANDLER ---
// Função de callback central para todos os eventos da pilha HCI (Bluetooth Host Controller Interface).
// É aqui que o servidor reage a eventos como o estado da pilha, novas conexões e desconexões.
static void packet_handler(uint8_t packet_type, uint16_t channel, uint8_t *packet, uint16_t size) {
    UNUSED(size);
    UNUSED(channel);
    bd_addr_t local_addr;

    if (packet_type != HCI_EVENT_PACKET) return;

    uint8_t event_type = hci_event_packet_get_type(packet);

    switch (event_type) {
        // Disparado quando o estado da pilha BTstack muda.
        case BTSTACK_EVENT_STATE:
            // Quando o estado é `HCI_STATE_WORKING, significa que a pilha está pronta.
            if (btstack_event_state_get_state(packet) != HCI_STATE_WORKING) return;
            // Obtém o endereço MAC local (`gap_local_bd_addr`).
            gap_local_bd_addr(local_addr);
            printf("BTstack UP. MAC Local: %s\n", bd_addr_to_str(local_addr));

            // Config Inicial
            uint16_t adv_int_min = 800;
            uint16_t adv_int_max = 800;
            bd_addr_t null_addr;
            memset(null_addr, 0, 6);
            // Configura os parâmetros de anúncio usando `adv_data`
            gap_advertisements_set_params(adv_int_min, adv_int_max, 0, 0x07, null_addr, 0x00, 0x00);
            // Configura os dados de anúncio usando `adv_data`
            gap_advertisements_set_data(adv_data_len, (uint8_t*) adv_data);
            // Habilita os anúncios
            gap_advertisements_enable(1);
            printf("--> Anuncio INICIAL ATIVADO.\n");

            // Timer setup
            // Configura o timer para reativar anúncios se necessário
            btstack_run_loop_remove_timer(&timer_anuncio);
            timer_anuncio.process = &reativar_anuncio_task;
            btstack_run_loop_set_timer(&timer_anuncio, 100);
            btstack_run_loop_add_timer(&timer_anuncio);
            break;

        // Evento de nova conexão BLE
        case HCI_EVENT_LE_META:
            if (hci_event_le_meta_get_subevent_code(packet) == HCI_SUBEVENT_LE_CONNECTION_COMPLETE) {
                // Obtem o `novo_handle` da conexão
                hci_con_handle_t novo_handle = hci_subevent_le_connection_complete_get_connection_handle(packet);

                // Obtem endereco_cliente` (endereço MAC do cliente)
                bd_addr_t endereco_cliente;
                hci_subevent_le_connection_complete_get_peer_address(packet, endereco_cliente);

                // Registrar o novo cliente, passa Handle e Endereço
                if (adicionar_conexao(novo_handle, endereco_cliente)) {
                    printf("--> Conexao 0x%04x estabelecida.\n", novo_handle);
                    // Solicita atualização dos parâmetros de conexão para otimiza-la (latência, intervalo)
                    gap_request_connection_parameter_update(novo_handle, 80, 120, 0, 500);
                    // Agenda reativação
                    btstack_run_loop_remove_timer(&timer_anuncio);
                    timer_anuncio.process = &reativar_anuncio_task;
                    // Reagenda o `timer_anuncio` para um intervalo maior (1000ms),
                    // pois já há uma conexão ativa.
                    btstack_run_loop_set_timer(&timer_anuncio, 1000);
                    btstack_run_loop_add_timer(&timer_anuncio);
                }
            }
            break;
        // Evento de desconexão BLE
        case HCI_EVENT_DISCONNECTION_COMPLETE: {
            // Obtem o handle da conexão que foi desconectada
            hci_con_handle_t handle_desconectado = hci_event_disconnection_complete_get_connection_handle(packet);
            // Remove o cliente do array
            remover_conexao(handle_desconectado);
            printf("--> Conexao 0x%04x desconectada.\n", handle_desconectado);
            // Reagenda o `timer_anuncio` para um intervalo menor (50ms) para que o servidor
            // possa anunciar rapidamente e aceitar novas conexões
            btstack_run_loop_remove_timer(&timer_anuncio);
            timer_anuncio.process = &reativar_anuncio_task;
            btstack_run_loop_set_timer(&timer_anuncio, 50);
            btstack_run_loop_add_timer(&timer_anuncio);
            break;
        }
    }
}

// --- HEARTBEAT ---
static btstack_timer_source_t heartbeat;
// Função de callback do timer "heartbeat" que executa periodicamente.
static void heartbeat_handler(struct btstack_timer_source *ts) {
    // Gera uma cor aleatória
    int cor_aleatoria = (rand() % 3) + 1;
    if (contar_conexoes() == 0 && cor_aleatoria == 1) printf(".\n");
    // Atualiza o estado do robô.
    atualizar_cor_alvo(cor_aleatoria);
    // Pisca o LED do CYW43 para indicar atividade
    static int led = 0;
    cyw43_arch_gpio_put(CYW43_WL_GPIO_LED_PIN, led);
    led = !led;
    // Reagenda o `heartbeat` para rodar novamente após 10 segundos.
    // Este é o "ritmo" do servidor para enviar notificações.
    btstack_run_loop_set_timer(ts, 10000);
    btstack_run_loop_add_timer(ts);
}

// --- MAIN ---
int main() {
    // Inicializa todas as funcionalidades de I/O padrão (USB serial, etc.)
    stdio_init_all();
    // Um atraso inicial para dar tempo ao usuário de abrir um terminal serial.
    sleep_ms(5000);
    // Inicializa o gerador de números aleatórios usando o tempo atual em microssegundos para uma semente.
    srand(time_us_32());
    // Inicializa o array de conexões dos clientes
    init_conexoes();

    printf("\n\n--- SERVER V_FINAL_MAC_ID ---\n");
    // Inicializa o chip CYW43 (Wi-Fi/Bluetooth).
    // Em caso de falha, imprime mensagem de erro!
    if (cyw43_arch_init()) {
        printf("ERRO: Falha ao iniciar CYW43\n");
        return -1;
    }
    // Inicializa a camada L2CAP da pilha BTstack.
    l2cap_init();
    // Inicializa o Security Manager para lidar com emparelhamento e segurança.
    sm_init();
    // Inicializa o servidor GATT, passando a tabela de perfil
    // (gerada pelo `ucr.gatt`) e os callbacks de leitura/escrita.
    att_server_init(profile_data, att_read_callback, att_write_callback);
    // Registra o packet handler para eventos HCI.
    static btstack_packet_callback_registration_t hci_callback_registration;
    hci_callback_registration.callback = &packet_handler;
    // Registra o packet_handler` para processar eventos HCI
    hci_add_event_handler(&hci_callback_registration);
    // Registra o packet handler para processar eventos GATT
    att_server_register_packet_handler(packet_handler);
    // Configura o timer "heartbeat" para enviar notificações periódicas.
    heartbeat.process = &heartbeat_handler;
    btstack_run_loop_set_timer(&heartbeat, 2000);
    btstack_run_loop_add_timer(&heartbeat);
    // Liga o rádio Bluetooth.
    hci_power_control(HCI_POWER_ON);
    // Inicia o loop de eventos do BTstack.É o coração da pilha,
    // onde todos os eventos são processados e os callbacks são chamados.
    // A partir deste ponto, o programa executa em um loop infinito,
    // respondendo a eventos Bluetooth.
    btstack_run_loop_execute();
    return 0;
}
