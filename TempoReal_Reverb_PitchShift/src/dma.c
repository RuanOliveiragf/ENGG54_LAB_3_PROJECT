/////////////////////////////////////////////////////////////////////////////
// dma.c - DMA de Ã¡udio em loopback (MIC â†’ FONE) usando ping-pong
//////////////////////////////////////////////////////////////////////////////

#include "ezdsp5502.h"
#include "csl.h"  
#include "ezdsp5502_mcbsp.h"
#include "csl_dma.h"
#include "csl_irq.h"
#include "dma.h"
#include "isr.h"
#include "effects.h"  // Inclui as funÃ§Ãµes de efeitos

// =================== VARIÃVEIS GLOBAIS ===================

// Handles para os canais de DMA
static DMA_Handle hDmaRx;   // RecepÃ§Ã£o (McBSP â†’ memÃ³ria)
static DMA_Handle hDmaTx;   // TransmissÃ£o (memÃ³ria â†’ McBSP)

// Flag ping-pong: 0 = PING (primeira metade), 1 = PONG (segunda metade)
volatile Uint16 dmaPingPongFlag = 0;

// Buffers de entrada (mic) e saída (fone)
#pragma DATA_SECTION(RxBuffer, "dmaMem")
#pragma DATA_ALIGN(RxBuffer, 4096)
Uint16 RxBuffer[AUDIO_BUFFER_SIZE];

#pragma DATA_SECTION(TxBuffer, "dmaMem")
#pragma DATA_ALIGN(TxBuffer, 4096)
Uint16 TxBuffer[AUDIO_BUFFER_SIZE];

// =================== CONFIGURAÃ‡ÃƒO DMA TX ===================

static DMA_Config dmaTxConfig = {
    DMA_DMACSDP_RMK(
        DMA_DMACSDP_DSTBEN_NOBURST,   // Destination burst
        DMA_DMACSDP_DSTPACK_OFF,      // Destination packing
        DMA_DMACSDP_DST_PERIPH,       // Destino: perifÃ©rico (McBSP1 DXR)
        DMA_DMACSDP_SRCBEN_NOBURST,   // Source burst
        DMA_DMACSDP_SRCPACK_OFF,      // Source packing
        DMA_DMACSDP_SRC_DARAMPORT1,   // Origem: DARAM (memÃ³ria)
        DMA_DMACSDP_DATATYPE_16BIT    // 16 bits
    ),
    DMA_DMACCR_RMK(
        DMA_DMACCR_DSTAMODE_CONST,    // EndereÃ§o destino fixo (DXR)
        DMA_DMACCR_SRCAMODE_POSTINC,  // Origem incrementa (buffer)
        DMA_DMACCR_ENDPROG_OFF,
        DMA_DMACCR_WP_DEFAULT,
        DMA_DMACCR_REPEAT_ALWAYS,     // Loop infinito
        DMA_DMACCR_AUTOINIT_ON,       // Auto-init
        DMA_DMACCR_EN_STOP,           // ComeÃ§a parado
        DMA_DMACCR_PRIO_HI,           // Alta prioridade
        DMA_DMACCR_FS_ELEMENT,        // Sync por elemento
        DMA_DMACCR_SYNC_XEVT1         // Evento de TX da McBSP1
    ),
    DMA_DMACICR_RMK(
        DMA_DMACICR_AERRIE_OFF,
        DMA_DMACICR_BLOCKIE_OFF,
        DMA_DMACICR_LASTIE_OFF,
        DMA_DMACICR_FRAMEIE_OFF,      // Tx nÃ£o precisa de interrupÃ§Ã£o
        DMA_DMACICR_FIRSTHALFIE_OFF,
        DMA_DMACICR_DROPIE_OFF,
        DMA_DMACICR_TIMEOUTIE_OFF
    ),
    (DMA_AdrPtr)0x0000,   // Origem: serÃ¡ TxBuffer em configAudioDma
    0,
    (DMA_AdrPtr)0x5804,   // DXR1 da McBSP1 (transmissÃ£o)
    0,
    AUDIO_BUFFER_SIZE,    // # de elementos de 16 bits
    1,                    // 1 frame
    0,
    0,
    0,
    0
};

// =================== CONFIGURAÃ‡ÃƒO DMA RX ===================

static DMA_Config dmaRxConfig = {
    DMA_DMACSDP_RMK(
        DMA_DMACSDP_DSTBEN_NOBURST,
        DMA_DMACSDP_DSTPACK_OFF,
        DMA_DMACSDP_DST_DARAMPORT1,   // Destino: memÃ³ria
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
    // InterrupÃ§Ãµes de meio e fim de buffer (ping-pong)
    DMA_DMACICR_RMK(
        DMA_DMACICR_AERRIE_OFF,
        DMA_DMACICR_BLOCKIE_OFF,
        DMA_DMACICR_LASTIE_OFF,
        DMA_DMACICR_FRAMEIE_ON,       // Fim do buffer â†’ processa PONG
        DMA_DMACICR_FIRSTHALFIE_ON,   // Meio do buffer â†’ processa PING
        DMA_DMACICR_DROPIE_OFF,
        DMA_DMACICR_TIMEOUTIE_OFF
    ),
    (DMA_AdrPtr)0x5800,   // DRR1 da McBSP1 (recepÃ§Ã£o)
    0,
    (DMA_AdrPtr)0x0000,   // Destino: serÃ¡ RxBuffer em configAudioDma
    0,
    AUDIO_BUFFER_SIZE,    // # de elementos
    1,
    0,
    0,
    0,
    0
};

// =================== CONFIGURAÃ‡ÃƒO E CONTROLE ===================

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

    // === ConfiguraÃ§Ã£o de interrupÃ§Ãµes ===
    // A tabela de vetores e as interrupÃ§Ãµes globais sÃ£o configuradas em main.c.
    // Aqui apenas associamos os eventos de DMA Ã s suas ISRs.
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
    //currentEffect = EFFECT_LOOPBACK;
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

// =================== PROCESSAMENTO (LOOPBACK/FLANGER/TREMOLO/REVERB) ===================

// ISR do DMA de recepÃ§Ã£o: escolhe PING/PONG e aplica efeitos
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

    processAudioReverbWrapper(pRx, pTx);

}

// ISR do DMA de transmissÃ£o: nÃ£o precisamos fazer nada
interrupt void dmaTxIsr(void)
{
    // vazio
}


