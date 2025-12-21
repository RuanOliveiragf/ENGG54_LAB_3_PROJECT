import os
from audio_io import load_wav, save_wav
from effects.reverb import apply_reverb_stereo
from effects.flanger import apply_flanger
from effects.tremolo import apply_tremolo
from pitch_shift.shift_assets import shift_to_note

REVERB_PRESETS = {
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

def main():

    output_folder = "output/final"
    if not os.path.exists(output_folder):
        os.makedirs(output_folder)
        
    print("Carregando áudio original...")
    fs, audio_original = load_wav("audio_files/original.wav") #

    tasks = [
        {"name": "REV-HALL",      "type": "reverb", "preset": "REV-HALL"},
        {"name": "REV-ROOM2",      "type": "reverb", "preset": "REV-ROOM"},
        {"name": "REV-STAGE B",   "type": "stage",  "note": "B4"},
        {"name": "REV-STAGE D",   "type": "stage",  "note": "D4"},
        {"name": "REV-STAGE F",   "type": "stage",  "note": "F4"},
        {"name": "REV-STAGE Gb",  "type": "stage",  "note": "F#4"},
        {"name": "FLANGER",       "type": "flanger"},
        {"name": "TREMOLO",       "type": "tremolo"}
    ]

    print(f"--- Gerando {len(tasks)} efeitos em '{output_folder}' ---")

    for task in tasks:
        name = task["name"]
        effect_type = task["type"]
        print(f"\nProcessando: {name}...")

        if effect_type == "reverb":
            
            params = REVERB_PRESETS[task["preset"]]
            processed = apply_reverb_stereo(
                audio_original, fs,
                delays_combs=params["combs_ms"],
                gains_combs=params["combs_gains"],
                delays_ap=params["aps_ms"],
                gains_ap=params["aps_gains"],
                wet_gain=params["wet_gain"]
            )
            
        elif effect_type == "stage":
            
            target_note = task["note"]
            print(f" > Aplicando Pitch Shift para {target_note}...")
            shifted_audio = shift_to_note(audio_original, fs, target_note=target_note, root_note="C4")
            
            params = REVERB_PRESETS["REV-STAGE"]
            print(f" > Aplicando Reverb Stage...")
            processed = apply_reverb_stereo(
                shifted_audio, fs,
                delays_combs=params["combs_ms"],
                gains_combs=params["combs_gains"],
                delays_ap=params["aps_ms"],
                gains_ap=params["aps_gains"],
                wet_gain=params["wet_gain"]
            )

        elif effect_type == "flanger":
            processed = apply_flanger(audio_original, fs)

        elif effect_type == "tremolo":
            processed = apply_tremolo(audio_original, fs)

        filename = f"{output_folder}/{name.replace(' ', '_')}.wav"
        save_wav(filename, fs, processed)
        print(f" > Salvo: {filename}")

    print("\nProcessamento concluído!")

if __name__ == "__main__":
    main()