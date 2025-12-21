import numpy as np
from scipy.io import wavfile

def load_wav(filename):
    """
    Carrega um arquivo WAV e o converte para float32 normalizado (-1.0 a 1.0).
    Retorna: (taxa_amostragem, dados_audio)
    """
    fs, data = wavfile.read(filename)
    
    # Se estéreo, converte para mono (média dos canais) para simplificar a validação inicial
    if len(data.shape) > 1:
        data = np.mean(data, axis=1)

    # Normalização dependendo do tipo de dado original
    if data.dtype == np.int16:
        data = data.astype(np.float32) / 32768.0
    elif data.dtype == np.int32:
        data = data.astype(np.float32) / 2147483648.0
    
    return fs, data

def save_wav(filename, fs, data):
    """Salva um array float32 como arquivo WAV int16."""
    # Clip para evitar distorção digital
    data = np.clip(data, -1.0, 1.0)
    # Converter para int16
    data_int16 = (data * 32767).astype(np.int16)
    wavfile.write(filename, fs, data_int16)