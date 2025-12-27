import queue

import cv2
import numpy as np
from picamera2 import Picamera2  # Importação simplificada

import utils.config


class ColorFollower:
    """Classe responsável pelo processamento de imagem utilizando Picamera2."""

    def __init__(self, color_queue, error_queue):
        # Filas de comunicação
        self.color_queue = color_queue
        self.error_queue = error_queue

        # Configuração inicial de cor
        self.current_id = 1
        self.lower_hsv, self.upper_hsv = self._update_color_limits(self.current_id)

        # Inicializa a Picamera2
        self.picam2 = Picamera2()

        # Configura a câmera para o formato RGB compatível com processamento
        self.config = self.picam2.create_preview_configuration(
            main={"format": "RGB888", "size": (utils.config.WIDTH, utils.config.HEIGHT)}
        )
        self.picam2.configure(self.config)

    def _update_color_limits(self, color_id):
        """Busca os limites HSV no dicionário de configuração."""
        ranges = utils.config.COLOR_MAP.get(color_id, utils.config.COLOR_MAP[1])
        return np.array(ranges["lower"]), np.array(ranges["upper"])

    def run(self):
        """Loop principal de captura utilizando a API da Picamera2."""

        # Inicia a câmera
        self.picam2.start()
        print("[VISÃO] Picamera2 iniciada com sucesso. Processamento ativo...")

        center_reference = utils.config.WIDTH // 2

        try:
            while True:
                # 1. Verifica se há novo ID de cor na fila
                try:
                    new_id = self.color_queue.get_nowait()
                    if new_id != self.current_id:
                        self.current_id = new_id
                        self.lower_hsv, self.upper_hsv = self._update_color_limits(
                            new_id
                        )
                        print(f"[VISÃO] Mudando alvo para Cor ID: {new_id}")
                except queue.Empty:
                    pass

                # 2. Captura o frame como um array numpy (RGB)
                # O Picamera2 gerencia o buffer internamente
                frame = self.picam2.capture_array()

                if frame is None:
                    continue

                # 3. Processamento de imagem
                # Converte RGB (Picamera2) para HSV para segmentação
                hsv = cv2.cvtColor(frame, cv2.COLOR_RGB2HSV)
                mask = cv2.inRange(hsv, self.lower_hsv, self.upper_hsv)

                # Limpeza morfológica
                mask = cv2.erode(mask, None, iterations=2)
                mask = cv2.dilate(mask, None, iterations=2)

                # 4. Detecção de contornos e cálculo do erro
                contours, _ = cv2.findContours(
                    mask, cv2.RETR_EXTERNAL, cv2.CHAIN_APPROX_SIMPLE
                )

                if contours:
                    largest_contour = max(contours, key=cv2.contourArea)

                    if cv2.contourArea(largest_contour) > 500:
                        M = cv2.moments(largest_contour)
                        if M["m00"] != 0:
                            cx = int(M["m10"] / M["m00"])
                            error = cx - center_reference

                            # Envia o erro para a fila do BLE
                            if self.error_queue.full():
                                try:
                                    self.error_queue.get_nowait()
                                except queue.Empty:
                                    pass
                            self.error_queue.put(error)

        except Exception as e:
            print(f"[VISÃO] Erro durante o processamento: {e}")
        finally:
            # Garante que a câmera seja liberada corretamente
            self.picam2.stop()
            self.picam2.close()
            print("[VISÃO] Recursos da câmera liberados.")
