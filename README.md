# Processador de Efeitos de Áudio em Tempo Real - DSP TMS320C5502
- [Processador de Efeitos de Áudio em Tempo Real - DSP TMS320C5502](#processador-de-efeitos-de-áudio-em-tempo-real---dsp-tms320c5502)
  - [📋 Funcionalidades Principais](#-funcionalidades-principais)
  - [🛠 Hardware e Software](#-hardware-e-software)
  - [📂 Estrutura do Repositório](#-estrutura-do-repositório)
  - [📖 Manual de Uso](#-manual-de-uso)
  - [🚀 Como Compilar e Executar](#-como-compilar-e-executar)
  - [⚙️ Detalhes de Implementação](#️-detalhes-de-implementação)

Este projeto implementa um sistema de processamento de áudio em tempo real utilizando o kit de desenvolvimento eZdsp TMS320C5502. O sistema é capaz de aplicar diversos efeitos de áudio (*Flanger*, *Tremolo*, *Reverb* e *Pitch Shifters*) controlados via botões físicos, com *feedback* visual através de LEDs e display OLED.

## 📋 Funcionalidades Principais
- **Processamento em Tempo Real:** Utilização de DMA e McBSP para baixa latência.

- **Múltiplos Efeitos:**
    - ***Loopback:*** Passagem direta do áudio (*Bypass*).
    - ***Reverb:*** Implementação com múltiplos *presets* (*Hall*, *Room*, *Stage*).
    - ***Pitch Shift:*** Alteração de tom integrado aos *presets* de Reverb.
    - ***Flanger:*** Efeito de modulação de *delay*.
    - ***Tremolo:*** Efeito de modulação de amplitude.

- **Interface de Usuário:**
    - Alternância de efeitos via botões (*Push Buttons*).
    - Display OLED para exibição do status/nome do efeito.
    - *Feedback* luminoso via LEDs do kit.

## 🛠 Hardware e Software
- **Plataforma:** Spectrum Digital eZdsp TMS320C5502.
- **Codec de Áudio:** AIC3204.
- **IDE:** Code Composer Studio (CCS).
- **Linguagem:** C e Assembly.

## 📂 Estrutura do Repositório
- ```Final_Project_Pro_MAX/```: Versão final e mais completa do projeto, integrando todos os efeitos e interface.
- ```TempoReal_.../```: Versões de desenvolvimento incremental e testes de tempo real.
- ```Efeitos_Offline/```: Implementações de teste dos algoritmos (Flanger, Tremolo) para validação em arquivos .wav ou .pcm no PC antes da implementação embarcada.
- ```OTIMIZACAO_.../```: Testes de otimização de código e presets.

## 📖 Manual de Uso
O controle do sistema é realizado através dos botões presentes na placa eZdsp. Abaixo está o mapeamento das funções conforme programado no ```main.c```.

**Controles Físicos**
| Botão    | Ação             | Descrição   |
| -------- | -----            | ----------- |
| SW1      | Mudar Efeito     | Alterna ciclicamente entre os 9 modos de operação disponíveis.     |
| SW2      | Ajustar LEDs     | Altera a frequência do timer que controla o padrão de piscagem dos LEDs (*feedback* visual de operação).        |

Ao pressionar o botão SW1, o sistema avança para o próximo efeito na seguinte ordem:
  1. ***LOOPBACK:*** Áudio original sem processamento.
  2. ***REVERB (Preset HALL):*** Reverb amplo, simulando um salão de concertos.
  3. ***REVERB (Preset ROOM 2):*** Reverb curto, simulando uma sala menor.
  4. ***REVERB STAGE + PITCH (Si/B):*** Reverb de palco com Pitch Shift ajustado para ~493Hz.
  5. ***REVERB STAGE + PITCH (Ré/D):*** Reverb de palco com Pitch Shift ajustado para ~293Hz.
  6. ***REVERB STAGE + PITCH (Fá/F):*** Reverb de palco com Pitch Shift ajustado para ~349Hz.
  7. ***REVERB STAGE + PITCH (Sol b/Gb):*** Reverb de palco com Pitch Shift ajustado para ~369Hz.
  8. ***FLANGER:*** Efeito de atraso modulado.
  9. ***TREMOLO:*** Variação cíclica de volume.

> A frequência base utilizada para os *Pitch Shifters* foi 261.63Hz (Dó/A).
 
> Após o item 9, o sistema retorna ao item 1.

**Feedback Visual**
- **OLED:** O nome do efeito atual e/ou passo do efeito é exibido no *display*.
- **LEDs:** Em repouso, os LEDs executam um padrão sequencial controlado pelo *Timer* (modificável via SW0).
 > Tentamos, ao trocar de efeito, um dos LEDs (LED0 a LED3) pisca brevemente para confirmar a transição, mas infelizmente não foi possível 

## 🚀 Como Compilar e Executar
1. Abra o Code Composer Studio (CCS).
2. Vá em File -> Import -> CCS Projects.
3. Selecione a pasta Final_Project_Pro_MAX.
4. Certifique-se de que a configuração de Target está correta para o eZdsp5502.
5. Compile o projeto (Build Project).
6. Inicie a sessão de Debug, conecte-se ao alvo e carregue o programa (.out).
7. Conecte uma fonte de áudio na entrada LINE IN e fones de ouvido/caixas na saída LINE OUT.
8. Execute o programa (Resume/Run).

## ⚙️ Detalhes de Implementação
- **Controlador de Efeitos:** A lógica de troca de contexto dos efeitos é gerenciada por ```effects_controller.c```, que garante a inicialização e limpeza de buffers ao alternar entre algoritmos complexos (como o Flanger e Reverb).
- ***Pitch Shift:*** Implementado no domínio do tempo, ativado condicionalmente junto com *presets* específicos de Reverb.
- **DMA (*Direct Memory Access*):** O áudio é transferido entre o Codec e a memória via DMA (*Ping-Pong buffers*) para liberar a CPU para o processamento matemático dos efeitos.
- **Memórias Externas (CEx):** Uma das principais dificuldades técnicas deste projeto foi a limitação da memória interna (DARAM) do DSP TMS320C5502, restrita a 64KB para dados e programa. Para contornar isso, utilizou-se a interface de memória externa (CE0) através do mapeamento de uma seção exclusiva no arquivo *linker* (```lnkx.cmd```), denominada ```effectsMem```. Essa abordagem liberou a DARAM para instruções críticas de tempo real, alocando os grandes *buffers* de áudio na memória externa.

---

Projeto desenvolvido pelos alunos Eduardo Vitor, Fábio Miguel, Felipe Trebino, Ruan Oliveira e Tiago Luigi para a disciplina ENGG54 - Laboratório Integrado III-A  (Laboratório de Processamento Digital de Sinais).
> **Observação:** "A diferença entre a civilização moderna e a barbárie primitiva não é a eletricidade, é o controle de versão; sem criar esse GitHub, somos apenas neandertais jogando arquivos .zip uns nos outros e rezando para que nada quebre."
