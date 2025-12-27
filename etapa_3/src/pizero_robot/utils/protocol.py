import utils.config


def encode_error(error_value):
    """
    Converte o erro numérico da visão em um byte de comando usando config.py.
    """
    if error_value is None:
        return bytes([utils.config.CMD_PARE])

    # Margem de erro (Zona morta)
    THRESHOLD = 20

    if abs(error_value) <= THRESHOLD:
        command = utils.config.CMD_RETO
    elif error_value < -THRESHOLD:
        command = utils.config.CMD_ESQUERDA
    else:
        command = utils.config.CMD_DIREITA

    return bytes([command])


def decode_color_id(data):
    """
    Decodifica o ID da cor enviado pela Pico via Bluetooth.
    """
    try:
        # data[0] pega o primeiro byte do pacote BLE (0x01, 0x02 ou 0x03)
        color_id = data[0]
        return color_id
    except (IndexError, TypeError):
        # Se o pacote estiver vazio ou inválido, o robô assume
        # Vermelho por segurança para não travar o sistema.
        return utils.config.COR_VERMELHO
