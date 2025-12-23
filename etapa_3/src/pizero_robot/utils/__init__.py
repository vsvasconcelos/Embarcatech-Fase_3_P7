"""
Pacote de Utilidades.
Este arquivo expõe as funções de conversão do protocolo binário para
facilitar o acesso em outros módulos do sistema.
"""

from .protocol import decode_color_id, encode_error

# O __all__ define o que será importado caso alguém use: from utils import *
__all__ = ["encode_error", "decode_color_id"]
