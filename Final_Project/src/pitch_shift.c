//////////////////////////////////////////////////////////////////////////////
// pitch_shift.c - VERSÃO OTIMIZADA E CORRIGIDA
//
// Melhora:
// 1. Usa Interpolação Linear para eliminar ruído metálico.
// 2. Corrige overflow de ganho que causava som picotado.
// 3. Usa aritmética Q15/Q31 para máxima performance no DSP.
//////////////////////////////////////////////////////////////////////////////

#include "pitch_shift.h"
#include "dma.h"

// Tamanho do Buffer fixo em Potência de 2 para velocidade máxima
// 4096 garante espaço suficiente para janelas grandes se necessário
#define PITCH_BUF_SIZE 4096
#define PITCH_MASK     4095

// Configuração da Janela
// 2048 amostras @ 48kHz ~= 42ms (Bom equilíbrio voz/instrumentos)
#define WINDOW_LEN     2048

// Shift amount para converter Phasor High (16b) para Delay Int (11b)
// Phasor varia de 0..65535. Delay varia de 0..2048.
// 65536 >> 5 = 2048. Então o shift é 5.
#define SHIFT_TO_DELAY_INT  5

// Alocação na memória interna (DARAM) para acesso rápido
Int16 pitchBuffer[PITCH_BUF_SIZE];

PitchShifter g_pitch;

// ---------------------------------------------------------------------------
// Inicialização
// ---------------------------------------------------------------------------
void initPitchShift()
{
    int i;
    g_pitch.buffer = pitchBuffer;
    g_pitch.buffer_len = PITCH_BUF_SIZE;
    g_pitch.window_size = WINDOW_LEN;

    g_pitch.write_ptr = 0;
    g_pitch.phasor = 0;

    for(i=0; i<PITCH_BUF_SIZE; i++) pitchBuffer[i] = 0;

    // Inicia na frequência base (1.0x, sem efeito)
    setPitchFrequency(ROOT_FREQ_HZ);
}

// ---------------------------------------------------------------------------
// Macro: Interpolação Linear (Q15)
// Fórmula: y = val + frac * (next - val)
// Usamos aritmética inteira: (frac * diff) >> 15
// ---------------------------------------------------------------------------
#define INTERPOLATE(val, next, frac) \
    (val + ( (Int16)( ((Int32)(next - val) * frac) >> 15 ) ))

// ---------------------------------------------------------------------------
// Processamento de Bloco Otimizado
// ---------------------------------------------------------------------------
void processAudioPitchShift(Uint16* rxBlock, Uint16* txBlock)
{
    int i;

    // Cache de registradores (Evita ler a struct na memória a cada loop)
    Int16* buff = g_pitch.buffer;
    Uint16 w_ptr = g_pitch.write_ptr;
    Uint32 phas = g_pitch.phasor;
    Int32  d_rate = g_pitch.delay_rate;

    // Offset de 180 graus para o ponteiro B (0.5 em Q32 é 0x80000000)
    Uint32 pB_offset = 0x80000000;

    for (i = 0; i < AUDIO_BLOCK_SIZE; i++) {
        Int16 input = (Int16)rxBlock[i];
        Int32 outputAccum;

        // 1. Escreve Entrada no Buffer Circular
        buff[w_ptr] = input;

        // ====================================================================
        // CANAL A (Grão 1)
        // ====================================================================
        // Pega os 16 bits superiores do Phasor (0..65535)
        Uint16 phA_high = phas >> 16;

        // --- Cálculo de Ganho (Janela Triangular) ---
        // Sobe de 0 a 32767, depois desce de 32767 a 0.
        // O valor máximo 32767 representa Ganho 1.0 em Q15.
        Uint16 rawGainA = (phA_high < 32768) ? phA_high : (65535 - phA_high);
        // Saturação de segurança (evita -32768)
        Int16 gainA = (rawGainA > 32767) ? 32767 : (Int16)rawGainA;

        // --- Cálculo de Delay com Fração ---
        // Delay Inteiro: Bits superiores convertidos para o tamanho da janela
        Int16 delayIntA = phA_high >> SHIFT_TO_DELAY_INT;

        // Delay Fracionário: Bits inferiores (máscara 0x7FFF pegando bits [20:6])
        // Isso nos dá a precisão "entre" as amostras para a interpolação.
        Int16 fracA = (phas >> 6) & 0x7FFF;

        // �ndices de Leitura no Buffer
        // idx0 é a amostra base. idx1 é a anterior (para onde o delay fracionário aponta).
        Int16 idxA0 = (w_ptr - delayIntA) & PITCH_MASK;
        Int16 idxA1 = (idxA0 - 1) & PITCH_MASK;

        // Leitura interpolada do Buffer
        Int16 valA = INTERPOLATE(buff[idxA0], buff[idxA1], fracA);

        // ====================================================================
        // CANAL B (Grão 2 - Defasado 180 graus)
        // ====================================================================
        Uint32 phasB = phas + pB_offset;
        Uint16 phB_high = phasB >> 16;

        Uint16 rawGainB = (phB_high < 32768) ? phB_high : (65535 - phB_high);
        Int16 gainB = (rawGainB > 32767) ? 32767 : (Int16)rawGainB;

        Int16 delayIntB = phB_high >> SHIFT_TO_DELAY_INT;
        Int16 fracB = (phasB >> 6) & 0x7FFF;

        Int16 idxB0 = (w_ptr - delayIntB) & PITCH_MASK;
        Int16 idxB1 = (idxB0 - 1) & PITCH_MASK;

        Int16 valB = INTERPOLATE(buff[idxB0], buff[idxB1], fracB);

        // ====================================================================
        // MIXAGEM (Crossfade)
        // ====================================================================
        // Soma ponderada usando acumulador de 32 bits para evitar overflow.
        // Shift >> 15 normaliza o ganho Q15 * Q15 de volta para Q15.
        // Como gainA + gainB soma ~1.0, o volume de saída é unitário (igual à entrada).

        outputAccum = ((Int32)valA * gainA >> 15) + ((Int32)valB * gainB >> 15);

        // Saturação Final (Hard Limiter) para converter de volta a 16 bits
        if (outputAccum > 32767) outputAccum = 32767;
        else if (outputAccum < -32768) outputAccum = -32768;

        txBlock[i] = (Uint16)outputAccum;

        // 3. Atualiza Ponteiros
        w_ptr = (w_ptr + 1) & PITCH_MASK;
        phas += d_rate;
    }

    // Salva estado de volta na estrutura global
    g_pitch.write_ptr = w_ptr;
    g_pitch.phasor = phas;
}

// Stub para compatibilidade (caso chamem a função antiga)
Int16 processPitchShiftSample(Int16 input) { return input; }

// ---------------------------------------------------------------------------
// Define a frequência alvo
// ---------------------------------------------------------------------------
void setPitchFrequency(float target_freq)
{
    // Relação = Freq_Alvo / Freq_Original
    float ratio = target_freq / ROOT_FREQ_HZ;

    // Taxa de variação do delay = (1.0 - ratio) / Tamanho_Janela
    // Se ratio > 1 (agudo), delay diminui. Se ratio < 1 (grave), delay aumenta.
    float delay_rate_f = (1.0f - ratio) / (float)WINDOW_LEN;

    // Converte float para Ponto Fixo Q32
    // Multiplica por 2^32 (4294967296.0)
    g_pitch.delay_rate = (Int32)(delay_rate_f * 4294967296.0f);
}
