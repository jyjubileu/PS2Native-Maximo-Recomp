# Fontes da interface medieval

Os três PNGs fornecidos pelo usuário serviram como referência de forma, cor e composição. As texturas de entrega foram produzidas para o overlay do Maximo Rex. Os componentes não incluem quadros do jogo, rótulos, valores de configuração ou glifos.

## Regenerar no Windows

Quem tem os dois recortes-mestre no acervo local pode executar o gerador com PowerShell 7 a partir de `assets/ui/maximo_f1`. O checkpoint inclui os 18 PNGs prontos usados pelo F1.

```powershell
pwsh -NoProfile -File .\source\Generate.ps1
```

`BuildMedieval.cs` define contornos, os acabamentos metálicos, a textura do couro e a montagem dos componentes. Os arquivos `crest-master.png` e `corner-master.png` são os recortes-mestre com alfa usados pelo gerador. O script grava 18 PNGs RGBA, `manifest.json` e `regions.inc` no diretório acima de `source`. Para alterar os cortes, edite as dimensões e margens de `Parts` e regenere os arquivos.

O manifesto registra as margens na ordem esquerda, superior, direita e inferior. A composição redimensiona os cantos proporcionalmente à interface e estende as demais regiões. Uma margem nula em um eixo indica que o componente ocupa toda a região nesse eixo. Margens totalmente nulas são usadas para imagens de proporção fixa, como as setas e o brasão. O host desenha o halo antes da linha selecionada e rasteriza o texto UTF-8 em pixels da janela com a fonte Georgia do Windows.

## Ornamentos no ImageGen

Modo nativo, com `transparent_background: true`. Os dois recortes-mestre ficam no acervo local. A regeneração na bancada reproduziu os 18 PNGs do checkpoint byte a byte.

### Brasão

> Use case: ui-mockup. Generate one game UI ornament for the approved Maximo Rex medieval settings theme described by the attached references. Output one isolated circular heraldic medallion on genuine transparency, front orthographic, ornament occupies almost all the square with a small transparent safety margin. Dark brown antiqued leather-black interior, warm finely scratched aged bronze, raised gold relief of a heraldic lion rampant in readable silhouette, antique polished gold bevel on concentric ring rim. Lion face and mane clear rather than fantastical, elegant handcrafted fantasy RPG interface. Strong warm specular bands from upper left, deep outer bronze underside, detailed antique metallurgy, medium contrast to match the brown and bright amber reference atlases. This is a production cutout to composite onto a UI header. No background, no drop shadow beyond the medallion, no checker pattern, no words, letters, numbers, labels, symbols other than the lion, watermark, corners, duplicated medallions or surrounding frame. Centered entire circular silhouette.

### Ornamento de canto

> Use case: ui-mockup. One isolated ornamental CORNER PIECE for a medieval game settings frame, using the Maximo Rex concept's metal colors. Make a front orthographic solid sculpted gold/bronze filigree escutcheon shaped to follow the upper-left 90 degree corner: a short nearly horizontal arm reaching right, a short nearly vertical arm reaching down, with a broad acanthus S-scroll and two small engraved curls tying them together. Elegant late-medieval metalwork, compact stylized silhouette, warm amber-gold beveled edges catching upper-left light, dark umber insets, fine scratches and patina, matches an antiqued gold lion medallion and a dark leather panel. A single contiguous ornament occupies the middle, with its whole outline and a transparent safety margin on all four sides. Clean cutout on genuine transparent background. No frame, background surface, checkerboard, lettering, medallion, labels, watermarks, gemstone, repeated ornaments, or game imagery.
