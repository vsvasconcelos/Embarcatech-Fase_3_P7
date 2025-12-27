import asyncio

from bleak import BleakClient, BleakScanner

import utils.config  # Arquivo central de configurações (UUIDs e Endereços)
from utils.protocol import decode_color_id, encode_error  # Funções de conversão binária


class BLEController:
    """Classe responsável por gerenciar a comunicação BLE com a Pi Pico W."""

    def __init__(self, error_queue, color_queue):
        # Fila para ler os erros gerados pela visão computacional
        self.error_queue = error_queue
        # Fila para enviar a cor recebida para a visão computacional
        self.color_queue = color_queue
        # Variável para armazenar o objeto do cliente BLE
        self.client = None

    def notification_callback(self, sender, data):
        color_id = decode_color_id(data)

        # Busca o nome usando a constante centralizada no config
        nome_cor = utils.config.COLOR_NAMES.get(color_id, "DESCONHECIDO")

        self.color_queue.put(color_id)
        print(f"[BLE] Nova cor recebida da Pico: ID {color_id} ({nome_cor})")

    async def run(self):
        """
        Tenta encontrar e conectar ao dispositivo.
        Retorna True se conectado com sucesso, False caso contrário.
        """
        print(f"[BLE] Buscando pelo dispositivo: {utils.config.DEVICE_NAME}...")

        try:
            # Escaneamento dinâmico para encontrar o endereço MAC pelo nome
            # Isso resolve problemas de cache do Bluetooth no Raspberry Pi
            devices = await BleakScanner.discover(timeout=10.0)
            alvo = next(
                (d for d in devices if d.name == utils.config.DEVICE_NAME), None
            )

            if not alvo:
                print(f"[BLE] Dispositivo '{utils.config.DEVICE_NAME}' não encontrado.")
                return False

            print(f"[BLE] Dispositivo encontrado: {alvo.address}. Conectando...")
            self.client = BleakClient(alvo.address)
            await self.client.connect()

            if self.client.is_connected:
                # Ativa notificações para receber trocas de cor da Pico
                await self.client.start_notify(
                    utils.config.UUID_COLOR_CHARACTERISTIC, self.notification_callback
                )
                return True

        except Exception as e:
            print(f"[BLE] Erro durante a tentativa de conexão: {e}")

        return False

    async def start_communication_loop(self):
        """
        Mantém a conexão ativa e envia os erros da fila para a Pico.
        Este método deve ser chamado após o run() retornar True.
        """
        if not self.client or not self.client.is_connected:
            print("[BLE] Erro: Tentativa de iniciar loop sem conexão ativa.")
            return

        print("[BLE] Loop de comunicação iniciado com sucesso.")

        try:
            while self.client.is_connected:
                # Verifica se a visão colocou um novo erro na fila
                if not self.error_queue.empty():
                    error_value = self.error_queue.get()
                    # Payload será b'\x00', b'\x01', b'\x02' ou b'\x03'
                    payload = encode_error(error_value)

                    try:
                        # Envia para a característica de direção na Pico
                        await self.client.write_gatt_char(
                            utils.config.UUID_DIRECTION_CHARACTERISTIC,
                            payload,
                            response=False,
                        )
                    except Exception as e:
                        print(f"[BLE] Erro ao enviar comando: {e}")

                # Pausa essencial para não sobrecarregar a CPU do Pi Zero
                await asyncio.sleep(0.01)

        except Exception as e:
            print(f"[BLE] Conexão perdida no loop de comunicação: {e}")
        finally:
            if self.client:
                await self.client.disconnect()
            print("[BLE] Cliente desconectado.")
