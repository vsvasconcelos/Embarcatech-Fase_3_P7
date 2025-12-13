cmake_minimum_required(VERSION 3.13)

# 1. Definir as configurações GLOBAIS de forma a garantir que:
#  - server.c seja compilado com 3 conexões; e
#  - A btstack (dentro do SDK) seja recompilado com 3 conexões
# Estas definições são macros que são passadas para o compilador
# Elas controlam o comportamento da pilha BTstack e do sistema
# garantindo que o SDK e a pilha Bluetooth vejam esses valores
# no momento da compilação
add_compile_definitions(
    # ---------------------- Aumentar limites de conexão ----------------------
    # Para permitir que múltiplos devices conectados
    # MAX_NR_HCI_CONNECTIONS:
    # Número máximo de conexões HCI (Host Controller Interface)
    # Cada cliente BLE conectado consome uma conexão HCI
    MAX_NR_HCI_CONNECTIONS=3
    # MAX_NR_GATT_CLIENTS:
    # Nº máximo de clientes GATT que o servidor pode gerenciar
    # Deve ser >= MAX_NR_HCI_CONNECTIONS para lidar com cada cliente
    MAX_NR_GATT_CLIENTS=3
    # MAX_NR_L2CAP_SERVICES:
    # Nº Máx de serviços L2CAP (Logical Link Control and Adaptation Protocol)
    # L2CAP é uma camada acima do HCI que multiplexa dados de várias fontes
    MAX_NR_L2CAP_SERVICES=3
    # MAX_NR_L2CAP_CHANNELS:
    # Nº máximo de canais L2CAP. Cada conexão pode ter múltiplos canais
    MAX_NR_L2CAP_CHANNELS=3
    # MAX_NR_SM_LOOKUP_ENTRIES:
    # Nº máximo de entradas na tabela de look-up do Security Manager (SM)
    # Usado para emparelhamento e gerenciamento de chaves de segurança
    MAX_NR_SM_LOOKUP_ENTRIES=3
    # ---------------------- Aumentar buffers de memória ----------------------
    # Para suportar múltiplos clientes e maiores volumes de dados
    # HCI_ACL_PAYLOAD_SIZE:
    # Tamanho máx do payload de dados em pacotes HCI ACL
    # (Asynchronous Connection-Less). Valores maiores permite transferir
    # mais dados por pacote, melhorando o throughput
    HCI_ACL_PAYLOAD_SIZE=1024
    # MAX_ATT_DB_SIZE:
    # Tamanho máx da tabela de atributos (Attribute Table) do GATT em bytes
    # Cada serviço e característica GATT consome espaço nesta tabela
    # Necessário aumentar se muitos serviços/características existirem
    MAX_ATT_DB_SIZE=512
    # NVM_NUM_DEVICE_DB_ENTRIES:
    # Nº de entradas no banco de dados de dispositivos não-volátil (NVM)
    # Armazena infos sobre dispositivos emparelhados para reconexão rápida
    # ------ Configurações gerais persistência dos dados --------------------
    NVM_NUM_DEVICE_DB_ENTRIES=16
    # NVM_NUM_LINK_KEYS:
    # Nº de chaves de link que podem ser armazenadas em NVM
    # Chaves de link são usadas para segurança e reconexão autenticada
    NVM_NUM_LINK_KEYS=16
)

# Inclui o script de importação do SDK do Raspberry Pi Pico.
# Isso configura o ambiente para construir projetos para o Pico.
include($ENV{PICO_SDK_PATH}/external/pico_sdk_import.cmake)

# Define o nome do projeto e as linguagens de programação utilizadas.
project(UCR C CXX ASM)

# Define o padrão da linguagem C e C++ para o compilador.
# CMAKE_C_STANDARD 11: Usa C11, um padrão moderno da linguagem C.
set(CMAKE_C_STANDARD 11)
# CMAKE_CXX_STANDARD 17: Usa C++17, um padrão moderno da linguagem C++.
set(CMAKE_CXX_STANDARD 17)
# CMAKE_EXPORT_COMPILE_COMMANDS ON: Gera um arquivo compile_commands.json,
# útil para ferramentas de análise de código e IDEs.
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Define a placa alvo como Raspberry Pi Pico W.
# CACHE STRING: Indica que esta é uma variável de cache do CMake.
set(PICO_BOARD pico_w CACHE STRING "Board type")

# Inicializa o SDK do Raspberry Pi Pico.
# Esta função deve ser chamada após definir PICO_BOARD.
pico_sdk_init()

# 2. Forçar a inclusão do diretório atual para encontrar arquivos de
# cabeçalho locais, como btstack_config.h, antes dos cabeçalhos do SDK
# ou de bibliotecas.
# O "BEFORE" garante que as definições locais tenham prioridade.
include_directories(BEFORE .)

# Adiciona o exec. ao projeto, especificando seu arquivo fonte principal.
add_executable(${PROJECT_NAME}
    server.c
    )

# Gera o header da Matriz de LEDs
pico_generate_pio_header(${PROJECT_NAME} ${CMAKE_CURRENT_LIST_DIR}/ws2812.pio)

# Gera o arquivo de cabeçalho GATT (e.g., ucr.h)
# a partir do arquivo .gatt.
# Este cabeçalho conterá os 'handles' dos atributos GATT,
# facilitando o acesso programático.
pico_btstack_make_gatt_header(${PROJECT_NAME} PRIVATE
    "${CMAKE_CURRENT_LIST_DIR}/ucr.gatt")

# Adiciona saídas extras ao processo de build (e.g., arquivos .uf2 para flash).
pico_add_extra_outputs(${PROJECT_NAME})

# Linka as bibliotecas necessárias para o projeto.
# pico_stdlib: Biblioteca padrão do Pico.
# pico_btstack_ble: Biblioteca da pilha BLE do BTstack.
# pico_btstack_cyw43: Driver para o chip CYW43, que fornece o rádio
# Wi-Fi/Bluetooth no Pico W.
# pico_cyw43_arch_none: Configuração de arquitetura do CYW43 para ambiente
# bare-metal (sem OS).
# hardware_adc: Biblioteca para usar o conversor ADC da Pico.
target_link_libraries(${PROJECT_NAME}
    pico_stdlib
    pico_btstack_ble
    pico_btstack_cyw43
    pico_cyw43_arch_none
    hardware_adc
    hardware_pio   # Adicionado para Neopixel
    hardware_dma
    )

# Adiciona o diretório atual aos caminhos de inclusão para o executável.
target_include_directories(${PROJECT_NAME} PRIVATE
    ${CMAKE_CURRENT_LIST_DIR}
    )

# Habilita a saída de console via USB e desabilita via UART.
pico_enable_stdio_usb(${PROJECT_NAME} 1)
pico_enable_stdio_uart(${PROJECT_NAME} 0)
