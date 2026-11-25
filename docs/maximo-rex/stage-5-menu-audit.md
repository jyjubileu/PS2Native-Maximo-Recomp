# Etapa 5 — Auditoria da interface do host

Referência funcional `maximo-rex-stage4-candidate-1` (`b2bbe57`). Esta auditoria precede as alterações do menu.

## Arquitetura encontrada

| Área | Implementação no candidato |
| --- | --- |
| Pausa | `host/window.cpp` abre o Quick Menu com F1, Guide ou Select + Start. `on_vblank` e `hold_for_menu` mantêm o jogo parado na borda de quadro, com reancoragem do áudio pelo `SoundPipe`. |
| Composição | `host/quickmenu.cpp` desenha a interface em RGBA com fonte própria. A janela compõe a imagem em uma textura SDL ou no passe de apresentação WebGPU. Quadros gravados pelo GS não incluem a interface. |
| Entradas | SDL2 entrega eventos da janela e amostras dos dispositivos. `keymap` resolve teclado e mouse; `poll_port` converte o controle para os botões e eixos do PS2. O replay trabalha com essa amostra no runtime. |
| Foco | O menu consome eventos SDL e desativa o mouse relativo. A liberação após fechar leva em conta os botões digitais, enquanto os analógicos e pulsos do mouse também precisam ser considerados. |
| Preferências | `keys.ini` fica no diretório dos atalhos, junto ao cartão salvo por padrão. O arquivo suporta layouts personalizados, ajuste visual, escala, filtro 2D e `pace`. F2 e as opções antigas do menu escrevem nesse arquivo. |
| Atalhos | F1 abre e fecha. F2 percorre os presets de apresentação. F5 salva, F8 carrega e F9 gera um relatório. F11 troca o modo da janela. |
| Renderização | `PS2_GS` escolhe a implementação. WebGPU oferece D3D12 e Vulkan no Windows, com escala interna de 1–4×, definida na inicialização. O caminho CPU usa resolução interna nativa. |
| Apresentação | SDL já cria uma janela redimensionável e suporta tela cheia no modo desktop. WebGPU escolhe o modo de apresentação entre os oferecidos pela superfície. `PS2_PACE` também afeta o ritmo emulado. |
| Proporção | `--wide` acompanha o ELF widescreen modificado. O launcher 4:3 usa o ELF original. A proporção precisa ser escolhida com o ELF correspondente no launcher. |

## Escopo incremental

1. Reutilizar a pausa, o canvas e os caminhos de composição do Quick Menu para o painel modal. Manter seus atalhos e bloquear entradas mantidas até voltarem ao neutro.
2. Plugar modos de vídeo suportados, deixando escolhas do GS para a próxima inicialização. Mostrar o perfil ativo na interface e selecionar entre builds no launcher. Avaliar sincronização conforme as capacidades reais da superfície.
3. Reutilizar o modelo de teclado e a API SDL_GameController para remapeamento. Reservar F1 e a abertura física do menu. Preservar as amostras de entrada gravadas pela emulação.
4. Manter `keys.ini` como base e criar preferências separadas, com escrita atômica no perfil. Exercitar carregamento com valores válidos, inválidos e overrides do ambiente.
5. Construir os dois perfis do PAL `SLES_517.59`, executar os testes sintéticos e comparar cenas equivalentes da Etapa 4.

O painel e as preferências pertencem ao host, que atende qualquer jogo com entrada em janela. O nome Maximo Rex e a separação dos ELFs 4:3 e 16:9 pertencem à apresentação e ao launcher. A implementação VI Branch Timing permanece na referência funcional.
