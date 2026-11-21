# PS2Native-Maximo-Recomp

Maximo Rex is a source-only PS2Native project for auditing and rebuilding the PAL release of *Maximo vs. Army of Zin* (`SLES_517.59`, CRC `86E533E0`). It includes the Stage 4 fix for VU1 integer-register timing. The fix is shared by the VU1 interpreters and generators and applies independently of the game.

Thanks to **exoteric33** and the contributors to [ps2-native-recomp](https://github.com/exoteric33/ps2-native-recomp), and to **Samir Nuri** for [ps2-web-recomp](https://github.com/snuri00/ps2-web-recomp). The source is distributed under the [MIT license](LICENSE), with [third-party notices](docs/maximo-rex/dependencies.md) retained.

## Compatibility

| Area | Evidence |
|---|---|
| PAL gameplay in 4:3 and 16:9 | Stage 4 completed 235 frames of scripted village movement per profile. The brown polygon disappeared in the compared captures. The input checkpoint, renderer and scene matter when comparing performance. |
| 4× internal resolution | Fresh 4:3 build, D3D12 with DXC, three captured gameplay frames at 2560 × 1792. |
| Level transitions | The disc-only rebuild reached the title, new-game selection and opening cinematic. Transitions between playable levels remain unverified. |
| CDVD Seek Fix | The historical seek completion behavior is present in the runtime baseline. |
| Other regions and full playthrough | Outside the validated scope. |

Fresh PAL builds also completed 62 frames per aspect ratio from copies of the historical checkpoints and reproduced eight reference captures byte for byte. The Stage 4 differential matched PCSX2 for all 28 colors of the affected strip and all 840 color values across 30 matched packets. Headless CPU-renderer AOT frame time increased by 17.7% in the measured 4:3 scene. The 16:9 result improved in its own measurement window. This is an experimental source candidate. [Validation and limits](docs/maximo-rex/stage-4-vi-branch-timing.md)

## Build and test

Use a C++20-capable Clang toolchain, CMake 3.20 or newer, Ninja and Python 3.11 or newer. SDL2 enables the desktop window. wgpu-native enables WebGPU rendering and higher internal resolutions. Node.js is used by the optional Wasm checks. The tested package versions and their sources are listed in [Dependencies](docs/maximo-rex/dependencies.md).

Use your own legitimate PAL disc image. The 16:9 build also uses a separately obtained PNACH with the `Widescreen 16:9` section. The repository distributes no disc images, commercial executables, game assets, game-generated code, saves, dumps, or built game binaries.

```powershell
python scripts/check_vi_branch.py
python scripts/build_maximo.py --profile 4x3 --disc "D:\local\maximo-pal.iso"
python scripts/build_maximo.py --profile 16x9 --disc "D:\local\maximo-pal.iso" --pnach "D:\local\SLES-51759_86E533E0.pnach" --section "Widescreen 16:9"
```

The build starts from the checkout's C++ sources and generates the game runner under the ignored `build/` directory. [Build and run instructions](docs/maximo-rex/build.md)

## Engineering history

- [Stage 1 — renderer and scene baseline](docs/maximo-rex/stage-1-renderer-baseline.md)
- [Stage 2 — first vertex-color divergence](docs/maximo-rex/stage-2-vertex-color.md)
- [Stage 3 — synchronized DMA1, VIF1 and VU1](docs/maximo-rex/stage-3-vif-vu-timing.md)
- [Stage 4 — VI branch timing and validation](docs/maximo-rex/stage-4-vi-branch-timing.md)
- [Source provenance](docs/maximo-rex/provenance.md) · [Publication audit](docs/maximo-rex/publication-audit.md)
