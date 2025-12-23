import asyncio  # Biblioteca para programação assíncrona (gerencia tarefas em paralelo)

from bleak import BleakClient  # Classe principal para agir como cliente GATT

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
        """Função chamada automaticamente quando a Pico envia uma nova cor."""
        # Decodifica o byte recebido (0x01, 0x02...) para um inteiro
        color_id = decode_color_id(data)
        # Coloca o ID da cor na fila para que a thread de visão o veja
        self.color_queue.put(color_id)
        print(f"[BLE] Nova cor recebida da Pico: ID {color_id}")

    async def run(self):
        """Loop principal de execução do Cliente GATT."""
        print(f"[BLE] Tentando conectar ao endereço {utils.config.PICO_ADDRESS}...")

        # O bloco 'async with' garante que a conexão feche corretamente ao sair
        async with BleakClient(utils.config.PICO_ADDRESS) as client:
            self.client = client
            print(f"[BLE] Conectado com sucesso à Pi Pico!")

            # Ativa as notificações para a característica de Cor
            # A Pico "avisará" a Pi Zero sempre que a cor mudar
            await client.start_notify(
                utils.config.UUID_COLOR_CHARACTERISTIC, self.notification_callback
            )

            # Loop infinito de escrita de comandos de direção
            while True:
                # Verifica se há um novo cálculo de erro na fila da visão
                if not self.error_queue.empty():
                    # Pega o valor do erro (ex: -45)
                    error_value = self.error_queue.get()

                    # Converte o inteiro para 2 bytes (binário) usando nosso protocolo
                    payload = encode_error(error_value)

                    try:
                        # Envia o erro para a característica de direção da Pico
                        # 'response=False' torna o envio mais rápido (sem confirmação)
                        await client.write_gatt_char(
                            utils.config.UUID_DIRECTION_CHARACTERISTIC,
                            payload,
                            response=False,
                        )
                    except Exception as e:
                        print(f"[BLE] Erro ao enviar comando: {e}")

                # Pausa mínima para permitir que outras tarefas assíncronas rodem
                # Sem este sleep, o loop travaria o processador em 100%
                await asyncio.sleep(0.01)  # Frequência de 100Hz

    async def start(self):
        """Inicia o serviço com lógica de reconexão simples."""
        while True:
            try:
                await self.run()
            except Exception as e:
                print(f"[BLE] Conexão perdida ou falhou: {e}. Tentando em 5s...")
                await asyncio.sleep(5)
