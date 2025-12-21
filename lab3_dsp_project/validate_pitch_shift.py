from file_manager import AudioManager
from audio_io import save_wav
from pitch_shift.shift_assets import shift_to_note, shift_to_freq

output_folder = "output"

def main():
    manager = AudioManager(base_folder="audio_files", dry_key="ORIGINAL") 
    fs, audio = manager.get_dry_audio()
    
    ROOT_NOTE = "C4" 
    
    # 1. Criar uma Tríade Maior (Dó - Mi - Sol)
    #note_C4 = shift_to_note(audio, fs,target_note="C4", root_note=ROOT_NOTE) # Original
    #note_E4 = shift_to_note(audio, fs, target_note="E4", root_note=ROOT_NOTE) # Terça
    note_A4 = shift_to_note(audio, fs, target_note="A4", root_note=ROOT_NOTE) # Quinta
    
    #save_wav(f"{output_folder}/note_C4.wav", fs, note_C4)
    #save_wav(f"{output_folder}/note_E4.wav", fs, note_E4)
    save_wav(f"{output_folder}/note_A4.wav", fs, note_A4)
    
    target_frequencies = {
        "432Hz_Verdi": 432.0,  # Afinação "Aurea" / Verdi
        "528Hz_Love": 528.0,   # Frequência de Solfeggio (DNA Repair)
        "60Hz_Hum": 60.0,      # Ruído de rede elétrica (Grave)
        "1000Hz_Sine": 1000.0  # Apito padrão de TV (Agudo)
    }
    
    # Teste simples de mudança para 432 Hz a partir de 440 Hz    
    freq_432 = shift_to_freq(audio, fs, target_freq=432.0, root_freq=440.0)
    #save_wav(f"{output_folder}/freq_432.wav", fs, freq_432)

    ROOT_FREQ = 261.63

    for name, freq_target in target_frequencies.items():
        print(f"Processando: {name} ({freq_target} Hz)...")

        processed_audio = shift_to_freq(audio, fs, target_freq=freq_target, root_freq=ROOT_FREQ)
        
        filename = f"{output_folder}/freq_{name}.wav"
        save_wav(filename, fs, processed_audio)

if __name__ == "__main__":
    main()