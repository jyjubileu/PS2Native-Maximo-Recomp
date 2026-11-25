# Etapa 7A — Alinhamento de campos

O alvo é o PAL `SLES_517.59`, CRC original `86E533E0`, com o perfil 16:9 e Direct3D 12 a 4×. A mudança é opt-in e funciona na apresentação WebGPU.

## Comparar no jogo

Abra o launcher do perfil widescreen com `.\scripts\start_maximo.ps1 -Profile 16x9 -BuildDirectory .\build\stage7a-16x9`. No F1, em **Gráficos → Composição entrelaçada**, escolha o modo e pressione **Aplicar**.

| Modo | Comportamento |
| --- | --- |
| Mesclar campos | Média dos campos na mesma coordenada da imagem. É o padrão. |
| Campo atual | Exibe somente o campo atual. Expõe a diferença de fase vertical entre campos. |
| Campos alinhados | Registra as duas paridades no mesmo centro vertical e depois faz a mesma média temporal. |

O terceiro modo salva `align_fields = on`. O arquivo anterior com `current_field` continua válido. `PS2_NO_DEINTERLACE` mantém a precedência. Em apresentação SDL/CPU, ficam disponíveis os dois modos históricos.

## Funcionamento

O par de apresentação recebe o campo e o número do quadro junto dos registros de saída. Em modo de quadro entrelaçado com um único circuito, os campos consecutivos de paridade oposta usam endereços de origem compatíveis. Uma virada de página de cor é permitida. O compositor amostra cada campo deslocado em meia linha em direções opostas. Em 4× isso corresponde a duas amostras para cada lado. A filtragem de apresentação é aplicada depois. Quando o par está incompleto, a opção usa a imagem atual até ter histórico compatível.

Nos estados PAL medidos, o erro mediano de registro entre campos na vila pausada caiu de 7,752 para 0,297 em luminância de 8 bits após um deslocamento de quatro amostras. Na apresentação alinhada os quadros estáticos permaneceram idênticos durante a alternância. Na cutscene em movimento, o erro médio entre quadros consecutivos foi 2,002 com campos alinhados e 2,001 na mesclagem original. O efeito foi aferido numa região fixa da mesma cena, incluindo 21 amostras de cada modo. A média temporal ainda pode suavizar movimento real.

A correção de alinhamento foi testada no compositor. A resolução interna, o rasterizador, o patch widescreen e os tempos do emulador mantêm o comportamento anterior. Os testes em dois enquadramentos de gameplay e uma cutscene abrangem Oakmed Hollows.
