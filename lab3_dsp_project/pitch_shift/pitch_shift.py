import numpy as np

# https://www.youtube.com/watch?v=PjKlMXhxtTM # Vídeo explicando o algoritmo de pitch-shifting
# https://github.com/JentGent/pitch-shift # Exemplo de implementação em Python utilizando outros algoritmos de pitch-shifting.
# Outra implementação: https://github.com/danigb/timestretch

class RealTimePitchShifter:
    def __init__(self, fs, window_size_ms=30):
        """
        Inicializa o Pitch Shifter utilizando a técnica de Overlap-Add (OLA) com buffer circular.
        
        Args:
            fs: Taxa de amostragem.
            window_size_ms: Tamanho da janela em ms. Quanto menor, mais robótico.
        """
        
        self.fs = fs
        self.window_size = int((window_size_ms / 1000.0) * fs) # Tamanho da janela em amostras
        
        # Buffer circular: precisa ser maior que a janela por segurança
        self.buffer_len = self.window_size * 2 
        self.buffer = np.zeros(self.buffer_len, dtype=np.float32)
        
        self.write_ptr = 0 # Ponteiro de escrita no buffer circular
        self.phasor = 0.0 # Controla a posição relativa dos ponteiros de leitura
    
    def process_block(self, x, semitones):
        """Processa um bloco de áudio mantendo o estado interno."""
        
        N = len(x)
        y = np.zeros(N, dtype=np.float32)
        
        # 1. Calcular o fator de velocidade
        # Se factor = 2.0 (oitava acima), o delay precisa diminuir rápido.
        factor = 2 ** (semitones / 12.0)
        
        # A taxa de variação do delay.
        # R = 1.0 - factor. 
        # Ex: Se factor é 2.0, R = -1.0 (o delay encurta 1 amostra a cada amostra).
        delay_rate = (1.0 - factor) / self.window_size

        # 2. Processamento (Loop por amostra)
        for n in range(N):
            input_sample = x[n]
            
            # A. Escrever no Buffer Circular na velocidade normal (1x)
            self.buffer[self.write_ptr] = input_sample
            
            # B. Calcular a posição dos Ponteiros de Leitura
            # Se utiliza dois ponteiros (A e B) defasados em 180 graus (0.5) para fazer crossfade
            # Phasor varia de 0.0 a 1.0 (dente de serra)
            
            delay_a = self.phasor * self.window_size # Delay do ponteiro A
            delay_b = ((self.phasor + 0.5) % 1.0) * self.window_size # Delay do ponteiro B (180 graus defasado)
            
            # C. Ler do Buffer com Interpolação (Função auxiliar interna)
            val_a = self._read_buffer(self.write_ptr - delay_a)
            val_b = self._read_buffer(self.write_ptr - delay_b)
            
            # D. Calcular Ganho do Crossfade (Janela Triangular)
            # Quando o ponteiro A está no meio (0.5), ganho é máximo. Nas pontas, é zero (esconde o 'clique').
            # Função triangular simples: 1 - 2 * |x - 0.5|
            gain_a = 1.0 - 2.0 * abs(self.phasor - 0.5)
            gain_b = 1.0 - 2.0 * abs(((self.phasor + 0.5) % 1.0) - 0.5)
            
            # E. Somar as saídas ponderadas
            y[n] = (val_a * gain_a) + (val_b * gain_b)

            # 3. Atualizar Ponteiros
            self.write_ptr = (self.write_ptr + 1) % self.buffer_len
            
            # Atualiza o phasor
            self.phasor += delay_rate
            
            # Mantém o phasor dentro de 0.0 a 1.0 (Loop do atraso)
            if self.phasor >= 1.0:
                self.phasor -= 1.0
            elif self.phasor < 0.0:
                self.phasor += 1.0
                
        # Normalização de segurança
        peak = np.max(np.abs(y))
        if peak > 1.0:
            y /= peak
            
        return y

    def _read_buffer(self, position_float):
        """Lê do buffer circular com Interpolação Linear."""
        # Garante circularidade
        while position_float < 0:
            position_float += self.buffer_len
        while position_float >= self.buffer_len:
            position_float -= self.buffer_len
            
        # Índices para interpolação linear
        idx_int = int(position_float) # Índice inteiro
        frac = position_float - idx_int # Parte fracionária
        
        # Próximo índice (circular) 
        idx_next = (idx_int + 1) % self.buffer_len
        
        val_current = self.buffer[idx_int]
        val_next = self.buffer[idx_next]
        
        return (val_current * (1.0 - frac)) + (val_next * frac)

def change_pitch(x, fs, semitones):
    print(f"--- Pitch Shift (Real-Time Granular): {semitones} semitons ---")
    
    # Instancia a classe
    shifter = RealTimePitchShifter(fs, window_size_ms=40)
    
    # Processa o áudio todo (simulando stream)
    return shifter.process_block(x, semitones)