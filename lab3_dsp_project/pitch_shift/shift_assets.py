import numpy as np
from pitch_shift.notes import get_freq
from pitch_shift.pitch_shift import change_pitch

def shift_to_note(audio, fs, target_note, root_note="C4"):
    """
    Muda o tom do áudio de uma Nota Raiz para uma Nota Alvo.
    
    Args:
        audio: Array de áudio.
        target_note (str): Nota desejada (ex: 'G#4', 'E5').
        root_note (str): Nota original do áudio. Padrão é 'C4'.
    Returns:
        np.array: Áudio com nova afinação (duração alterada).
    """
    freq_root = get_freq(root_note)
    freq_target = get_freq(target_note)
    
    if freq_root is None or freq_target is None:
        print(f"Erro: Nota '{root_note}' ou '{target_note}' inválida.")
        return audio
    
    
    # Cálculo da diferença em semitons
    # f_target = f_root * 2^(n/12)
    # n = 12 * log2(f_target / f_root)
    
    ratio = freq_target / freq_root
    semitones_diff = 12.0 * np.log2(ratio)
    
    print(f"--- Musical Shift: {root_note} ({freq_root:.1f}Hz) -> {target_note} ({freq_target:.1f}Hz) ---")
    print(f" > Diferença: {semitones_diff:.2f} semitons")
    
    return change_pitch(audio, fs, semitones_diff)

def shift_to_freq(audio, fs, target_freq, root_freq=261.63):
    """
    Muda o tom para uma frequência específica.
    
    Args:
        audio: Array de áudio.
        target_freq (float): Frequência desejada em Hz.
        root_freq (float): Frequência original do áudio em Hz.
    
    Returns:
        np.array: Áudio com nova afinação (duração alterada).
    """
    
    # Cálculo da diferença em semitons
    # f_target = f_root * 2^(n/12)
    # n = 12 * log2(f_target / f_root)
    
    ratio = target_freq / root_freq
    semitones_diff = 12.0 * np.log2(ratio)
    
    print(f"--- Frequency Shift: {root_freq:.1f}Hz -> {target_freq:.1f}Hz ---")
    print(f" > Diferença: {semitones_diff:.2f} semitons")
    
    return change_pitch(audio, fs, semitones_diff)