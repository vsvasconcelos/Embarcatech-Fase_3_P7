# Generated Cmake Pico project file

cmake_minimum_required(VERSION 3.13)

project(tinyml_gate LANGUAGES C CXX ASM)
set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# == DO NOT EDIT THE FOLLOWING LINES for the Raspberry Pi Pico VS Code Extension to work ==
if(WIN32)
    set(USERHOME $ENV{USERPROFILE})
else()
    set(USERHOME $ENV{HOME})
endif()
set(sdkVersion 2.1.1)
set(toolchainVersion 14_2_Rel1)
set(picotoolVersion 2.1.1)
set(picoVscode ${USERHOME}/.pico-sdk/cmake/pico-vscode.cmake)
if (EXISTS ${picoVscode})
    include(${picoVscode})
endif()
# ====================================================================================
set(PICO_BOARD pico_w CACHE STRING "Board type")

# Pull in Raspberry Pi Pico SDK (must be before project)
include(pico_sdk_import.cmake)

# Initialise the Raspberry Pi Pico SDK
pico_sdk_init()

# Adicione após include(pico_sdk_import.cmake)
   set(CMSIS_DSP_PATH ${CMAKE_CURRENT_SOURCE_DIR}/CMSIS-DSP)
   set(CMSIS_NN_PATH ${CMAKE_CURRENT_SOURCE_DIR}/CMSIS-NN)
   
   # Adicione as bibliotecas ao seu target
   #add_subdirectory(${CMSIS_DSP_PATH}/Source EXCLUDE_FROM_ALL)
   #add_subdirectory(${CMSIS_NN_PATH}/Source EXCLUDE_FROM_ALL)

# Biblioteca de configuração do FreeRTOS (NECESSÁRIA)
# O propósito desta biblioteca INTERFACE é fornecer o caminho para o seu arquivo FreeRTOSConfig.h.
# Sem isso, a cadeia de dependências é quebrada e os cabeçalhos de rede não serão encontrados.
add_library(freertos_config INTERFACE)
target_include_directories(freertos_config INTERFACE
    # O diretório que contém o seu FreeRTOSConfig.h
    ${CMAKE_CURRENT_LIST_DIR}/FreeRTOS/include
    # Caminho CRÍTICO para os cabeçalhos portáteis específicos da arquitetura (portmacro.h)
    ${CMAKE_CURRENT_LIST_DIR}/FreeRTOS/portable/GCC/ARM_CM0
)
target_compile_definitions(freertos_config
    INTERFACE
        projCOVERAGE_TEST=0
)
# --- Biblioteca Paho MQTT Embedded C ---
# Define a biblioteca Paho MQTT como uma biblioteca INTERFACE.
# Esta é a maneira idiomática no Pico SDK para agrupar arquivos-fonte e seus
# requisitos de uso sem criar um arquivo de biblioteca estática intermediário.
add_library(paho.mqtt.embedded-c INTERFACE)

# Variáveis dos caminhos
SET(FREERTOS_PATH ${CMAKE_CURRENT_LIST_DIR}/FreeRTOS)
set(PAHO_MQTT_DIR ${CMAKE_CURRENT_LIST_DIR}/lib/paho.mqtt.embedded-c)
set(TFLM_CORE_SRCS
    ${CMAKE_CURRENT_LIST_DIR}/edge-impulse-sdk/tensorflow/lite/micro/micro_context.cc
    ${CMAKE_CURRENT_LIST_DIR}/edge-impulse-sdk/tensorflow/lite/micro/micro_utils.cc
)
set(TFLM_KERNEL_SRCS
    ${CMAKE_CURRENT_LIST_DIR}/edge-impulse-sdk/tensorflow/lite/micro/kernels/fully_connected.cc
    ${CMAKE_CURRENT_LIST_DIR}/edge-impulse-sdk/tensorflow/lite/micro/kernels/softmax.cc 
)

# Suprimir warnings comuns
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wno-macro-redefined")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-macro-redefined")

# -----------------------------------------------------------------------------
# Integração da Biblioteca do Edge Impulse (TinyML)
# -----------------------------------------------------------------------------

# Adiciona os arquivos fonte da biblioteca do Edge Impulse ao nosso executável.
# O CMake é inteligente e compilará os arquivos .cpp com o compilador C++.
file(GLOB_RECURSE EDGE_IMPULSE_SOURCES
    "edge-impulse-sdk/*.cpp"
    "tflite-model/*.cpp"
)
# Biblioteca Estática para o Edge Impulse SDK (C++)
# Esta é a abordagem moderna e correta. Em vez de adicionar todos os ficheiros
# e includes ao executável, encapsulamo-los numa biblioteca.
# Cria biblioteca STATIC para encapsular todo o SDK do Edge Impulse
add_library(edge-impulse-sdk STATIC ${EDGE_IMPULSE_SOURCES})

# Adiciona as pastas da biblioteca do Edge Impulse ao path de includes
# para que o compilador possa encontrar os arquivos .h
include_directories(
    ${CMAKE_CURRENT_LIST_DIR}
    ${CMAKE_CURRENT_LIST_DIR}/FreeRTOS/include
    ${CMAKE_CURRENT_LIST_DIR}/FreeRTOS/portable/GCC/ARM_CM0
    ${CMAKE_CURRENT_LIST_DIR}/libs
    ${CMAKE_CURRENT_LIST_DIR}/libs/pico-ssd1306
    ${CMAKE_CURRENT_LIST_DIR}/model-parameters
    ${CMAKE_CURRENT_LIST_DIR}/edge-impulse-sdk
    ${CMAKE_CURRENT_LIST_DIR}/edge-impulse-sdk/tensorflow
    ${CMAKE_CURRENT_LIST_DIR}/edge-impulse-sdk/tensorflow/lite/micro
    ${CMAKE_CURRENT_LIST_DIR}/edge-impulse-sdk/classifier
    ${CMAKE_CURRENT_LIST_DIR}/edge-impulse-sdk/dsp
    ${CMAKE_CURRENT_LIST_DIR}/edge-impulse-sdk/porting
    ${CMAKE_CURRENT_LIST_DIR}/tflite-model

)

target_sources(edge-impulse-sdk PRIVATE
    ${TFLM_CORE_SRCS}
    ${TFLM_KERNEL_SRCS}
)

# Adiciona os arquivos-fonte à biblioteca INTERFACE.
# Eles serão compilados diretamente no executável final que linca esta biblioteca.
target_sources(paho.mqtt.embedded-c INTERFACE
    # MQTT Client
    ${CMAKE_CURRENT_LIST_DIR}/lib/paho.mqtt.embedded-c/MQTTClient-C/src/MQTTClient.c
    ${CMAKE_CURRENT_LIST_DIR}/lib/paho.mqtt.embedded-c/MQTTClient-C/src/MQTTConnectClient.c
    ${CMAKE_CURRENT_LIST_DIR}/lib/paho.mqtt.embedded-c/MQTTClient-C/src/MQTTSubscribeClient.c
    ${CMAKE_CURRENT_LIST_DIR}/lib/paho.mqtt.embedded-c/MQTTClient-C/src/MQTTUnsubscribeClient.c
    ${CMAKE_CURRENT_LIST_DIR}/lib/paho.mqtt.embedded-c/MQTTClient-C/src/MQTTSerializePublish.c
    ${CMAKE_CURRENT_LIST_DIR}/lib/paho.mqtt.embedded-c/MQTTClient-C/src/MQTTDeserializePublish.c

    # MQTT Packet
    ${CMAKE_CURRENT_LIST_DIR}/lib/paho.mqtt.embedded-c/MQTTPacket/src/MQTTPacket.c
    ${CMAKE_CURRENT_LIST_DIR}/lib/paho.mqtt.embedded-c/MQTTPacket/src/MQTTConnectServer.c
    ${CMAKE_CURRENT_LIST_DIR}/lib/paho.mqtt.embedded-c/MQTTPacket/src/MQTTFormat.c
    ${CMAKE_CURRENT_LIST_DIR}/lib/paho.mqtt.embedded-c/MQTTPacket/src/MQTTSubscribeServer.c
    ${CMAKE_CURRENT_LIST_DIR}/lib/paho.mqtt.embedded-c/MQTTPacket/src/MQTTUnsubscribeServer.c
)
# Adiciona os diretórios de inclusão necessários para a biblioteca Paho MQTT.
# Eles são marcados como INTERFACE para que qualquer alvo que lincar com paho.mqtt.embedded-c
# (como nosso executável principal) também tenha esses caminhos adicionados ao seu caminho de busca de inclusão.
target_include_directories(paho.mqtt.embedded-c INTERFACE
    ${CMAKE_CURRENT_LIST_DIR}/lib/paho.mqtt.embedded-c/MQTTClient-C/src/
    ${CMAKE_CURRENT_LIST_DIR}/lib/paho.mqtt.embedded-c/MQTTPacket/src/
)
# Suprimir warnings da biblioteca MQTT
#target_compile_options(tinyml_gate PRIVATE
    #-Wno-address-of-packed-member
    #-Wno-stringop-overflow
    #-Wno-macro-redefined

set_source_files_properties(
    lib/paho.mqtt.embedded-c/MQTTClient-C/src/MQTTClient.c
    PROPERTIES COMPILE_FLAGS "-Wno-address-of-packed-member -Wno-stringop-overflow"    
)
# Executável principal
add_executable(${PROJECT_NAME}
# Inclui o new_delete.cpp customizado ANTES dos sources do Edge Impulse
    ${CUSTOM_NEW_DELETE}
    # Inclui os fontes do Edge Impulse na compilação
    #${EDGE_IMPULSE_SOURCES}

    main.c
    lib/ssd1306_i2c.c
    src/button_a.c
    src/button_b.c
    src/button_j.c
    src/ctrl.c
    src/display_gate.c
    src/led_rgb.c
    src/oled/display.c
    src/oled/i2c.c
    src/util.c
    src/mpu6050.c
    #src/new_delete.cpp    # Gerenciador de memória C++ thread-safe
    
    # Camadas de abstração de rede e timer do Paho
    src/paho_network.c
    src/paho_timer.c
    src/mqtt_task.c    # O arquivo corrigido

# Tarefa de Machine Learning (C++) agora está incluída
    src/ml.cpp   # Tarefa de Machine Learning (C++)
    src/ei_porting.cpp    # Implementação da camada de portabilidade do EI

    # Lista dos arquivos fontes do FreeRTOS a serem compilados
    # Fontes do FreeRTOS
    FreeRTOS/event_groups.c
    FreeRTOS/list.c
    FreeRTOS/croutine.c
    FreeRTOS/queue.c
    FreeRTOS/stream_buffer.c
    FreeRTOS/tasks.c
    FreeRTOS/timers.c
    FreeRTOS/freertos_hooks.c
    FreeRTOS/portable/MemMang/heap_4.c
    FreeRTOS/portable/GCC/ARM_CM0/port.c
    FreeRTOS/portable/GCC/ARM_CM0/portasm.c
)
set_target_properties(tinyml_gate PROPERTIES SUFFIX .elf)

# Define as flags do compilador necessárias - APÓS criar o executável
target_compile_definitions(tinyml_gate PRIVATE
    CMSIS_NN=1
    TF_LITE_STATIC_MEMORY=1
)

# Adiciona os diretórios de inclusão NECESSÁRIOS à biblioteca.
# A palavra-chave PUBLIC garante que qualquer alvo que vincule a esta biblioteca
# herdará automaticamente esses caminhos de inclusão.
target_include_directories(edge-impulse-sdk PUBLIC
    ${CMAKE_CURRENT_LIST_DIR}
)

# Adiciona diretórios de inclusão específicos do projeto.
# NOTA: Todos os caminhos de inclusão manuais apontando para dentro do Pico SDK foram removidos.
# Eles não são mais necessários porque a lincagem com as bibliotecas corretas abaixo
# cuidará disso de forma automática e robusta.
target_include_directories(${PROJECT_NAME} PRIVATE
    #${CMAKE_CURRENT_LIST_DIR}
    ${CMAKE_CURRENT_LIST_DIR}/include
    ${CMAKE_CURRENT_LIST_DIR}/src
    ${CMAKE_CURRENT_LIST_DIR}/edge-impulse-sdk
    ${CMAKE_CURRENT_LIST_DIR}/edge-impulse-sdk/porting
    ${CMAKE_CURRENT_LIST_DIR}/edge-impulse-sdk/tensorflow
)    
target_include_directories(${PROJECT_NAME} PRIVATE
    ${CMAKE_CURRENT_LIST_DIR}/edge-impulse-sdk/third_party/cmsis/CMSIS/DSP/Include
    ${CMAKE_CURRENT_LIST_DIR}/edge-impulse-sdk/third_party/cmsis/CMSIS/NN/Include
    ${CMAKE_CURRENT_LIST_DIR}/edge-impulse-sdk/third_party/cmsis/CMSIS/Core/Include

    # FreeRTOS includes
    ${CMAKE_CURRENT_LIST_DIR}/FreeRTOS/include
    ${CMAKE_CURRENT_LIST_DIR}/FreeRTOS/portable/GCC/ARM_CM0

    # CORREÇÃO: Adicionar includes específicos do CYW43/WiFi
    ${PICO_SDK_PATH}/src/rp2_common/pico_cyw43_arch/include
    ${PICO_SDK_PATH}/src/rp2_common/pico_cyw43_driver/include
    ${PICO_SDK_PATH}/lib/cyw43-driver/src
    ${PICO_SDK_PATH}/src/rp2_common/pico_lwip/include
    ${PICO_SDK_PATH}/lib/lwip/src/include
    ${PICO_SDK_PATH}/lib/lwip/contrib/ports/freertos/include
    ${PICO_SDK_PATH}/src/common/pico_async_context/include
)
   # Linca todas as bibliotecas necessárias ao executável principal.
# A ordem geralmente é: bibliotecas do projeto, depois bibliotecas de hardware, depois bibliotecas de alto nível do SDK.
target_link_libraries(${PROJECT_NAME} PRIVATE
    # Biblioteca específica do projeto
    paho.mqtt.embedded-c
    edge-impulse-sdk

    # Bibliotecas de hardware padrão do Pico
    pico_stdlib
    hardware_gpio
    hardware_i2c
    hardware_spi
    hardware_adc
    hardware_pwm
    hardware_watchdog
  
    # Bibliotecas de alto nível do SDK para FreeRTOS e Rede
    # Estas automaticamente incluem todas as dependências transitivas necessárias
    # (kernel do FreeRTOS, lwIP, driver CYW43, etc.).
    freertos_config
    pico_cyw43_arch_sys_freertos
    pico_lwip_freertos
)

# Configurações específicas para C++
target_compile_options(edge-impulse-sdk PRIVATE
    $<$<COMPILE_LANGUAGE:CXX>:-fno-exceptions>
    $<$<COMPILE_LANGUAGE:CXX>:-fno-rtti>
    $<$<COMPILE_LANGUAGE:CXX>:-fno-use-cxa-atexit>
)

# Definições para desabilitar recursos C++ não suportados
target_compile_definitions(edge-impulse-sdk PUBLIC
    PICO_CXX_ENABLE_EXCEPTIONS=0
    #EI_PORTING_REALTEKAMBD=1
    #EI_PORTING_RTTHREAD=1
    EI_DSP_USE_MATRIX_OPS=0
    EI_PORTING_GENERIC_CPLUSPLUS=1
    EI_CLASSIFIER_TFLITE_ENABLE_CMSIS_NN=1
    EI_DSP_USE_CMSIS_DSP=0 # Cortex-M0+ não possui instruções DSP
    # Desativa o pico_cxx_options para usar o new_delete personalizado
    #LIB_PICO_CXX_OPTIONS=0
    PICO_CXX_DISABLE_ALLOCATION_OVERRIDES=1
)

# Habilita stdio sobre USB e/ou UART
pico_enable_stdio_usb(${PROJECT_NAME} 1)
pico_enable_stdio_uart(${PROJECT_NAME} 0)

# Gera o arquivo.uf2 e outros formatos de saída
pico_add_extra_outputs(${PROJECT_NAME})