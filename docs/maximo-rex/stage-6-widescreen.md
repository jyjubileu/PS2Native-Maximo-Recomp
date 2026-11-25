# Etapa 6 — Apresentação e analógico em widescreen

O alvo da bancada é o PAL `SLES_517.59`, CRC `86E533E0`, com ELF e lista de patches 16:9. A branch parte da Etapa 5 em `1646f5b`.

## Como testar no jogo

Execute `scripts/start_maximo.ps1 -Profile 16x9 -BuildDirectory .\build\stage6-16x9`. Abra **F1** e selecione **Aplicar** depois de editar. As duas opções funcionam na próxima imagem ou amostra de entrada, respectivamente. Os valores iniciais preservam a apresentação e a curva da Etapa 5.

| Aba | Ajuste | Efeito |
| --- | --- | --- |
| Gráficos | **Composição entrelaçada → Campo atual** | Exibe apenas o campo atual no modo de quadro entrelaçado (`SMODE2.INT=FFMD=1`). **Mesclar campos** restabelece a média do campo atual e do anterior. |
| Controles | **Ganho esquerdo (PCSX2) → 133%** | Substitui a sensibilidade geral do analógico esquerdo pela escala de referência usada no DualShock 2 do PCSX2. **Geral** restabelece a sensibilidade que o perfil já tinha. |

`PS2_NO_DEINTERLACE` continua disponível como override da composição. A nova opção é salva como `current_field`. O ganho esquerdo é salvo como `pad.left_reference_gain`. A zona morta, a inversão e as entradas mapeadas continuam passando pelo processamento existente. Com 133%, a faixa parcial continua disponível e a saturação axial chega antes, como na referência. O analógico direito continua usando a sensibilidade geral.

## Evidência gráfica

Os quatro dumps PAL examinados registram só o circuito PCRTC 2 ativo e `SMODE2=3`. Em D3D12 a 4×, reexecutar um mesmo dump com a matriz de opções do PCSX2 produziu quatro pares de pixels idênticos para Anti-Desfoque. Desabilitar Deslocamento Entrelaçado alinhou verticalmente um dos campos. O melhor ajuste foi de quatro pixels a 4×, com desvio horizontal zero. A opção do PCSX2 atua no retângulo de destino durante a mesclagem. [Código do renderizador de referência](https://github.com/PCSX2/pcsx2/blob/v2.9.184/pcsx2/GS/Renderers/Common/GSRenderer.cpp), [versão executada](https://github.com/PCSX2/pcsx2/releases/tag/v2.9.186).

O PS2Native expande o campo para a altura de saída e pode fazer a média temporal pelo passe de apresentação. O A/B do quadro 10592, com o mesmo filtro e D3D12 a 4×, mediu um aumento de 10,7% na variação vertical de luminância ao exibir Campo atual. A saída do GS capturada por `--frames-dir` fica antes da composição. O ensaio de janela mede o que o passe de apresentação entregou. As bordas, a chuva e a neblina permanecem visíveis na cutscene ensaiada.

## Evidência de entrada

A gravação pós-host tem 252 quadros noroeste sustentados entre 9102 e 9353. As medianas de LX e LY são 41 e 33, com raio normalizado mediano de 1,008. Um replay sintético no PCSX2 reproduziu os bytes em um ponto da vila. Um segundo replay começou do mesmo estado e usou 12 e 2, equivalentes à escala 1,33 para uma amostra SDL compatível com a gravação. A captura passiva inicial foi idêntica nos dois replays. O segundo percurso avançou mais em relação ao baú da cena. [Processamento DualShock 2 na referência](https://github.com/PCSX2/pcsx2/blob/v2.9.184/pcsx2/SIO/Pad/PadDualshock2.cpp).

No PS2Native, a sonda SDL repetiu a mesma entrada nos dois modos. O quadro 5348 registrou 41 e 33 em Geral e 12 e 2 em 133%. A suíte SDL/SIO2 conferiu quatro direções principais, quatro diagonais, deflexão parcial e o eixo direito. O replay do perfil experimental reproduziu quatro imagens byte a byte a partir das amostras gravadas.

Os testes físicos novos no GameSir ficam para a rodada manual. A gravação disponível contém os bytes pós-host e permite ancorar a divergência de calibração sem inferir valores brutos exatos. A comparação de movimento no PCSX2 cobre a diagonal noroeste. As quatro diagonais foram exercitadas na saída SIO2. As comparações de vídeo sincronizam os casos dentro de cada emulador. O dump do PCSX2 e o F9 do PS2Native são cenas diferentes.
