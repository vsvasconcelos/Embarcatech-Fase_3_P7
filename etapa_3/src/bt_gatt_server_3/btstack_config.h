// btstack_config.h - Configuração Local
#ifndef BTSTACK_CONFIG_H
#define BTSTACK_CONFIG_H

// Este arquivo de cabeçalho define as configurações específicas da pilha BTstack para este projeto.
// Ele pode sobrescrever ou complementar as configurações definidas em `btstack_config_common.h`
// e as macros de compilação do `CMakeLists.txt`, dependendo da ordem de inclusão e definição.

// --- HABILITA FUNCIONALIDADES ---
// ENABLE_LE_PERIPHERAL: Ativa o modo de periférico BLE, permitindo que o dispositivo anuncie e aceite conexões.
#define ENABLE_LE_PERIPHERAL
// ENABLE_LOG_INFO: Ativa logs informativos da pilha, úteis para entender o fluxo de eventos.
#define ENABLE_LOG_INFO
// ENABLE_LOG_ERROR: Ativa logs de erro, essenciais para depuração e identificação de problemas.
#define ENABLE_LOG_ERROR
// ENABLE_PRINTF_HEXDUMP: Permite a impressão de dados em formato hexadecimal, útil para inspecionar pacotes Bluetooth.
#define ENABLE_PRINTF_HEXDUMP

// --- DEFINIÇÃO CRÍTICA: MÚLTIPLOS CLIENTES ---
// Estas macros controlam a capacidade do servidor de gerenciar múltiplas conexões e serviços.
// Elas devem estar alinhadas com as definições em CMakeLists.txt para consistência.
#define MAX_NR_HCI_CONNECTIONS 3        // Número máximo de conexões HCI (Host Controller Interface).
#define MAX_NR_GATT_CLIENTS 3           // Número máximo de clientes GATT que podem se conectar.
#define MAX_NR_SM_LOOKUP_ENTRIES 3      // Número de entradas no Security Manager para emparelhamento.
#define MAX_NR_L2CAP_SERVICES 3         // Número máximo de serviços L2CAP.
#define MAX_NR_L2CAP_CHANNELS 3         // Número máximo de canais L2CAP.

// --- BUFFERS DE MEMÓRIA ---
// NOTA IMPORTANTE: A macro HCI_ACL_PAYLOAD_SIZE já está definida no CMakeLists.txt como 1024.
//                  É uma boa prática remover esta linha daqui para evitar redundância e potenciais conflitos.
// #define HCI_ACL_PAYLOAD_SIZE (255 + 4) // Tamanho do payload de pacotes ACL. Definido aqui para referência, mas sobrescrito pelo CMake.

// HCI_OUTGOING_PRE_BUFFER_SIZE: Tamanho de um buffer de pré-saída HCI.
#define HCI_OUTGOING_PRE_BUFFER_SIZE 4
// HCI_ACL_CHUNK_SIZE_ALIGNMENT: Alinhamento do tamanho do chunk ACL.
#define HCI_ACL_CHUNK_SIZE_ALIGNMENT 4

// --- CONTROLE DE FLUXO E LIMITES ---
// MAX_NR_CONTROLLER_ACL_BUFFERS/SCO_PACKETS: Limita os buffers no controlador para evitar sobrecarga do CYW43.
#define MAX_NR_CONTROLLER_ACL_BUFFERS 3
#define MAX_NR_CONTROLLER_SCO_PACKETS 3
// ENABLE_HCI_CONTROLLER_TO_HOST_FLOW_CONTROL: Habilita o controle de fluxo para evitar que o controlador sature o host.
#define ENABLE_HCI_CONTROLLER_TO_HOST_FLOW_CONTROL

// HCI_HOST_ACL_PACKET_LEN/NUM, HCI_HOST_SCO_PACKET_LEN/NUM: Configurações de tamanho e número de pacotes
//                                                         que o host pode bufferizar para controle de fluxo.
#define HCI_HOST_ACL_PACKET_LEN (255+4)
#define HCI_HOST_ACL_PACKET_NUM 3
#define HCI_HOST_SCO_PACKET_LEN 120
#define HCI_HOST_SCO_PACKET_NUM 3

// --- OUTROS ---
// NVM_NUM_DEVICE_DB_ENTRIES/NVM_NUM_LINK_KEYS: Entradas para o banco de dados de dispositivos e chaves de link em NVM.
#define NVM_NUM_DEVICE_DB_ENTRIES 16
#define NVM_NUM_LINK_KEYS 16
// MAX_ATT_DB_SIZE: Tamanho fixo para o banco de dados ATT, importante para a alocação de memória.
#define MAX_ATT_DB_SIZE 512
// HAVE_EMBEDDED_TIME_MS: Indica suporte a função de tempo em milissegundos.
#define HAVE_EMBEDDED_TIME_MS
// HAVE_ASSERT: Mapeia asserts para o mecanismo do Pico SDK.
#define HAVE_ASSERT
// ENABLE_SOFTWARE_AES128: Habilita criptografia AES128 por software.
#define ENABLE_SOFTWARE_AES128
// ENABLE_MICRO_ECC_FOR_LE_SECURE_CONNECTIONS: Habilita Micro-ECC para conexões BLE seguras.
#define ENABLE_MICRO_ECC_FOR_LE_SECURE_CONNECTIONS

#endif