# Stage 2 — Vertex-color origin

The incoming strip carried the intended base color `(18,68,73,127)`. A native `XGKICK` was carrying `(238,128,128,127)` for V0 of primitive #6496, at vertex index 7 within the strip. The four UNPACKs for the strip retained three 16-byte VU1 qwords at `m363–m365`. The preceding `0x0110` routine had left the qwords there. Under the lighting path of `0x0CF0` those operands yielded a negative red value and `0xFFFFF7EE` at `m1C7`. The low byte `EE` reached the GIF as red 238.

Two local differential tests isolated the effect.

| Experiment | Result |
|---|---|
| Change the 48 input bytes in an isolated execution of `0x0CF0` | All 28 color values matched the PCSX2 comparison. The recorded XYZ and ADC pattern were preserved. |
| Replay the reference GS Dump with the nine differing native colors substituted | The brown triangle appeared at the same camera. Across four occurrences the replay differed by 108 bytes. |

The test separated color from vertex assembly. Later native DMA restored the region with VIF command `0x6C09035E`. The evidence at this stage still allowed multiple explanations for why the caller reached the lighting path. The next stage traced command order, bank state and actual VU issue cycles.
