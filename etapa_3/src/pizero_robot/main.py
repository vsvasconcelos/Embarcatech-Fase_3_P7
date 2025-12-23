import asyncio  # Gerencia as tarefas assíncronas (BLE)
import queue  # Filas seguras para troca de dados entre Threads
import sys  # Para lidar com encerramento do script
import threading  # Gerencia a execução em paralelo (Visão)

from core import BLEController, ColorFollower  # Módulos configurados via __init__.py


def main():
    """Função principal que orquestra a integração do sistema."""

    print("--- Inicializando Robô Seguidor de Faixa (Pi Zero 2 W) ---")

    # Criação das Filas de Comunicação (Thread-Safe Queues)
    # A fila de erro tem tamanho 1 para garantir que o rádio envie sempre o dado mais atual
    error_q = queue.Queue(maxsize=1)
    # A fila de cor recebe as notificações de troca de cor da Pico
    color_q = queue.Queue()

    # Instanciação dos Módulos
    # O VisionThread lerá de color_q e escreverá em error_q
    vision_module = ColorFollower(color_queue=color_q, error_queue=error_q)
    # O BLEController lerá de error_q e escreverá em color_q
    ble_module = BLEController(error_queue=error_q, color_queue=color_q)

    # Configuração e Inicialização da Thread de Visão
    # Definimos como 'daemon=True' para que ela feche automaticamente se o main fechar
    vision_thread = threading.Thread(
        target=vision_module.run, name="VisionThread", daemon=True
    )

    try:
        # Inicia o processamento de imagem
        vision_thread.start()
        print("[SISTEMA] Thread de Visão iniciada.")

        # Inicia o Loop de Eventos do Asyncio (Módulo BLE)
        # O .start() do ble_module contém a lógica de reconexão automática
        print("[SISTEMA] Iniciando loop de eventos BLE...")
        asyncio.run(ble_module.start())

    except KeyboardInterrupt:
        # Captura o Ctrl+C para encerrar o robô graciosamente
        print("\n[SISTEMA] Encerrando aplicação pelo usuário...")
        sys.exit(0)
    except Exception as e:
        print(f"[SISTEMA] Erro crítico na execução: {e}")
        sys.exit(1)


if __name__ == "__main__":
    main()
