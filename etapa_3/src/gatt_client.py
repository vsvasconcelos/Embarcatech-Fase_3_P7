# framework padrão do Python para escrever código concorrente usando a sintaxe async/await
# Permite operações assíncronas, essenciais para comunicação BLE
import asyncio

# Biblioteca Bleak para operações BLE
# BleakClient: usada para conectar e interagir com dispositivos BLE
# BleakScanner: usada para escanear dispositivos BLE próximos
from bleak import BleakClient, BleakScanner

# --- CONFIGURAÇÃO DOS UUIDs (DEVEM BATER COM O SERVER.C na Pi Pico W - robô móvel) ---
# UUID do Serviço (Custom Service)
# UUIDs (Universally Unique Identifiers): No contexto BLE GATT, UUIDs são identificadores
# de 128 bits (16 bytes) usados para identificar serviços e características. Eles garantem
# que cada serviço e característica seja globalmente únicos, evitando conflitos quando
# múltiplos dispositivos BLE estão presentes

# SERVICE_UUID: Identifica o serviço personalizado que a Pico W (o servidor GATT) está oferecendo.
# Um serviço é uma coleção de características relacionadas.
SERVICE_UUID = "0000FF10-0000-1000-8000-00805F9B34FB"

# UUID da Característica de Cor (Notificação - Leitura)
# CHAR_COLOR_UUID: Identifica uma característica específica dentro do serviço.
# Esta característica é usada para a Pico W notificar o cliente sobre mudanças na cor.
# Notificação significa que o servidor envia dados para o cliente sem que o cliente precise explicitamente
# solicitá-los. Para que isso funcione, o cliente precisa se "inscrever" (subscribe) nas notificações
CHAR_COLOR_UUID = "0000FF11-0000-1000-8000-00805F9B34FB"

# UUID da Característica de Comando (Escrita)
# CHAR_CMD_UUID: Identifica outra característica para o cliente escrever comandos para a Pico W.
# `Escrita` permite que o cliente envie dados para o servidor, que pode então agir com base nesses dados
# (neste caso, controlar um robô).
CHAR_CMD_UUID = "0000FF12-0000-1000-8000-00805F9B34FB"

# Nome do dispositivo definido no pacote de anúncio do server.c
# DEVICE_NAME: Define o nome esperado do dispositivo BLE que procuramos. Este nome é transmitido pelo servidor BLE
# em seus pacotes de anúncio. É crucial que este nome corresponda ao que está configurado no `server.c` da Pico W.
DEVICE_NAME = "BitDogLab_UCR"
# DEVICE_NAME = "Pico"

# Mapas para tradução dos códigos (Hex -> Texto)
# Dicionário Python que mapeia os valores numéricos (recebidos da Pico W) para nomes de cores.
COLOR_MAP = {1: "VERMELHO", 2: "VERDE", 3: "AZUL"}


# --- CALLBACK (passada como argumento para outra função quando um evento específico ocorre) DE NOTIFICAÇÃO ---
# Esta função é chamada automaticamente sempre que a Pico W envia um dado
# bleak chamará notification_handler toda vez que a Pico W enviar uma notificação para a característica CHAR_COLOR_UUID
# sender: o objeto que enviou a notificação (neste caso, a característica CHAR_COLOR_UUID)
# data: Os dados reais enviados na notificação, que vêm como um array de bytes.
def notification_handler(sender, data):
    # Converte o array de bytes para um número inteiro, Especifica a ordem dos bytes.
    # "Little-endian" significa que o byte menos significativo (o que representa o menor valor) vem primeiro.
    val = int.from_bytes(data, byteorder="little")
    # Tenta obter o nome da cor do COLOR_MAP usando o valor inteiro. Se o valor não estiver no mapa, ele retorna uma
    # string indicando que é um valor desconhecido, juntamente com o valor bruto
    color_name = COLOR_MAP.get(val, f"DESCONHECIDO ({val})")
    print(f"\n[NOTIFICACAO RECEBIDA] A Pico W mudou a cor para: {color_name}")
    print("Digite um comando ( r | p | e | d | s ): ", end="", flush=True)


# --- FUNÇÃO PRINCIPAL ---
async def main():
    # Esta é a função principal que orquestra todas as operações do cliente BLE.
    print("--- INICIANDO CLIENTE GATT NA PI ZERO 2W ---")
    print(f"Procurando por dispositivo chamado '{DEVICE_NAME}'...")

    # 1. SCAN: Procura o dispositivo pelo nome
    device = await BleakScanner.find_device_by_filter(
        lambda d, ad: d.name and d.name == DEVICE_NAME
    )

    if not device:
        print(f"Dispositivo '{DEVICE_NAME}' não encontrado.")
        return

    print(f"Encontrado! Conectando a {device.address}...")

    # 2. CONEXÃO
    async with BleakClient(device) as client:
        print(f"Conectado: {client.is_connected}")

        # 3. INSCRIÇÃO (SUBSCRIBE) NA CARACTERÍSTICA DE COR (FF11)
        # Isso equivale a clicar nas "múltiplas setas" no nRF Connect
        await client.start_notify(CHAR_COLOR_UUID, notification_handler)
        print("Inscrito nas notificações de COR. Aguardando dados...")

        # Loop de interação com o usuário (Para enviar comandos na FF12)
        print("\n--- CONTROLE DO ROBÔ ---")
        print("r: Reto | p: Parar | e: Esquerda | d: Direita | s: Sair")

        while True:
            # Usa run_in_executor para não bloquear o loop de notificação enquanto espera o input
            cmd_input = await asyncio.get_event_loop().run_in_executor(
                None, input, "Digite um comando: "
            )

            command_byte = None

            if cmd_input.lower() == "s":
                break
            elif cmd_input.lower() == "p":  # Parar (0x00)
                command_byte = b"\x00"
            elif cmd_input.lower() == "r":  # Reto (0x01)
                command_byte = b"\x01"
            elif cmd_input.lower() == "e":  # Esquerda (0x02)
                command_byte = b"\x02"
            elif cmd_input.lower() == "d":  # Direita (0x03)
                command_byte = b"\x03"

            # 4. ESCRITA (WRITE) NA CARACTERÍSTICA DE COMANDO (FF12)
            if command_byte:
                print(f"Enviando comando: {command_byte.hex()}...")
                await client.write_gatt_char(CHAR_CMD_UUID, command_byte, response=True)
            else:
                print("Comando inválido.")

        # Encerra
        await client.stop_notify(CHAR_COLOR_UUID)
        print("Desconectando...")


# Executa o loop assíncrono
if __name__ == "__main__":
    try:
        asyncio.run(main())
    except KeyboardInterrupt:
        print("\nEncerrado pelo usuário.")
    except Exception as e:
        print(f"Ocorreu um erro: {e}")
