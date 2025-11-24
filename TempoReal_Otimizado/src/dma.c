//////////////////////////////////////////////////////////////////////////////
// dma.c - DMA de áudio em loopback (MIC → FONE) usando ping-pong
//////////////////////////////////////////////////////////////////////////////

#include "ezdsp5502.h"
#include "ezdsp5502_mcbsp.h"
#include "csl_dma.h"
#include "csl_irq.h"
#include "dma.h"
#include "isr.h"
#include "effects.h"  // Inclui as funções de efeitos

// =================== VARIÁVEIS GLOBAIS ===================

// Handles para os canais de DMA
static DMA_Handle hDmaRx;   // Recepção (McBSP → memória)
static DMA_Handle hDmaTx;   // Transmissão (memória → McBSP)

// Flag ping-pong: 0 = PING (primeira metade), 1 = PONG (segunda metade)
volatile Uint16 dmaPingPongFlag = 0;

// Variável para seleção de efeitos (DEFINIÇÃO ÚNICA)
volatile Uint8 currentEffect = EFFECT_LOOPBACK;

// Buffers de entrada (mic) e saída (fone) - REDUZIDOS
#pragma DATA_SECTION(RxBuffer, "dmaMem")
#pragma DATA_ALIGN(RxBuffer, 512)
Uint16 RxBuffer[AUDIO_BUFFER_SIZE];  // 1KB total

#pragma DATA_SECTION(TxBuffer, "dmaMem")
#pragma DATA_ALIGN(TxBuffer, 512)
Uint16 TxBuffer[AUDIO_BUFFER_SIZE];  // 1KB total

// Vetor de interrupções (vem do projeto da TI)
extern void VECSTART(void);

// =================== CONFIGURAÇÃO DMA TX ===================

static DMA_Config dmaTxConfig = {
    DMA_DMACSDP_RMK(
        DMA_DMACSDP_DSTBEN_NOBURST,   // Destination burst
        DMA_DMACSDP_DSTPACK_OFF,      // Destination packing
        DMA_DMACSDP_DST_PERIPH,       // Destino: periférico (McBSP1 DXR)
        DMA_DMACSDP_SRCBEN_NOBURST,   // Source burst
        DMA_DMACSDP_SRCPACK_OFF,      // Source packing
        DMA_DMACSDP_SRC_DARAMPORT1,   // Origem: DARAM (memória)
        DMA_DMACSDP_DATATYPE_16BIT    // 16 bits
    ),
    DMA_DMACCR_RMK(
        DMA_DMACCR_DSTAMODE_CONST,    // Endereço destino fixo (DXR)
        DMA_DMACCR_SRCAMODE_POSTINC,  // Origem incrementa (buffer)
        DMA_DMACCR_ENDPROG_OFF,
        DMA_DMACCR_WP_DEFAULT,
        DMA_DMACCR_REPEAT_ALWAYS,     // Loop infinito
        DMA_DMACCR_AUTOINIT_ON,       // Auto-init
        DMA_DMACCR_EN_STOP,           // Começa parado
        DMA_DMACCR_PRIO_HI,           // Alta prioridade
        DMA_DMACCR_FS_ELEMENT,        // Sync por elemento
        DMA_DMACCR_SYNC_XEVT1         // Evento de TX da McBSP1
    ),
    DMA_DMACICR_RMK(
        DMA_DMACICR_AERRIE_OFF,
        DMA_DMACICR_BLOCKIE_OFF,
        DMA_DMACICR_LASTIE_OFF,
        DMA_DMACICR_FRAMEIE_OFF,      // Tx não precisa de interrupção
        DMA_DMACICR_FIRSTHALFIE_OFF,
        DMA_DMACICR_DROPIE_OFF,
        DMA_DMACICR_TIMEOUTIE_OFF
    ),
    (DMA_AdrPtr)0x0000,   // Origem: será TxBuffer em configAudioDma
    0,
    (DMA_AdrPtr)0x5804,   // DXR1 da McBSP1 (transmissão)
    0,
    AUDIO_BUFFER_SIZE,    // # de elementos de 16 bits
    1,                    // 1 frame
    0,
    0,
    0,
    0
};

// =================== CONFIGURAÇÃO DMA RX ===================

static DMA_Config dmaRxConfig = {
    DMA_DMACSDP_RMK(
        DMA_DMACSDP_DSTBEN_NOBURST,
        DMA_DMACSDP_DSTPACK_OFF,
        DMA_DMACSDP_DST_DARAMPORT1,   // Destino: memória
        DMA_DMACSDP_SRCBEN_NOBURST,
        DMA_DMACSDP_SRCPACK_OFF,
        DMA_DMACSDP_SRC_PERIPH,       // Origem: McBSP1 DRR
        DMA_DMACSDP_DATATYPE_16BIT
    ),
    DMA_DMACCR_RMK(
        DMA_DMACCR_DSTAMODE_POSTINC,  // Destino incrementa (buffer)
        DMA_DMACCR_SRCAMODE_CONST,    // Origem fixa (DRR)
        DMA_DMACCR_ENDPROG_OFF,
        DMA_DMACCR_WP_DEFAULT,
        DMA_DMACCR_REPEAT_ALWAYS,
        DMA_DMACCR_AUTOINIT_ON,
        DMA_DMACCR_EN_STOP,
        DMA_DMACCR_PRIO_HI,
        DMA_DMACCR_FS_ELEMENT,
        DMA_DMACCR_SYNC_REVT1         // Evento de RX da McBSP1
    ),
    // Interrupções de meio e fim de buffer (ping-pong)
    DMA_DMACICR_RMK(
        DMA_DMACICR_AERRIE_OFF,
        DMA_DMACICR_BLOCKIE_OFF,
        DMA_DMACICR_LASTIE_OFF,
        DMA_DMACICR_FRAMEIE_ON,       // Fim do buffer → processa PONG
        DMA_DMACICR_FIRSTHALFIE_ON,   // Meio do buffer → processa PING
        DMA_DMACICR_DROPIE_OFF,
        DMA_DMACICR_TIMEOUTIE_OFF
    ),
    (DMA_AdrPtr)0x5800,   // DRR1 da McBSP1 (recepção)
    0,
    (DMA_AdrPtr)0x0000,   // Destino: será RxBuffer em configAudioDma
    0,
    AUDIO_BUFFER_SIZE,    // # de elementos
    1,
    0,
    0,
    0,
    0
};

// =================== CONFIGURAÇÃO E CONTROLE ===================

void configAudioDma(void)
{
    Uint16 rxEventId, txEventId;
    int i;

    // Zera buffers para evitar lixo inicial
    for (i = 0; i < AUDIO_BUFFER_SIZE; i++) {
        RxBuffer[i] = 0;
        TxBuffer[i] = 0;
    }

    // --- Canal de TX: origem = TxBuffer ---
    // (endereços de DMA são em palavras → << 1)
    dmaTxConfig.dmacssal = (DMA_AdrPtr)(((Uint32)&TxBuffer) << 1);
    hDmaTx = DMA_open(DMA_CHA0, 0);
    DMA_config(hDmaTx, &dmaTxConfig);

    // --- Canal de RX: destino = RxBuffer ---
    dmaRxConfig.dmacdsal = (DMA_AdrPtr)(((Uint32)&RxBuffer) << 1);
    hDmaRx = DMA_open(DMA_CHA1, 0);
    DMA_config(hDmaRx, &dmaRxConfig);

    // === Configuração de interrupções ===
    CSL_init();
    IRQ_setVecs((Uint32)(&VECSTART));

    rxEventId = DMA_getEventId(hDmaRx);
    txEventId = DMA_getEventId(hDmaTx);

    IRQ_disable(rxEventId);
    IRQ_disable(txEventId);
    IRQ_clear(rxEventId);
    IRQ_clear(txEventId);

    // Usamos apenas a ISR de RX para processamento
    IRQ_plug(rxEventId, &dmaRxIsr);
    IRQ_plug(txEventId, &dmaTxIsr); // Tx vazia, mas registrada

    IRQ_enable(rxEventId);
    IRQ_globalEnable();

    dmaPingPongFlag = 0;
    currentEffect = EFFECT_LOOPBACK;
}

void startAudioDma(void)
{
    dmaPingPongFlag = 0;
    DMA_start(hDmaRx);
    DMA_start(hDmaTx);
}

void stopAudioDma(void)
{
    DMA_stop(hDmaRx);
    DMA_stop(hDmaTx);
}

// =================== PROCESSAMENTO (COM INICIALIZAÇÃO DINÂMICA) ===================

// ISR do DMA de recepção: escolhe PING/PONG e aplica efeitos
interrupt void dmaRxIsr(void)
{
    Uint16* pRx;
    Uint16* pTx;
    static Uint8 lastEffect = 0xFF;  // Valor inválido inicial

    // Verifica se o efeito mudou e precisa de reinicialização
    if(lastEffect != currentEffect) {
        // Inicializa o novo efeito
        initEffect(currentEffect);
        lastEffect = currentEffect;
    }

    if(dmaPingPongFlag == 0) {
        // Bloco PING (primeira metade)
        pRx = &RxBuffer[0];
        pTx = &TxBuffer[0];
        dmaPingPongFlag = 1;
    } else {
        // Bloco PONG (segunda metade)
        pRx = &RxBuffer[AUDIO_BLOCK_SIZE];
        pTx = &TxBuffer[AUDIO_BLOCK_SIZE];
        dmaPingPongFlag = 0;
    }

    // Aplica o efeito selecionado
    switch(currentEffect)
    {
        case EFFECT_LOOPBACK:
            processAudioLoopback(pRx, pTx);
            break;

        case EFFECT_FLANGER:
            processAudioFlanger(pRx, pTx);
            break;

        case EFFECT_TREMOLO:
            processAudioTremolo(pRx, pTx);
            break;

        default:
            processAudioLoopback(pRx, pTx);
            break;
    }
}

// ISR do DMA de transmissão: não precisamos fazer nada
interrupt void dmaTxIsr(void)
{
    // vazio
}