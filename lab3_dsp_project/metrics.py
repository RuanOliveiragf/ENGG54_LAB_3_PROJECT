import numpy as np

def calculate_mse(original, processed):
    """Mean Squared Error"""
    # Garante mesmo tamanho
    min_len = min(len(original), len(processed))
    original = original[:min_len]
    processed = processed[:min_len]
    
    return np.mean((original - processed) ** 2)

def calculate_snr(original, processed):
    """Signal to Noise Ratio (dB) assuming 'processed' tries to mimic 'original'"""
    min_len = min(len(original), len(processed))
    original = original[:min_len]
    processed = processed[:min_len]
    
    noise = original - processed
    signal_power = np.sum(original ** 2)
    noise_power = np.sum(noise ** 2)
    
    if noise_power == 0:
        return np.inf
    
    return 10 * np.log10(signal_power / noise_power)

def calculate_prd(original, processed):
    """Percent Root-mean-square Difference"""
    min_len = min(len(original), len(processed))
    original = original[:min_len]
    processed = processed[:min_len]
    
    numerator = np.sum((original - processed) ** 2)
    denominator = np.sum(original ** 2)
    
    return np.sqrt(numerator / denominator) * 100