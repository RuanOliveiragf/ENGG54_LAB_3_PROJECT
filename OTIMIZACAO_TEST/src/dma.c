//////////////////////////////////////////////////////////////////////////////
// dma.c - DMA de áudio com sistema modular de efeitos
//////////////////////////////////////////////////////////////////////////////

#include "ezdsp5502.h"
#include "csl.h"  
#include "ezdsp5502_mcbsp.h"
#include "csl_dma.h"
#include "csl_irq.h"
#include "dma.h"
#include "isr.h"
#include "effects_controller.h"
#include "flanger.h"
#include "tremolo.h"
#include "reverb.h"

// =================== VARIÁVEIS GLOBAIS ===================

// Handles para os canais de DMA
static DMA_Handle hDmaRx;
static DMA_Handle hDmaTx;

// Flag ping-pong: 0 = PING (primeira metade), 1 = PONG (segunda metade)
volatile Uint16 dmaPingPongFlag = 0;

// Buffers de entrada (mic) e saída (fone)
#pragma DATA_SECTION(RxBuffer, "dmaMem")
#pragma DATA_ALIGN(RxBuffer, 4096)
Uint16 RxBuffer[AUDIO_BUFFER_SIZE];

#pragma DATA_SECTION(TxBuffer, "dmaMem")
#pragma DATA_ALIGN(TxBuffer, 4096)
Uint16 TxBuffer[AUDIO_BUFFER_SIZE];

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
    Uint32 txAddr, rxAddr;

    // Zera buffers para evitar lixo inicial
    for (i = 0; i < AUDIO_BUFFER_SIZE; i++) {
        RxBuffer[i] = 0;
        TxBuffer[i] = 0;
    }

    txAddr = ((Uint32)&TxBuffer) << 1;
    rxAddr = ((Uint32)&RxBuffer) << 1;

    // 2. Configura TX (Origem = TxBuffer)
    dmaTxConfig.dmacssal = (DMA_AdrPtr)(txAddr & 0xFFFF); // Parte Baixa
    dmaTxConfig.dmacssau = (Uint16)(txAddr >> 16);        // Parte Alta (IMPORTANTE!)

    hDmaTx = DMA_open(DMA_CHA0, 0);
    DMA_config(hDmaTx, &dmaTxConfig);

    // 3. Configura RX (Destino = RxBuffer)
    dmaRxConfig.dmacdsal = (DMA_AdrPtr)(rxAddr & 0xFFFF); // Parte Baixa
    dmaRxConfig.dmacdsau = (Uint16)(rxAddr >> 16);        // Parte Alta (IMPORTANTE!)

    hDmaRx = DMA_open(DMA_CHA1, 0);
    DMA_config(hDmaRx, &dmaRxConfig);

    // === Configuração de interrupções ===
    // A tabela de vetores e as interrupções globais são configuradas em main.c.
    // Aqui apenas associamos os eventos de DMA às suas ISRs.
    rxEventId = DMA_getEventId(hDmaRx);
    txEventId = DMA_getEventId(hDmaTx);

    IRQ_disable(rxEventId);
    IRQ_disable(txEventId);
    IRQ_clear(rxEventId);
    IRQ_clear(txEventId);

    // ISRs de RX e TX
    IRQ_plug(rxEventId, &dmaRxIsr);
    IRQ_plug(txEventId, &dmaTxIsr); // Tx vazia, mas registrada

    IRQ_enable(rxEventId);
    IRQ_enable(txEventId);

    dmaPingPongFlag = 0;
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

// =================== PROCESSAMENTO DE EFEITOS ===================

// Processador principal de áudio (chama o efeito ativo)
static void processAudioBlock(Uint16* rxBlock, Uint16* txBlock, Uint16 size)
{
    Uint8 effect = getCurrentEffect();
    
    switch (effect) {
        case EFFECT_LOOPBACK:
            // Loopback simples - copia entrada para saída
            for (int i = 0; i < size; i++) {
                txBlock[i] = rxBlock[i];
            }
            break;
            
        case EFFECT_FLANGER:
            // Processa efeito Flanger
            processAudioFlanger(rxBlock, txBlock, size);
            break;
            
        case EFFECT_TREMOLO:
            // Processa efeito Tremolo
            processAudioTremolo(rxBlock, txBlock, size);
            break;
            
        case EFFECT_REVERB:
            // Processa efeito Reverb
            processAudioReverb(rxBlock, txBlock, size);
            break;
            
        default:
            // Fallback para loopback (nunca deveria acontecer)
            for (int i = 0; i < size; i++) {
                txBlock[i] = rxBlock[i];
            }
            break;
    }
}

// =================== ISRs DE DMA ===================

// ISR do DMA de recepção: escolhe PING/PONG e aplica efeitos
interrupt void dmaRxIsr(void)
{
    Uint16* pRx;
    Uint16* pTx;

    if (dmaPingPongFlag == 0) {
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

    // Processa o bloco com o efeito atual
    processAudioBlock(pRx, pTx, AUDIO_BLOCK_SIZE);
}

// ISR do DMA de transmissão: não precisamos fazer nada
interrupt void dmaTxIsr(void)
{
    // Vazio - a transmissão é automática pelo DMA
}