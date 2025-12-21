import os
import numpy as np
from file_manager import AudioManager
from audio_io import save_wav
from effects.reverb import apply_reverb, apply_reverb_stereo

# Função auxiliar para estéreo (copiada do validate_effects.py)


def main():
    # 1. Configuração Inicial
    manager = AudioManager(base_folder="audio_files", dry_key="ORIGINAL") 
    fs, audio_original = manager.get_dry_audio()
    
    output_folder = "output"

    # 2. Definição dos Presets (Parâmetros Otimizados/Primos)
       
    presets = {
        "REV-HALL": {
            "combs_ms": [26.84, 28.02, 45.82, 33.63],
            "combs_gains": [0.758, 0.854, 0.796, 0.825],
            "aps_ms": [2.35, 6.9],
            "aps_gains": [0.653, 0.659],
            "wet_gain": 0.4
        },
        "REV-ROOM": {
            "combs_ms": [419, 359, 251, 467],
            "combs_gains": [0.50, 0.48, 0.56, 0.44],
            "aps_ms": [7.06, 6.46],
            "aps_gains": [0.716, 0.613],
            "wet_gain": 0.2
        },
        "REV-STAGE": {
            "combs_ms": [46.27, 39.96, 28.03, 51.85],
            "combs_gains": [0.758, 0.854, 0.796, 0.825],
            "aps_ms": [3.5, 1.2],
            "aps_gains": [0.7, 0.7],
            "wet_gain": 0.5
        }
    }

    print(f"--- Gerando 6 arquivos de Reverb (Mono/Stereo) ---")

    # 3. Loop de Geração
    for name, params in presets.items():
        print(f"\nProcessando {name}...")
        
        # Extrair parâmetross
        c_ms = params["combs_ms"]
        c_g  = params["combs_gains"]
        a_ms = params["aps_ms"]
        a_g  = params["aps_gains"]
        wet  = params["wet_gain"]

        # A. Gerar MONO
        audio_mono = apply_reverb(
            audio_original, fs, 
            delays_combs_ms=c_ms, 
            gains_combs=c_g, 
            delays_ap_ms=a_ms, 
            gains_ap=a_g, 
            wet_gain=wet
        )
        filename_mono = f"{output_folder}/{name}_mono.wav"
        save_wav(filename_mono, fs, audio_mono)
        print(f" > Salvo: {filename_mono}")

        # B. Gerar ESTÉREO (com spread)
        audio_stereo = apply_reverb_stereo(
            audio_original, fs, 
            delays_combs=c_ms, 
            gains_combs=c_g, 
            delays_ap=a_ms, 
            gains_ap=a_g, 
            wet_gain=wet,
            spread=23 # ~0.5ms de defasagem para abrir a imagem
        )
        filename_stereo = f"{output_folder}/{name}_stereo.wav"
        save_wav(filename_stereo, fs, audio_stereo)
        print(f" > Salvo: {filename_stereo}")

    print("\nConcluído! Verifique a pasta 'output/'.")

if __name__ == "__main__":
    main()