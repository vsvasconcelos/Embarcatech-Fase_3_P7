#ifndef _PICO_BTSTACK_CONFIG_COMMON_H
#define _PICO_BTSTACK_CONFIG_COMMON_H

// Verificação de segurança: Garante que a funcionalidade BLE esteja habilitada.\
// Se a macro ENABLE_BLE não estiver definida, um erro de compilação será gerado,\
// lembrando o desenvolvedor de linkar a biblioteca 'pico_btstack_ble'.
#ifndef ENABLE_BLE
#error Please link to pico_btstack_ble
#endif

// --- BTstack features that can be enabled ---
// ENABLE_LE_PERIPHERAL: Habilita o papel de periférico BLE, permitindo que o dispositivo anuncie e aceite conexões.
#define ENABLE_LE_PERIPHERAL
// ENABLE_LOG_INFO: Habilita mensagens de log informativas da pilha BTstack, úteis para depuração.
#define ENABLE_LOG_INFO
// ENABLE_LOG_ERROR: Habilita mensagens de log de erro da pilha BTstack, cruciais para identificar problemas.
#define ENABLE_LOG_ERROR
// ENABLE_PRINTF_HEXDUMP: Habilita a funcionalidade de imprimir dumps hexadecimais, inestimável para analisar pacotes de dados.
#define ENABLE_PRINTF_HEXDUMP

// Seções comentadas para Cliente: Este bloco demonstra como a pilha poderia ser configurada para operar como um cliente BLE.\
// No nosso projeto atual, estamos operando como um servidor, então estas linhas estão desabilitadas.\
// #if RUNNING_AS_CLIENT
// #define ENABLE_LE_CENTRAL
// #define MAX_NR_GATT_CLIENTS 1
// #else
// #define MAX_NR_GATT_CLIENTS 0
// #endif

// --- BTstack configuration: buffers, sizes, ... ---
// Estas são configurações mais profundas da pilha, muitas delas podem ser sobrescritas pelo CMakeLists.txt ou btstack_config.h.\
// No entanto, é bom tê-las aqui para referência e para garantir que valores padrão sensatos sejam aplicados se não forem definidos em outro lugar.\
// #define HCI_OUTGOING_PRE_BUFFER_SIZE 4
// #define HCI_ACL_PAYLOAD_SIZE (255 + 4)
// #define HCI_ACL_CHUNK_SIZE_ALIGNMENT 4
// #define MAX_NR_HCI_CONNECTIONS 1
// #define MAX_NR_SM_LOOKUP_ENTRIES 3
// #define MAX_NR_WHITELIST_ENTRIES 16
// #define MAX_NR_LE_DEVICE_DB_ENTRIES 16

// Limita o número de buffers ACL/SCO usados pela pilha para evitar sobrecarga no barramento compartilhado do CYW43.\
// Isso é vital para garantir que Wi-Fi e Bluetooth possam coexistir sem problemas de desempenho ou perda de dados.
#define MAX_NR_CONTROLLER_ACL_BUFFERS 3
#define MAX_NR_CONTROLLER_SCO_PACKETS 3

// Habilita e configura o controle de fluxo HCI Controller-to-Host.\
// Isso impede que o controlador Bluetooth envie dados mais rápido do que o microcontrolador (host) pode processar,\
// prevenindo sobrecargas de buffer e perda de pacotes.
#define ENABLE_HCI_CONTROLLER_TO_HOST_FLOW_CONTROL
#define HCI_HOST_ACL_PACKET_LEN (255+4) // Tamanho máximo do pacote ACL que o host pode receber.
#define HCI_HOST_ACL_PACKET_NUM 3       // Número de pacotes ACL que o host pode bufferizar.
#define HCI_HOST_SCO_PACKET_LEN 120     // Tamanho máximo do pacote SCO (Synchronous Connection-Oriented) que o host pode receber.
#define HCI_HOST_SCO_PACKET_NUM 3       // Número de pacotes SCO que o host pode bufferizar.

// --- Link Key DB and LE Device DB using TLV on top of Flash Sector interface ---
// NVM (Non-Volatile Memory) é usada para armazenar informações persistentes.\
// NVM_NUM_DEVICE_DB_ENTRIES: Número de entradas no banco de dados de dispositivos LE em flash.\
//                            Armazena endereços e informações de emparelhamento de dispositivos conhecidos.
#define NVM_NUM_DEVICE_DB_ENTRIES 16
// NVM_NUM_LINK_KEYS: Número de chaves de link que podem ser armazenadas em flash.\
//                    Chaves de link são usadas para reconexão segura com dispositivos emparelhados.
#define NVM_NUM_LINK_KEYS 16

// MAX_ATT_DB_SIZE: Tamanho máximo da tabela de atributos (Attribute Table) do GATT em bytes.\
// Como o BTstack não usa malloc/free no Pico, a tabela de atributos tem um tamanho fixo.\
// Este valor deve ser grande o suficiente para acomodar todos os serviços e características do seu perfil GATT.
#define MAX_ATT_DB_SIZE 512

// --- BTstack HAL configuration (Hardware Abstraction Layer) ---
// HAVE_EMBEDDED_TIME_MS: Indica que o sistema possui uma forma de obter o tempo em milissegundos.\
//                        Essencial para funções de temporização da pilha.
#define HAVE_EMBEDDED_TIME_MS
// HAVE_ASSERT: Mapeia o btstack_assert para a função assert() do Pico SDK.\
//              Útil para depuração, pois interrupções em condições críticas resultam em falhas explícitas.
#define HAVE_ASSERT
// HCI_RESET_RESEND_TIMEOUT_MS: Define o tempo limite para reenviar um comando HCI Reset.\
//                              Alguns dongles Bluetooth podem levar mais tempo para responder.
#define HCI_RESET_RESEND_TIMEOUT_MS 1000
// ENABLE_SOFTWARE_AES128: Habilita a implementação de software para o algoritmo de criptografia AES128.\
//                         Usado para segurança em conexões BLE.
#define ENABLE_SOFTWARE_AES128
// ENABLE_MICRO_ECC_FOR_LE_SECURE_CONNECTIONS: Habilita a criptografia de Curva Elíptica (ECC) para BLE Secure Connections.\
//                                             Oferece um nível mais alto de segurança para o emparelhamento BLE.
#define ENABLE_MICRO_ECC_FOR_LE_SECURE_CONNECTIONS

#endif // _PICO_BTSTACK_CONFIG_COMMON_H