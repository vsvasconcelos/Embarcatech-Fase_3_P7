import struct  # Biblioteca para converter dados Python em bytes (formato C)


def encode_error(error: int) -> bytes:
    """
    Converte um inteiro (erro do centroide) em 2 bytes (short).
    Formato: '<h'
    '<' = Little Endian (padrão ARM/Pico)
    'h' = signed short (inteiro de 16 bits, suporta negativos)
    """
    try:
        # Garante que o erro esteja no limite de 16 bits (-32768 a 32767)
        error = max(min(error, 32767), -32768)
        return struct.pack("<h", int(error))
    except Exception as e:
        print(f"Erro na codificação do protocolo: {e}")
        return struct.pack("<h", 0)


def decode_color_id(data: bytes) -> int:
    """
    Converte o byte recebido da Pico em um inteiro.
    Formato: '<B'
    'B' = unsigned char (inteiro de 8 bits, 0 a 255)
    """
    try:
        # Desempacota o primeiro byte da sequência recebida
        return struct.unpack("<B", data)[0]
    except Exception as e:
        print(f"Erro na decodificação da cor: {e}")
        return 1  # Retorna ID padrão em caso de erro
