import os
from file_manager import AudioManager
from effects.reverb import apply_reverb, apply_reverb_stereo
from effects.flanger import apply_flanger
from effects.tremolo import apply_tremolo
from pitch_shift.shift_assets import shift_to_note
import numpy as np

from audio_io import save_wav

def main():
    manager = AudioManager(base_folder="audio_files", dry_key="ORIGINAL") 
    fs, audio_original = manager.get_dry_audio()

    # Comb Filters
    combs_ms = [36.43, 56.21, 44.86, 25.17] # Delays (Tamanho da sala), em ms
    combs_gains = [0.863, 0.807, 0.879, 0.906] # Ganhos 
    
    # All-Pass (Difusão)
    aps_ms = [2.6, 1.22] # Delays em ms
    aps_gains = [0.722, 0.638] # Ganhos

    print("Aplicando Reverb...")
    
    processed_audio_reverb = apply_reverb(
        audio_original, 
        fs, 
        combs_ms,    
        combs_gains, 
        aps_ms,
        aps_gains,
        wet_gain=0.5
    )
    
    processed_audio_reverb_stereo = apply_reverb_stereo(
        audio_original, 
        fs, 
        combs_ms,    
        combs_gains, 
        aps_ms,
        aps_gains,
        wet_gain=0.5
    )
    
    """"processed_audio_flanger = apply_flanger(
        audio_original, 
        fs
    )
    
    processed_audio_tremolo = apply_tremolo(
        audio_original,
        fs
    )
    
    reverb_A4 = shift_to_note(processed_audio_reverb, fs, target_note="A4", root_note="C4")
    reverb_A3 = shift_to_note(processed_audio_reverb, fs, target_note="A3", root_note="C4")
    """
    #save_wav("output/reverb_test_A4.wav", fs, reverb_A4)
    #save_wav("output/reverb_test_A3.wav", fs, reverb_A3)
    save_wav("output/reverb_test.wav", fs, processed_audio_reverb)
    save_wav("output/reverb_test_stereo.wav", fs, processed_audio_reverb_stereo)
    #save_wav("output/flanger_test.wav", fs, processed_audio_flanger)
    #save_wav("output/tremolo_test.wav", fs, processed_audio_tremolo)


if __name__ == "__main__":
    main()