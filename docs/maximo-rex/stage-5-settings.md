# Maximo Rex Settings — Etapa 5

## Iniciar o jogo

O launcher de PowerShell abre o perfil selecionado com o executável, o ELF e as opções de `runner.json`. O 4:3 usa o ELF PAL original `SLES_517.59`, CRC `86E533E0`. O 16:9 usa o ELF gerado com a seção `Widescreen 16:9` do PNACH e a lista de patches do perfil. Encerre o jogo e execute o launcher com o outro perfil para mudar a proporção.

```powershell
.\scripts\start_maximo.ps1 -Profile 4x3
.\scripts\start_maximo.ps1 -Profile 16x9
```

Por padrão, o launcher procura a build em `build/stage5-4x3` ou `build/stage5-16x9`. Passe `-BuildDirectory` para selecionar outro diretório gerado por `scripts/build_maximo.py`. Use `-QuickDirectory` para definir um diretório local de preferências, estados rápidos e relatórios.

## Navegação e atalhos

| Ação | Teclado | Gamepad |
| --- | --- | --- |
| Abrir ou fechar | **F1** | **Guide**. Abra também segurando **Select** e pressionando **Start**. |
| Trocar de aba | Tab ou Shift+Tab | LB ou RB |
| Percorrer opções | Cima e baixo | Direcional ou analógico esquerdo |
| Ajustar ou escolher | Esquerda, direita ou Enter | Direcional ou A |
| Voltar ou cancelar | Esc | B |
| Aplicar e voltar ao jogo | Ctrl+Enter | Y |

Com o mouse, clique nas abas, nas setas dos valores e nos botões do rodapé. A roda percorre listas longas. **Padrões** prepara a restauração de vídeo, teclado e gamepad. **Aplicar** grava as mudanças e volta ao jogo. **Cancelar** ou **F1** descarta o rascunho. O teclado e o gamepad também têm suas próprias ações de restauração na aba Controles.

F2 percorre os presets de apresentação da sessão. F5 salva no slot rápido, F8 carrega e F9 gera um relatório. F11 alterna o modo da janela na sessão. Os atalhos F5, F8 e F9 fecham o painel quando ele estiver aberto.

## Gráficos

| Opção | Comportamento |
| --- | --- |
| Renderizador | Automático, D3D12, Vulkan ou CPU na build Windows com WebGPU. Requer reinício. O cabeçalho mostra o renderizador que abriu, incluindo CPU quando a inicialização do GPU falha. |
| Resolução interna | 1× a 4× por GPU. CPU trabalha a 1×. Requer reinício. |
| Modo da janela | Janela redimensionável e tela cheia no modo desktop. Aplicação imediata. A linha Saída mostra a resolução física atual. |
| Sincronização vertical | Modo automático do WebGPU ou FIFO. O modo Imediato aparece ao percorrer as escolhas quando a superfície ativa o oferece. Requer reinício. |
| Apresentação | Presets do F2 para ampliação e contornos. Aplicação imediata. |
| Filtro de texto e arte 2D | Suave, Nítido ou Mais nítido. Atua sobre arte 2D ampliada pelo GPU. Requer reinício. |
| FPS e tempo por quadro | Mostra a cadência de apresentação medida pelo host. Aplicação imediata. |

`PS2_GS`, `PS2_GPU_BACKEND`, `PS2_SCALE`, `PS2_SHARP_2D`, `PS2_PRESENT_MODE` e `PS2_LOOK` definidos pelo ambiente mantêm precedência na inicialização. Use uma sessão PowerShell sem esses overrides para testar as escolhas persistentes do menu. A interface não muda `PS2_PACE` nem oferece limitação independente ou desbloqueio de quadros. O limite de escala da implementação é 4×.

## Controles e preferências

- **Teclado e mouse.** Selecione um botão do PS2 ou uma direção dos analógicos para capturar a próxima tecla ou botão. Os atalhos do host estão reservados. Se a entrada já pertence a outra ação, o menu indica a troca.
- **Gamepad.** São configuráveis os 16 botões do PS2. Gatilhos podem servir como entrada, com pressão analógica quando mapeados para L2 ou R2. O mapa de cada perfil vale para os dois controles SDL. A abertura do menu continua usando os botões físicos Guide ou Select e Start.
- **Analógicos e vibração.** Zona morta de 0% a 40%, sensibilidade de 50% a 150%, inversão individual em LX, LY, RX e RY, e opção de vibração para dispositivos que ofereçam esse recurso ao SDL.
- **Dispositivos.** As duas portas mostram o nome informado pelo SDL ou o estado Desconectado. A compatibilidade física depende do modo e do mapeamento reconhecidos por SDL_GameController. O launcher mantém suporte a `gamecontrollerdb.txt`.

`keys.ini` é a base de teclado e mouse, incluindo os layouts existentes. `rex-settings.ini` guarda opções de vídeo, seleção de layout padrão, ajustes do gamepad e os remapeamentos feitos pelo painel. Ele fica na pasta dos atalhos rápidos do perfil. A atualização usa bloqueio de escrita e substituição atômica. Valores inválidos são registrados e descartados, e o runtime usa as preferências válidas restantes e os padrões. O menu preserva `keys.ini`, e não há migração do arquivo antigo.

## Implementação e regressões

O painel RGBA em `host/quickmenu.cpp` é composto pelo WebGPU ou pelo SDL. A pausa continua no limite de quadro do Quick Menu. Enquanto o menu está aberto ou a janela perdeu o foco, o host entrega entradas neutras. Após fechar ou recuperar o foco, o host aguarda o retorno dos botões e eixos ao repouso. A gravação e o replay continuam consumindo as amostras do PS2 depois da leitura do host.

Em uma build SDL, compile o alvo `ps2padtest` e execute o teste convencional e a variante `--settings` com um diretório de teste novo. O segundo caso exercita edição, conflitos, cancelamento, foco, calibração e persistência através da janela SDL e do SIO2. `scripts/check_vi_branch.py` mantém a validação sintética de VIF/VU e a comparação interpretada/AOT/Wasm. A extensão `<quadro> key <scancode>` em `PS2_PAD_SCRIPT` permite repetir os atalhos do host em uma execução local do jogo, usando a mesma fila SDL do produto.
