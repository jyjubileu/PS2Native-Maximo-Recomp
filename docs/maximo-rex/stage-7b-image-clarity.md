# Etapa 7B — Nitidez de apresentação

O perfil experimental usa o PAL SLES_517.59, CRC original 86E533E0, em 16:9. A build de teste abre em D3D12 a 4× com as preferências copiadas da Etapa 7A. As duas opções novas começam desativadas.

## Usar pelo F1

| Configuração | Opções |
| --- | --- |
| Gráficos → Composição entrelaçada | Mesclar campos, Campo atual, Campos alinhados, Alinhados adaptativos |
| Gráficos → Redução da janela | Suave, Por área |

Escolha as opções e confirme em **Aplicar**. **Campos alinhados** com redução **Suave** recupera a apresentação da Etapa 7A para esse perfil. A composição adaptativa requer apresentação WebGPU com espaço para três campos. A redução por área é independente da composição.

## Composição dos campos

O compositor identifica a paridade e recentra os campos compatíveis como na Etapa 7A. Com três campos contíguos, estima o movimento pela diferença RGB entre o campo atual e o de dois VSyncs antes. Ambos têm a mesma paridade. Até seis códigos de diferença mantém a média com o campo oposto. Entre seis e 24 códigos, reduz suavemente o peso do campo oposto. Com histórico incompatível, usa o comportamento de Campos alinhados. D3D12 e Vulkan executam o mesmo shader.

O terceiro campo reserva 32 MiB adicionais a 4× nas GPUs que suportam o buffer, inclusive quando a opção está desligada. Essa reserva permite trocar o modo durante o jogo.

## Redução para a janela

Por área integra a cobertura de cada texel no pixel de destino com pesos não negativos. O modo atua no caminho de redução existente quando a proporção de origem e destino de cada eixo fica entre 1 e 8. Fora dessas condições, o compositor segue o caminho anterior. A saída 1:1 preserva os pixels e o preset de ampliação continua separado. A área evita valores além dos extremos da entrada. Em alguns padrões sintéticos acima de Nyquist, apresentou mais alias que o filtro Suave.

## Verificação

No trecho sincronizado da espada, 12 campos em D3D12 a 4× para janela 960 × 540 deram 3,45 px de largura mediana 10–90% com Campos alinhados e 1,40 px com Alinhados adaptativos. A troca isolada para Por área mediu 3,44 px. Na vila pausada, os 14 campos do trecho estável foram idênticos em cada variante com correção de fase. O reprodutor experimental conserva o mesmo ELF e PNACH 16:9.
