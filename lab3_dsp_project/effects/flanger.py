import numpy as np
from effects.assets import Oscillator, FIRCombFilter

def apply_flanger(x, fs, delay_min_ms=1.0, delay_max_ms=5.0, rate_hz=0.5, depth_gain=0.7):
    """
    Aplica o efeito Flanger combinando um FIR Comb Filter com um Oscilador (LFO).
    
    Args:
        x: Sinal de áudio de entrada.
        fs: Taxa de amostragem do áudio.
        delay_min_ms: Atraso mínimo em milissegundos.
        delay_max_ms: Atraso máximo em milissegundos.
        rate_hz: Velocidade da oscilação do LFO em Hz.
        depth_gain: Ganho do efeito (0.0 a 1.0), controla a intensidade do flanger.
    """
    print(f"--- Flanger: Rate={rate_hz}Hz, Delay={delay_min_ms}-{delay_max_ms}ms ---")
    
    N = len(x)
    y = np.zeros(N, dtype=np.float32)
    
    # 1. PREPARAR O LFO (OSCILADOR)
    # O Flanger precisa modular o ATRASO.
    # Precisamos converter Min/Max ms para Média e Amplitude em AMOSTRAS.
    # L(n) = L0 + A * sin(wn)
    
    avg_delay_s = (delay_min_ms + delay_max_ms) / 2000.0 # Média (L0)
    amp_delay_s = (delay_max_ms - delay_min_ms) / 2000.0 # Amplitude (A)
    
    L0_samples = avg_delay_s * fs
    A_samples  = amp_delay_s * fs
    
    # Instância do LFO
    lfo = Oscillator(fs=fs, 
                     rate_hz=rate_hz, 
                     center_val=L0_samples,
                     range_val=A_samples) 
    
    # 2. PREPARAR O FILTRO
    # FIRCombFilter utilizado apenas como gerenciador de buffer circular
    fir_comb = FIRCombFilter(delay_ms=delay_max_ms + 2.0, gain=depth_gain, fs=fs)
    
    # Acesso direto aos componentes internos para manipulação manual
    buffer = fir_comb.buffer
    buffer_len = len(buffer)
    write_ptr = 0 
    
    # 3. PROCESSAMENTO
    for n in range(N):
        input_sample = x[n]
        
        # A. Obter o atraso atual do LFO
        current_delay_samples = lfo.next()
        
        # B. Ler do Buffer com Interpolação Linear
        # Ponteiro de leitura = Ponteiro de escrita - Atraso do LFO
        read_ptr_float = write_ptr - current_delay_samples
        
        # Circularidade
        while read_ptr_float < 0:
            read_ptr_float += buffer_len
            
        # Interpolação
        
        # 1. Índice inteiro anterior
        idx_int = int(read_ptr_float)
        # 2. Calcula a distância decimal
        frac = read_ptr_float - idx_int
        # 3. Próximo índice (circular)
        idx_next = (idx_int + 1) % buffer_len
        # 4. Lê os valores reais armazenados no buffer
        val_current = buffer[idx_int]
        val_next = buffer[idx_next]
        # 5. Calcula a média ponderada
        delayed_val = (val_current * (1.0 - frac)) + (val_next * frac)
        
        # C. Equação FIR Comb: y[n] = x[n] + g * x[n - L(n)]
        y[n] = input_sample + (depth_gain * delayed_val)
        
        # D. Escrever no Buffer
        buffer[write_ptr] = input_sample
        write_ptr = (write_ptr + 1) % buffer_len

    # Normalização de segurança
    peak = np.max(np.abs(y))
    if peak > 1.0:
        y /= peak
        
    return y