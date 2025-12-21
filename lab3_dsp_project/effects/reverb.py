import numpy as np
from effects.assets import IIRCombFilter, AllPassFilter

def apply_reverb(x, fs, delays_combs_ms, gains_combs, delays_ap_ms, gains_ap, wet_gain=0.4, print_info=True):
    
    if print_info:
        print(f"--- Reverb: Processando {len(x)} amostras a {fs}Hz ---")
    
    # 1. Inicializa Filtros (Recebendo MS direto)
    combs = []
    for i in range(len(delays_combs_ms)):
        combs.append(IIRCombFilter(delays_combs_ms[i], gains_combs[i], fs))
        
    aps = []
    for i in range(len(delays_ap_ms)):
        aps.append(AllPassFilter(delays_ap_ms[i], gains_ap[i], fs))
    
    N = len(x)
    y = np.zeros(N, dtype=np.float32)

    comb_scale = 1.0 / len(combs) 
        
    # 2. Loop de Áudio
    for n in range(N):
        input_sample = x[n]
        # Soma dos Combs (Paralelo)
        comb_sum = 0.0
        for comb in combs:
            comb_sum += comb.process(input_sample)
            
        ap_out = comb_sum * comb_scale
            
        # Série de All-Pass
        for ap in aps:
            ap_out = ap.process(ap_out)
            
        reverb_signal = ap_out
        
        #y[n] = (x[n] * (1 - wet_gain)) + (reverb_signal * wet_gain)
        y[n] = input_sample + (reverb_signal * wet_gain)
            
    # 3. Normalização de Segurança (Evita o ruído digital se passar de 1.0)
    max_amp = np.max(np.abs(y))
    if max_amp > 1.0:
        if print_info:
            print(f" > Normalizando volume final (Pico: {max_amp:.2f})")
        y = y / max_amp
        
    return y

def apply_reverb_stereo(x, fs, delays_combs, gains_combs, delays_ap, gains_ap, wet_gain=0.4, spread=23):
    """
    Gera um Reverb Estéreo processando L e R com atrasos ligeiramente diferentes.
    """
    # 1. Calcula desvio em ms para o canal direito
    spread_ms = (spread / fs) * 1000.0
    
    # Canal Esquerdo (Parâmetros Originais)
    left = apply_reverb(x, fs, delays_combs, gains_combs, delays_ap, gains_ap, wet_gain)
    
    # Canal Direito (Parâmetros com Spread nos Combs)
    delays_right = [d + spread_ms for d in delays_combs]
    
    right = apply_reverb(x, fs, delays_right, gains_combs, delays_ap, gains_ap, wet_gain)
    
    # Junta em estéreo (N, 2)
    stereo_output = np.column_stack((left, right))
    
    return stereo_output