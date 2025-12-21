import numpy as np

class IIRCombFilter:
    def __init__(self, delay_ms, gain, fs):
        self.delay_samples = int((delay_ms / 1000.0) * fs)
        self.gain = gain
        self.buffer = np.zeros(self.delay_samples, dtype=np.float32)
        self.ptr = 0

    def process(self, x):
        # 1. Lê o valor atrasado
        output = self.buffer[self.ptr]
        
        # 2. Realimentação
        # y[n] = x[n] + g * y[n-M]
        new_val = x + (self.gain * output)
        
        # 3. Atualiza buffer circular
        self.buffer[self.ptr] = new_val
        self.ptr = (self.ptr + 1) % self.delay_samples
        
        return output

class FIRCombFilter:
    def __init__(self, delay_ms, gain, fs):
        self.delay_samples = int((delay_ms / 1000.0) * fs)
        self.gain = gain
        self.buffer = np.zeros(self.delay_samples, dtype=np.float32)
        self.ptr = 0

    def process(self, x):        
        # 1. Lê o valor atrasado
        output = self.buffer[self.ptr]
        
        # Implementação FIR Comb Filter
        # y[n] = x[n] + g * x[n-M]
        self.buffer[self.ptr] = x + (self.gain * output)
        
        # 2. Atualiza buffer circular
        self.ptr = (self.ptr + 1) % self.delay_samples
        
        return output

class AllPassFilter:
    def __init__(self, delay_ms, gain, fs):
        self.delay_samples = int((delay_ms / 1000.0) * fs)
        self.gain = gain
        self.buffer = np.zeros(self.delay_samples, dtype=np.float32)
        self.ptr = 0

    def process(self, x):
        delayed = self.buffer[self.ptr]
        
        # Forma Canônica Schroeder All-Pass
        # buffer_in = x + g * delayed
        buffer_in = x + (self.gain * delayed)
        
        # y = delayed - g * buffer_in
        y = delayed - (self.gain * buffer_in)
        
        self.buffer[self.ptr] = buffer_in
        self.ptr = (self.ptr + 1) % self.delay_samples
        
        return y
    
class Oscillator:
    def __init__(self, fs, rate_hz, center_val, range_val):
        """
        Oscilador de Baixa Frequência (LFO) Genérico.
        Pode ser usado para modular atraso (Flanger) ou ganho (Tremolo).
        
        Fórmula: y(n) = center_val + range_val * sin(2*pi*rate*n/fs)
        
        Args:
            fs (float): Taxa de amostragem do áudio.
            rate_hz (float): Velocidade da oscilação em Hz (ex: 0.5 Hz).
            center_val (float): Valor central da oscilação (Offset / DC).
            range_val (float): Amplitude da oscilação (Swing). O valor vai variar
                               entre (center - range) e (center + range).
        """
        self.fs = fs
        self.rate = rate_hz
        self.center = center_val
        self.range = range_val
        
        self.phase = 0.0
        # Calcula o passo da fase por amostra: omega / fs
        self.phase_step = 2 * np.pi * self.rate / self.fs

    def next(self):
        """Retorna o próximo valor da modulação."""
        # Calcula o valor da senoide (-1 a 1)
        osc_val = np.sin(self.phase)
        
        # Mapeia para a escala desejada (ex: atraso em amostras ou ganho)
        output = self.center + (self.range * osc_val)
        
        # Atualiza a fase para a próxima chamada
        self.phase += self.phase_step
        if self.phase > 2 * np.pi:
            self.phase -= 2 * np.pi
            
        return output