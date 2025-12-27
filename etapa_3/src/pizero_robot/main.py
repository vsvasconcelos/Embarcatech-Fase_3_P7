import asyncio  # Gerencia as tarefas assíncronas (BLE)
import queue  # Filas seguras para troca de dados entre Threads
import sys  # Para lidar com encerramento do script
import threading  # Gerencia a execução em paralelo (Visão)
import time

# Importação dos módulos do projeto
from core import BLEController, ColorFollower  # Módulos configurados via __init__.py


def main():
    """Função principal que orquestra a integração do sistema."""

    print("--- Inicializando Robô Seguidor de Faixa (Pi Zero 2 W) ---")

    # 1. Criação das Filas de Comunicação (Thread-Safe Queues)
    # Fila de erro: Tamanho 1 para enviar sempre o cálculo mais recente
    error_q = queue.Queue(maxsize=1)
    # Fila de cor: Recebe as notificações de troca de cor da Pico
    color_q = queue.Queue()

    # 2. Instanciação dos Módulos
    vision_module = ColorFollower(color_queue=color_q, error_queue=error_q)
    ble_module = BLEController(error_queue=error_q, color_queue=color_q)

    # 3. Configuração da Thread de Visão
    # daemon=True garante que a thread morra se o processo principal encerrar
    vision_thread = threading.Thread(
        target=vision_module.run, name="VisionThread", daemon=True
    )

    async def inicializar_sistema():
        """
        Função interna assíncrona para garantir que a visão
        só inicie após o Bluetooth estar estável.
        """
        print("[SISTEMA] Aguardando conexão BLE antes de iniciar Visão...")

        # Tenta conectar (chama o run() do ble_client.py)
        sucesso_ble = await ble_module.run()

        if sucesso_ble:
            print("[SISTEMA] BLE Conectado com sucesso!")
            print("[SISTEMA] Aguardando 4s para estabilizar hardware (Câmera/BT)...")

            # Pausa crucial para o Raspberry Pi Zero 2 W não sobrecarregar o barramento
            await asyncio.sleep(4)

            # Agora iniciamos a thread de visão
            print("[SISTEMA] Iniciando Thread de Visão...")
            vision_thread.start()

            # Mantém o programa "preso" aqui processando a comunicação BLE
            # Isso evita o erro 'terminate called without an active exception'
            await ble_module.start_communication_loop()
        else:
            print(
                "[ERRO] Não foi possível encontrar a BitDogLab. Verifique o dispositivo."
            )

    # 4. Execução do Loop Principal
    try:
        # Inicia o loop de eventos assíncrono
        asyncio.run(inicializar_sistema())
    except KeyboardInterrupt:
        print("\n[SISTEMA] Encerrando aplicação pelo usuário (Ctrl+C)...")
        sys.exit(0)
    except Exception as e:
        print(f"[SISTEMA] Erro inesperado: {e}")
        sys.exit(1)


if __name__ == "__main__":
    main()
