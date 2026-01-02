"""
Configurações Globais do Projeto.
Centraliza UUIDs, hardware, visão e definições de protocolo.
"""

# --- CONFIGURAÇÕES BLE (Bluetooth Low Energy) ---
DEVICE_NAME = "BitDogLab_UCR"
UUID_DIRECTION_CHARACTERISTIC = "0000FF12-0000-1000-8000-00805F9B34FB"
UUID_COLOR_CHARACTERISTIC = "0000FF11-0000-1000-8000-00805F9B34FB"

# --- DEFINIÇÕES DE COMANDO (Protocolo) ---
CMD_PARE = 0x00
CMD_RETO = 0x01
CMD_ESQUERDA = 0x02
CMD_DIREITA = 0x03

# --- DEFINIÇÕES DE CORES (Protocolo) ---
COR_VERMELHO = 0x01
COR_VERDE = 0x02
COR_AZUL = 0x03

COLOR_NAMES = {COR_VERMELHO: "VERMELHO", COR_VERDE: "VERDE", COR_AZUL: "AZUL"}

# --- CONFIGURAÇÕES DE VISÃO ---
WIDTH = 320
HEIGHT = 240

"""
COLOR_MAP = {
    COR_VERMELHO: {"lower": [0, 100, 100], "upper": [10, 255, 255]},
    COR_VERDE: {"lower": [35, 100, 100], "upper": [85, 255, 255]},
    COR_AZUL: {"lower": [100, 100, 100], "upper": [130, 255, 255]},
}
"""

COLOR_MAP = {
    COR_VERMELHO: {"lower": [0, 120, 70], "upper": [10, 255, 255]},
    COR_VERDE: {"lower": [35, 100, 50], "upper": [85, 255, 255]},
    COR_AZUL: {"lower": [100, 150, 50], "upper": [130, 255, 255]},
}
