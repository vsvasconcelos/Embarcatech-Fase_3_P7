"""
Configurações Globais do Projeto.
Centraliza UUIDs, endereços de hardware, parâmetros de imagem e limites HSV.
"""

# --- CONFIGURAÇÕES BLE (Bluetooth Low Energy) ---
# Endereço MAC da Pi Pico W (Obtido ao rodar o server na Pico)
PICO_ADDRESS = "XX:XX:XX:XX:XX:XX"

# UUIDs das Características (Devem ser idênticos aos do Servidor GATT na Pico)
# UUID para enviar o erro do centroide (Escrita)
UUID_DIRECTION_CHARACTERISTIC = "0000FF12-0000-1000-8000-00805F9B34FB"
# UUID para receber a notificação de cor (Leitura/Notificação)
UUID_COLOR_CHARACTERISTIC = "0000FF11-0000-1000-8000-00805F9B34FB"

# --- CONFIGURAÇÕES DE VISÃO COMPUTACIONAL ---
# Resolução reduzida para garantir alta performance na Pi Zero 2 W
WIDTH = 320
HEIGHT = 240

# --- MAPA DE CORES (Dicionário HSV) ---
# Estrutura: ID_DA_COR: {'lower': [H, S, V], 'upper': [H, S, V]}
# Baseado na Abordagem Clássica do projeto
COLOR_MAP = {
    1: {  # Vermelho
        "lower": [0, 100, 100],
        "upper": [10, 255, 255],
    },
    2: {  # Verde
        "lower": [40, 50, 50],
        "upper": [80, 255, 255],
    },
    3: {  # Azul
        "lower": [100, 150, 0],
        "upper": [140, 255, 255],
    },
}

# --- PARÂMETROS DE CONTROLE ---
# Área mínima em pixels para considerar um objeto como "faixa" (filtro de ruído)
MIN_AREA_THRESHOLD = 500
