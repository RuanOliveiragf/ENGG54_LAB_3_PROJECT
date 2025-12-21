//////////////////////////////////////////////////////////////////////////////
// * File name: oled.c
// *                                                                          
// * Description:  OSD9616 OLED functions.
// *                                                                          
// * Copyright (C) 2011 Texas Instruments Incorporated - http://www.ti.com/ 
// * Copyright (C) 2011 Spectrum Digital, Incorporated
// *                                                                          
// *                                                                          
// *  Redistribution and use in source and binary forms, with or without      
// *  modification, are permitted provided that the following conditions      
// *  are met:                                                                
// *                                                                          
// *    Redistributions of source code must retain the above copyright        
// *    notice, this list of conditions and the following disclaimer.         
// *                                                                          
// *    Redistributions in binary form must reproduce the above copyright     
// *    notice, this list of conditions and the following disclaimer in the   
// *    documentation and/or other materials provided with the                
// *    distribution.                                                         
// *                                                                          
// *    Neither the name of Texas Instruments Incorporated nor the names of   
// *    its contributors may be used to endorse or promote products derived   
// *    from this software without specific prior written permission.         
// *                                                                          
// *  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS     
// *  "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT       
// *  LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR   
// *  PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
// *  OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,   
// *  SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT        
// *  LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,   
// *  DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY   
// *  THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT     
// *  (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE   
// *  OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.    
// *                                                                          
//////////////////////////////////////////////////////////////////////////////
 
#include"ezdsp5502.h"
#include"lcd.h"

/*
 *  Int16 oled_start()
 *
 *      Start the OSD9616 display
 */
Int16 oled_start()
{
    Int16 i;
    int page;  // <-- CORREÇÃO: Declarada corretamente
    Uint16 cmd[10];              // For multibyte commands
    
    /* Initialize  Display */
    osd9616_init( );
    
    osd9616_send(0x00,0x2e);     // Deactivate Scrolling
    
    // Limpa toda a tela (todas as páginas)
    for (page = 0; page < 8; page++) {  // <-- CORREÇÃO: 'page' declarada
        osd9616_send(0x00,0x00);     // low column = 0
        osd9616_send(0x00,0x10);     // high column = 0
        osd9616_send(0x00,0xb0 + page); // page 0 a 7

        for(i=0;i<128;i++) {
            osd9616_send(0x40,0x00); // limpa tudo
        }
    }
    
    // Posiciona na página 0 (primeira linha) para escrever
    osd9616_send(0x00,0x00);
    osd9616_send(0x00,0x10);
    osd9616_send(0x00,0xb0);
    
    // Mostra mensagem inicial
    oled_show_effect_name("LOOPBACK");
    
    return 0;
}

// ---------------------------------------------------------------------------
// Funções auxiliares para mostrar o nome do efeito no OLED
// ---------------------------------------------------------------------------

// Limpa totalmente a pagina 0 (primeira linha)
static void oled_clear_page0(void)
{
    int i;
    osd9616_send(0x00, 0x00);      // low column = 0
    osd9616_send(0x00, 0x10);      // high column = 0
    osd9616_send(0x00, 0xB0 + 0);  // page 0

    for (i = 0; i < 128; i++)
    {
        osd9616_send(0x40, 0x00);  // limpa com zeros
    }

    // reposiciona cursor no inicio da pagina 0
    osd9616_send(0x00, 0x00);
    osd9616_send(0x00, 0x10);
    osd9616_send(0x00, 0xB0 + 0);
}

// Limpa todo o display
void oled_clear_all(void)
{
    int i, page;

    for (page = 0; page < 8; page++) {
        osd9616_send(0x00, 0x00);      // low column = 0
        osd9616_send(0x00, 0x10);      // high column = 0
        osd9616_send(0x00, 0xB0 + page); // página atual

        for (i = 0; i < 128; i++) {
            osd9616_send(0x40, 0x00);  // limpa com zeros
        }
    }

    // Reposiciona na página 0
    osd9616_send(0x00, 0x00);
    osd9616_send(0x00, 0x10);
    osd9616_send(0x00, 0xB0);
}

// Desenha um caractere (A–Z, espaco) usando printLetter
static void oled_print_char(char c)
{
    switch (c)
    {
        // --- LETRAS  ---
        case 'A': printLetter(0x7C,0x12,0x12,0x7C); break; 
        case 'B': printLetter(0x36,0x49,0x49,0x7F); break; 
        case 'C': printLetter(0x22,0x41,0x41,0x3E); break; 
        case 'D': printLetter(0x3E,0x41,0x41,0x7F); break; 
        case 'E': printLetter(0x41,0x49,0x49,0x7F); break; 
        case 'F': printLetter(0x01,0x09,0x09,0x7F); break; 
        case 'G': printLetter(0x3A,0x49,0x41,0x3E); break; 
        case 'H': printLetter(0x7F,0x08,0x08,0x7F); break; 
        case 'I': printLetter(0x41,0x7F,0x41,0x00); break; 
        case 'J': printLetter(0x3F,0x40,0x40,0x20); break; 
        case 'K': printLetter(0x63,0x14,0x08,0x7F); break; 
        case 'L': printLetter(0x40,0x40,0x40,0x7F); break; 
        case 'M': printLetter(0x77,0x08,0x08,0x77); break; 
        case 'N': printLetter(0x7F,0x04,0x02,0x7F); break; 
        case 'O': printLetter(0x3E,0x41,0x41,0x3E); break; 
        case 'P': printLetter(0x06,0x09,0x09,0x7F); break; 
        case 'Q': printLetter(0x3E,0x51,0x41,0x3E); break; 
        case 'R': printLetter(0x76,0x09,0x09,0x7F); break; 
        case 'S': printLetter(0x32,0x49,0x49,0x26); break; 
        case 'T': printLetter(0x01,0x7F,0x01,0x01); break; 
        case 'U': printLetter(0x3F,0x40,0x40,0x3F); break; 
        case 'V': printLetter(0x0F,0x30,0x30,0x0F); break; 
        case 'W': printLetter(0x3F,0x20,0x20,0x3F); break; 
        case 'X': printLetter(0x63,0x14,0x14,0x63); break; 
        case 'Y': printLetter(0x0F,0x70,0x08,0x07); break; 
        case 'Z': printLetter(0x45,0x49,0x51,0x61); break; 

        // --- NÚMEROS ---
        case '0': printLetter(0x3E,0x41,0x41,0x3E); break; 
        case '1': printLetter(0x40,0x7F,0x42,0x00); break; 
        case '2': printLetter(0x46,0x49,0x51,0x62); break; 
        case '3': printLetter(0x36,0x49,0x49,0x22); break; 
        case '4': printLetter(0x7F,0x12,0x14,0x18); break; 
        case '5': printLetter(0x39,0x45,0x45,0x27); break;
        case '6': printLetter(0x30,0x49,0x4A,0x3C); break; 
        case '7': printLetter(0x07,0x09,0x71,0x01); break; 
        case '8': printLetter(0x36,0x49,0x49,0x36); break; 
        case '9': printLetter(0x3E,0x49,0x49,0x06); break; 

        // --- CARACTERES ESPECIAIS  ---
        case '+': printLetter(0x08,0x3E,0x08,0x08); break; 
        case '-': printLetter(0x08,0x08,0x08,0x08); break; 
        case '=': printLetter(0x14,0x14,0x14,0x14); break; 

        // Espaço
        case ' ':
            osd9616_send(0x40, 0x00);
            osd9616_send(0x40, 0x00);
            osd9616_send(0x40, 0x00);
            osd9616_send(0x40, 0x00);
            osd9616_send(0x40, 0x00);
            break;

        default:
            // Caractere desconhecido (imprime espaço)
            osd9616_send(0x40, 0x00);
            osd9616_send(0x40, 0x00);
            osd9616_send(0x40, 0x00);
            osd9616_send(0x40, 0x00);
            osd9616_send(0x40, 0x00);
            break;
    }
}

// Funcao publica: mostra o nome do efeito na primeira linha
void oled_show_effect_name(const char* name)
{
    int i;
    int len = 0;

    if (name == NULL) return;

    // 1. Calcula o tamanho da string (limitado a 16 caracteres para segurança)
    while (name[len] != '\0' && len < 16) {
        len++;
    }

    // Limpa a linha onde vamos escrever
    oled_clear_page0();

    // 2. Percorre o texto do FINAL para o INÍCIO
    for (i = len - 1; i >= 0; i--)
    {
        char c = name[i];

        // Garante maiusculo
        if (c >= 'a' && c <= 'z')
            c = c - 'a' + 'A';

        oled_print_char(c);
    }
}

// Mostra o nome correspondente ao passo de efeito (effectStep da main)
void oled_show_effect_step_name(int step)
{
    const char* name;

    switch (step)
    {
        case 0:  name = "LOOPBACK";       break;
        case 1:  name = "REVERB HALL";    break;
        case 2:  name = "REVERB ROOM";    break;
        case 3:  name = "REV STAGE + B";  break;
        case 4:  name = "REV STAGE + D";  break;
        case 5:  name = "REV STAGE + F";  break;
        case 6:  name = "REV STAGE + GB"; break;
        case 7:  name = "FLANGER";        break;
        case 8:  name = "TREMOLO";        break;
        default: name = "LOOPBACK";       break;
    }

    oled_show_effect_name(name);
}
