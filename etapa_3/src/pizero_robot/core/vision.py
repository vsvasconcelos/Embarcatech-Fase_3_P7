import queue
import time

import cv2
import numpy as np
from picamera2 import Picamera2

import utils.config
from utils import streamer


class ColorFollower:
    """Classe responsável pelo processamento de imagem com ROI, Telemetria e Debug em Amarelo."""

    def __init__(self, color_queue, error_queue):
        self.color_queue = color_queue
        self.error_queue = error_queue
        self.current_id = 1
        self.lower_hsv, self.upper_hsv = self._update_color_limits(self.current_id)

        self.picam2 = Picamera2()
        self.config = self.picam2.create_preview_configuration(
            main={"format": "RGB888", "size": (utils.config.WIDTH, utils.config.HEIGHT)}
        )
        self.picam2.configure(self.config)

        self.fps = 0
        self.last_time = time.time()
        self.current_error = 0

    def _update_color_limits(self, color_id):
        ranges = utils.config.COLOR_MAP.get(color_id, utils.config.COLOR_MAP[1])
        return np.array(ranges["lower"]), np.array(ranges["upper"])

    def run(self):
        self.picam2.start()
        print("[VISÃO] Sistema Ativo: Alvo e Linha Central em AMARELO.")
        center_reference = utils.config.WIDTH // 2

        frame_count = 0
        SKIP_FACTOR = 6
        YELLOW = (0, 255, 255)  # Definição da cor Amarela (BGR) | Linha de Referência
        ORANGE = (0, 165, 255)  # Definição da cor Laranja (BGR) | Linha ROI

        try:
            while True:
                now = time.time()
                dt = now - self.last_time
                self.last_time = now
                if dt > 0:
                    self.fps = 1 / dt

                frame_count += 1

                try:
                    new_id = self.color_queue.get_nowait()
                    if new_id != self.current_id:
                        self.current_id = new_id
                        self.lower_hsv, self.upper_hsv = self._update_color_limits(
                            new_id
                        )
                except queue.Empty:
                    pass

                frame_raw = self.picam2.capture_array()
                if frame_raw is None:
                    continue

                # Inversão de canais para corrigir o hardware
                frame = frame_raw[:, :, ::-1].copy()

                # ROI: Foco nos 60% inferiores
                roi_y_start = int(utils.config.HEIGHT * 0.4)
                roi_frame = frame[roi_y_start:, :, :]

                # Processamento Lógico (HSV)
                hsv = cv2.cvtColor(roi_frame, cv2.COLOR_RGB2HSV)
                mask = cv2.inRange(hsv, self.lower_hsv, self.upper_hsv)
                mask = cv2.erode(mask, None, iterations=1)
                mask = cv2.dilate(mask, None, iterations=1)

                contours, _ = cv2.findContours(
                    mask, cv2.RETR_EXTERNAL, cv2.CHAIN_APPROX_SIMPLE
                )

                self.current_error = 0

                if contours:
                    largest_contour = max(contours, key=cv2.contourArea)
                    if cv2.contourArea(largest_contour) > 300:
                        M = cv2.moments(largest_contour)
                        if M["m00"] != 0:
                            cx = int(M["m10"] / M["m00"])
                            self.current_error = cx - center_reference

                            if self.error_queue.full():
                                try:
                                    self.error_queue.get_nowait()
                                except queue.Empty:
                                    pass
                            self.error_queue.put(self.current_error)

                # --- ENVIO PARA O STREAMER (FRAME SKIPPING) ---
                if frame_count >= SKIP_FACTOR:
                    debug_frame = cv2.cvtColor(frame, cv2.COLOR_RGB2BGR)

                    # Desenhamos uma linha horizontal onde o processamento da ROI começa
                    cv2.line(
                        debug_frame,
                        (0, roi_y_start),
                        (utils.config.WIDTH, roi_y_start),
                        ORANGE,
                        1,
                    )

                    # 1. Linha Central Vertical (AMARELA)
                    cv2.line(
                        debug_frame,
                        (center_reference, 0),
                        (center_reference, utils.config.HEIGHT),
                        YELLOW,
                        2,
                    )

                    # 2. Círculo de Detecção (AMARELO)
                    if contours and cv2.contourArea(largest_contour) > 300:
                        M = cv2.moments(largest_contour)
                        if M["m00"] != 0:
                            cx = int(M["m10"] / M["m00"])
                            cy = int(M["m01"] / M["m00"]) + roi_y_start
                            # Alterado para YELLOW
                            cv2.circle(debug_frame, (cx, cy), 10, YELLOW, -1)

                    # Telemetria
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
                        f"Alvo: {color_name} | Erro: {self.current_error}",
                        (10, 40),
                        cv2.FONT_HERSHEY_SIMPLEX,
                        0.5,
                        YELLOW,
                        1,
                    )

                    # Codificação JPEG (Qualidade 50% para fluidez)
                    encode_param = [int(cv2.IMWRITE_JPEG_QUALITY), 50]
                    _, jpeg_buffer = cv2.imencode(".jpg", debug_frame, encode_param)

                    with streamer.lock:
                        streamer.output_frame = jpeg_buffer.tobytes()
                    frame_count = 0

        except Exception as e:
            print(f"[VISÃO] Erro: {e}")
        finally:
            self.picam2.stop()
            self.picam2.close()
