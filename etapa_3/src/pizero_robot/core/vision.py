import queue  # Para lidar com a exceção Empty da fila

import cv2  # Biblioteca principal de Visão Computacional
import numpy as np  # Biblioteca para operações matemáticas em matrizes (imagens)

import utils.config  # Arquivo de configuração com os limites HSV e resoluções


class ColorFollower:
    """Classe responsável pelo processamento de imagem e detecção da faixa."""

    def __init__(self, color_queue, error_queue):
        # Fila para receber a cor alvo (ID) vinda do BLE
        self.color_queue = color_queue
        # Fila para enviar o erro do centroide para o BLE
        self.error_queue = error_queue

        # Define a cor inicial padrão (ex: ID 1 - Vermelho) de acordo com o config.py
        self.current_id = 1
        self.lower_hsv, self.upper_hsv = self._update_color_limits(self.current_id)

    def _update_color_limits(self, color_id):
        """Busca os limites HSV no dicionário de configuração baseado no ID."""
        ranges = utils.config.COLOR_MAP.get(color_id, utils.config.COLOR_MAP[1])
        return np.array(ranges["lower"]), np.array(ranges["upper"])

    def run(self):
        """Loop principal de captura e processamento de vídeo."""
        # Inicializa a câmera (0 é o índice padrão para a Pi Camera ou USB)
        cap = cv2.VideoCapture(0)

        # Define a resolução reduzida para ganhar performance na Pi Zero
        cap.set(cv2.CAP_PROP_FRAME_WIDTH, utils.config.WIDTH)
        cap.set(cv2.CAP_PROP_FRAME_HEIGHT, utils.config.HEIGHT)

        # Centro da imagem no eixo X (Referência do Robô)
        center_reference = utils.config.WIDTH // 2

        print("[VISÃO] Câmera inicializada. Processamento iniciado...")

        while True:
            # 1. Verifica se o BLE enviou um novo ID de cor
            try:
                # Tenta pegar um novo ID sem bloquear o processamento
                new_id = self.color_queue.get_nowait()
                if new_id != self.current_id:
                    self.current_id = new_id
                    self.lower_hsv, self.upper_hsv = self._update_color_limits(new_id)
                    print(f"[VISÃO] Mudando alvo para Cor ID: {new_id}")
            except queue.Empty:
                pass  # Nenhuma mudança de cor solicitada

            # 2. Captura o frame
            ret, frame = cap.read()
            if not ret:
                print("[VISÃO] Erro ao capturar frame.")
                break

            # 3. Processamento: Espaço de Cor HSV e Máscara
            # HSV é melhor que RGB para segmentar cores sob diferentes iluminações
            hsv = cv2.cvtColor(frame, cv2.COLOR_BGR2HSV)
            mask = cv2.inRange(hsv, self.lower_hsv, self.upper_hsv)

            # Operação morfológica para remover ruídos pequenos (pontos isolados)
            mask = cv2.erode(mask, None, iterations=2)
            mask = cv2.dilate(mask, None, iterations=2)

            # 4. Encontrar contornos da faixa segmentada
            contours, _ = cv2.findContours(
                mask, cv2.RETR_EXTERNAL, cv2.CHAIN_APPROX_SIMPLE
            )

            if contours:
                # Pega o maior contorno (assume-se que é a faixa colorida)
                largest_contour = max(contours, key=cv2.contourArea)

                # Se o contorno for grande o suficiente (evita processar ruído)
                if cv2.contourArea(largest_contour) > 500:
                    # Calcula os Momentos da imagem para achar o centroide
                    M = cv2.moments(largest_contour)
                    if M["m00"] != 0:
                        cx = int(M["m10"] / M["m00"])  # Coordenada X do centroide
                        # cy = int(M["m01"] / M["m00"]) # Coordenada Y (não usada no erro lateral)

                        # 5. Cálculo do Erro
                        # Positivo: faixa à direita | Negativo: faixa à esquerda
                        error = cx - center_reference

                        # Envia o erro para o módulo BLE
                        # Se a fila estiver cheia, remove o erro antigo e coloca o novo
                        if self.error_queue.full():
                            try:
                                self.error_queue.get_nowait()
                            except queue.Empty:
                                pass

                        self.error_queue.put(error)

            # Opcional: O cv2.imshow deve ser evitado na Pi Zero em produção (headless)
            # para economizar processamento e memória RAM.

        cap.release()
