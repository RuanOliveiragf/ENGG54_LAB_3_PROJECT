import numpy as np
from effects.assets import Oscillator

def apply_tremolo(x, fs, rate_hz=5.0, depth=0.8):
    """    
    Args:
        x: Sinal de áudio de entrada.
        fs: Taxa de amostragem do áudio.
        rate_hz: Velocidade da oscilação (Típico: 3Hz a 8Hz).
        depth: Profundidade do efeito (0.0 a 1.0). 
    """
    print(f"--- Tremolo: Rate={rate_hz}Hz, Depth={depth} ---")
    
    N = len(x)
    y = np.zeros(N, dtype=np.float32)
    
    # 1. CONFIGURAR O LFO (OSCILADOR)
    # A profundidade do tremolo (0 a 1, controla quanto o volume varia)     
    amp_val = depth / 2.0
    center_val = 1.0 - amp_val
    
    # Instancia o LFO
    lfo = Oscillator(fs=fs, 
                     rate_hz=rate_hz, 
                     center_val=center_val, 
                     range_val=amp_val)
    
    # 2. PROCESSAMENTO
    for n in range(N):
        input_sample = x[n]
        
        # A. Obter o ganho atual do LFO
        current_gain = lfo.next()
        
        # B. Aplicação do Tremolo (AM)
        # y[n] = x[n] * (1 + depth * sin(2*pi*rate*n/fs)) / 2
        y[n] = input_sample * current_gain
    
    peak = np.max(np.abs(y))
    if peak > 1.0:
        y /= peak
        
    return y