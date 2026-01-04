import asyncio
import os
import queue
import sys
import threading

from core import BLEController, ColorFollower
from utils import streamer


def pin_thread_to_core(thread_ident: int, core_id: int):
    """Fixa uma thread (por seu identificador) a um núcleo de CPU específico."""
    try:
        os.sched_setaffinity(thread_ident, {core_id})
        print(f"[Afinidade] Thread ID {thread_ident} fixada no core {core_id}")
    except (AttributeError, OSError) as e:
        print(f"[Afinidade] Erro ao fixar thread {thread_ident}: {e}")


def main():
    print("--- Inicializando Robô Seguidor de Faixa (Pi Zero 2 W) ---")

    error_q = queue.Queue(maxsize=1)
    color_q = queue.Queue()

    vision_module = ColorFollower(color_queue=color_q, error_queue=error_q)
    ble_module = BLEController(error_queue=error_q, color_queue=color_q)

    vision_thread = threading.Thread(
        target=vision_module.run, name="VisionThread", daemon=True
    )

    flask_thread = threading.Thread(
        target=streamer.start_server, name="FlaskThread", daemon=True
    )
    flask_thread.start()

    # Aplica afinidade para a thread do Flask (após iniciar)
    if flask_thread.native_id:
        pin_thread_to_core(flask_thread.native_id, 2)

    async def inicializar_sistema():
        print("[SISTEMA] Aguardando conexão BLE antes de iniciar Visão...")
        sucesso_ble = await ble_module.run()

        if sucesso_ble:
            print("[SISTEMA] BLE Conectado com sucesso!")
            print("[SISTEMA] Aguardando 4s para estabilizar hardware (Câmera/BT)...")
            await asyncio.sleep(4)

            # Inicia thread de visão e aplica afinidade
            print("[SISTEMA] Iniciando Thread de Visão...")
            vision_thread.start()

            if vision_thread.native_id:
                pin_thread_to_core(vision_thread.native_id, 1)

            # Aplica afinidade para a thread principal (que roda o asyncio)
            pin_thread_to_core(threading.current_thread().native_id, 0)

            await ble_module.start_communication_loop()
        else:
            print(
                "[ERRO] Não foi possível encontrar a BitDogLab. Verifique o dispositivo."
            )

    try:
        asyncio.run(inicializar_sistema())
    except KeyboardInterrupt:
        print("\n[SISTEMA] Encerrando aplicação pelo usuário (Ctrl+C)...")
        sys.exit(0)
    except Exception as e:
        print(f"[SISTEMA] Erro inesperado: {e}")
        sys.exit(1)


if __name__ == "__main__":
    main()
