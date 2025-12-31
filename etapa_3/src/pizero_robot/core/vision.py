import queue
import time

import cv2
import numpy as np
from picamera2 import Picamera2

import utils.config
from utils import streamer


class ColorFollower:
    """Classe responsável pelo processamento de imagem, debug e telemetria."""

    def __init__(self, color_queue, error_queue):
        self.color_queue = color_queue
        self.error_queue = error_queue
        self.current_id = 1
        self.lower_hsv, self.upper_hsv = self._update_color_limits(self.current_id)

        # Inicializa Picamera2 com configuração RGB
        self.picam2 = Picamera2()
        self.config = self.picam2.create_preview_configuration(
            main={"format": "RGB888", "size": (utils.config.WIDTH, utils.config.HEIGHT)}
        )
        self.picam2.configure(self.config)

        # Variáveis de telemetria
        self.fps = 0
        self.last_time = time.time()
        self.current_error = 0

    def _update_color_limits(self, color_id):
        """Busca os limites HSV no dicionário de configuração."""
        ranges = utils.config.COLOR_MAP.get(color_id, utils.config.COLOR_MAP[1])
        return np.array(ranges["lower"]), np.array(ranges["upper"])

    def run(self):
        """Loop principal com telemetria e tratamento de erros corrigido."""
        self.picam2.start()
        print("[VISÃO] Sistema de Telemetria e Debug Ativo.")
        center_reference = utils.config.WIDTH // 2

        try:
            while True:
                # 1. Cálculo de FPS
                now = time.time()
                dt = now - self.last_time
                self.last_time = now
                if dt > 0:
                    self.fps = 1 / dt

                # 2. Atualização de cor alvo via fila
                try:
                    new_id = self.color_queue.get_nowait()
                    if new_id != self.current_id:
                        self.current_id = new_id
                        self.lower_hsv, self.upper_hsv = self._update_color_limits(
                            new_id
                        )
                except queue.Empty:
                    pass

                # 3. Captura do frame
                frame_raw = self.picam2.capture_array()
                if frame_raw is None:
                    continue

                # Preparação do frame de debug (Conversão RGB -> BGR para visualização correta)
                # debug_frame = cv2.cvtColor(frame, cv2.COLOR_RGB2BGR)
                # Vermelho e Azul estavam invertidos
                frame = frame_raw[:, :, ::-1].copy()
                debug_frame = cv2.cvtColor(frame, cv2.COLOR_RGB2BGR)

                # 3. Restante do processamento de desenho (Telemetria)
                # A partir daqui, use o debug_frame para desenhar círculos e textos
                cv2.line(
                    debug_frame,
                    (center_reference, 0),
                    (center_reference, utils.config.HEIGHT),
                    (0, 255, 0),
                    1,
                )

                # 4. Processamento Lógico (Detecção HSV)
                hsv = cv2.cvtColor(frame, cv2.COLOR_RGB2HSV)
                mask = cv2.inRange(hsv, self.lower_hsv, self.upper_hsv)
                mask = cv2.erode(mask, None, iterations=2)
                mask = cv2.dilate(mask, None, iterations=2)

                contours, _ = cv2.findContours(
                    mask, cv2.RETR_EXTERNAL, cv2.CHAIN_APPROX_SIMPLE
                )

                self.current_error = 0

                if contours:
                    largest_contour = max(contours, key=cv2.contourArea)
                    if cv2.contourArea(largest_contour) > 500:
                        M = cv2.moments(largest_contour)
                        if M["m00"] != 0:
                            cx = int(M["m10"] / M["m00"])
                            cy = int(M["m01"] / M["m00"])
                            self.current_error = cx - center_reference

                            # Desenha marcação de detecção (Círculo Vermelho)
                            cv2.circle(debug_frame, (cx, cy), 10, (0, 0, 255), -1)

                            if self.error_queue.full():
                                try:
                                    self.error_queue.get_nowait()
                                except queue.Empty:
                                    pass
                            self.error_queue.put(self.current_error)

                # --- RENDERIZAÇÃO DA TELEMETRIA ---
                color_name = utils.config.COLOR_NAMES.get(self.current_id, "???")

                cv2.putText(
                    debug_frame,
                    f"FPS: {self.fps:.1f}",
                    (10, 20),
                    cv2.FONT_HERSHEY_SIMPLEX,
                    0.5,
                    (255, 255, 255),
                    1,
                )
                cv2.putText(
                    debug_frame,
                    f"Alvo: {color_name}",
                    (10, 40),
                    cv2.FONT_HERSHEY_SIMPLEX,
                    0.5,
                    (255, 255, 255),
                    1,
                )

                # Erro fica amarelo se estiver fora da zona morta
                err_color = (
                    (0, 255, 255) if abs(self.current_error) > 20 else (0, 255, 0)
                )
                cv2.putText(
                    debug_frame,
                    f"Erro: {self.current_error}px",
                    (10, 60),
                    cv2.FONT_HERSHEY_SIMPLEX,
                    0.5,
                    err_color,
                    1,
                )

                cv2.line(
                    debug_frame,
                    (center_reference, 0),
                    (center_reference, utils.config.HEIGHT),
                    (0, 255, 0),
                    1,
                )

                # --- ENVIO PARA O STREAMER ---
                # Reduzimos a qualidade para 50% (o padrão é 90-95) para acelerar a codificação
                encode_param = [int(cv2.IMWRITE_JPEG_QUALITY), 50]
                _, jpeg_buffer = cv2.imencode(".jpg", debug_frame, encode_param)

                # _, jpeg_buffer = cv2.imencode(".jpg", debug_frame)
                with streamer.lock:
                    streamer.output_frame = jpeg_buffer.tobytes()

        except Exception as e:
            print(f"[VISÃO] Erro inesperado: {e}")
        finally:
            # Fechamento seguro dos recursos da câmera
            self.picam2.stop()
            self.picam2.close()
            print("[VISÃO] Recursos da câmera liberados.")
