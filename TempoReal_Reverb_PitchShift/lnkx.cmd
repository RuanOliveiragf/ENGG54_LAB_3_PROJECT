-stack    01000      /* Primary stack size   */
-sysstack 0x1000      /* Secondary stack size */
-heap     0x1000      /* Heap area size       */

-c                    /* Use C linking conventions: auto-init vars at runtime */
-u _Reset             /* Force load of reset interrupt handler */

MEMORY
{
 PAGE 0:
  MMR    (RWIX): origin = 0x000000, length = 0x0000C0
  BTRSVD (RWIX): origin = 0x0000C0, length = 0x000240
  DARAM  (RWIX): origin = 0x000300, length = 0x00FB00  /* 64KB RAM Interna Rápida */
  VECS   (RWIX): origin = 0x00FE00, length = 0x000200

  /* CE0 é a SDRAM Externa (Grande e Lenta) */
  CE0          : origin = 0x010000, length = 0x3f0000
  CE1          : origin = 0x400000, length = 0x400000
  CE2          : origin = 0x800000, length = 0x400000
  CE3          : origin = 0xC00000, length = 0x3F8000
  PDROM   (RIX): origin = 0xFF8000, length = 0x008000

 PAGE 2:
  IOPORT (RWI) : origin = 0x000000, length = 0x020000
}

SECTIONS
{
   .text     >  DARAM align(32) fill = 20h { * (.text) }
   .stack    >  DARAM align(32) fill = 00h
   .sysstack >  DARAM align(32) fill = 00h
   .csldata  >  DARAM align(64) fill = 00h
   .data     >  DARAM align(32) fill = 00h
   .bss      >  DARAM align(32) fill = 00h
   .const    >  DARAM align(32) fill = 00h
   .sysmem   >  DARAM
   .switch   >  DARAM
   .cinit    >  DARAM
   .pinit    >  DARAM align(32) fill = 00h
   .cio      >  DARAM align(32) fill = 00h
   .args     >  DARAM align(32) fill = 00h
   .vectors: >  VECS

   /* Buffers do DMA devem ficar na RAM rápida interna */
   dmaMem    >  DARAM align(32) fill = 00h

   /* AQUI ESTA A CORREÇÃO DE MEMÓRIA: */
   /* Manda os buffers de efeito para a memória externa (CE0) */
   effectsMem > CE0

   .ioport   >  IOPORT PAGE 2
}
