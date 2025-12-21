def create_note_dict():
    """
    Gera um dicionário com frequências de notas musicais (C0 a B8).
    Fórmula: f = 440 * 2^((n-69)/12)
    """
    notes = ['C', 'C#', 'D', 'D#', 'E', 'F', 'F#', 'G', 'G#', 'A', 'A#', 'B']
    note_dict = {}
    
    # MIDI note 12 = C0, MIDI 69 = A4
    for midi_note in range(12, 120):
        octave = (midi_note // 12) - 1
        note_name = notes[midi_note % 12]
        full_name = f"{note_name}{octave}"
        
        # Cálculo da frequência
        freq = 440.0 * (2 ** ((midi_note - 69) / 12.0))
        note_dict[full_name] = freq
        
    return note_dict

NOTES = create_note_dict()

def get_freq(note_name):
    """Retorna a frequência de uma nota (ex: 'A4' -> 440.0)."""
    return NOTES.get(note_name.upper(), None)