//////////////////////////////////////////////////////////////////////////////
// pitch_shift.h - Header para Pitch Shift em Tempo Real
//////////////////////////////////////////////////////////////////////////////

#ifndef PITCH_SHIFT_H_
#define PITCH_SHIFT_H_

#include "tistdtypes.h"

// Configurações
#define ROOT_FREQ_HZ    261.63f  // Nota Dó (C4) como raiz
#define WINDOW_SIZE_MS  30.0f    // Tamanho da janela (ms)
#define FS_HZ           48000.0f // Taxa de amostragem

// Estrutura do Pitch Shifter
typedef struct {
    Int16* buffer;        // Buffer circular de áudio
    Uint16 buffer_len;    // Tamanho total do buffer
    Uint16 window_size;   // Tamanho da janela em amostras
    Uint16 write_ptr;     // Ponteiro de escrita

    // Variáveis de Controle em Ponto Fixo (Q31/Q32)
    Uint32 phasor;        // Fase atual (0x00000000 a 0xFFFFFFFF representa 0.0 a 1.0)
    Int32  delay_rate;    // Taxa de variação do delay por amostra (Q32)

} PitchShifter;

// Instância Global
extern PitchShifter g_pitch;

// Protótipos
void initPitchShift();
Int16 processPitchShiftSample(Int16 input);
void processAudioPitchShift(Uint16* rxBlock, Uint16* txBlock);

// Muda a frequência alvo instantaneamente (chamar no main loop)
void setPitchFrequency(float target_freq);

#endif /* PITCH_SHIFT_H_ */
