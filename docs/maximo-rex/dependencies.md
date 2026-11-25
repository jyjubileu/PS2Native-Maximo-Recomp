# Dependencies and notices

The imported PS2Native source carries the [MIT license](../../LICENSE) and attribution to Samir Nuri and exoteric33. The original [upstream revision and license](https://github.com/exoteric33/ps2-native-recomp/blob/a50471a823c64e0e2893329527820a7c4deed608/LICENSE) are pinned in the provenance record.

## Notices retained in the source

- The VLC tables in the IPU are adapted from Play! and retain the [BSD-2-Clause notice](../../runtime/src/ipu/NOTICE).
- The upstream disc toolkit retains the MAME CHD-container attribution and full [BSD 3-Clause notice](../../tools/ps2_disc_toolkit/containers.py) alongside the affected source.
- The official upstream snapshot in the Git history includes the altered Mark Adler `puff` implementation and its zlib terms in `runtime/src/gs/gpu/gpu.cpp`. The runtime inherited by Maximo Rex uses the earlier renderer snapshot. The notice stays with the code in the upstream import.
- PCSX2 2.9.184 was a separate test reference. The test oracles and synthesized programs in this repository are authored test data. PCSX2 source, installers, BIOS files and patch databases are external. [PCSX2 source and license](https://github.com/PCSX2/pcsx2/tree/v2.9.184)
- SMAA 1x uses a WGSL port and the upstream lookup data. The copyright and MIT permission notice apply to the port and the locally prepared tables. [SMAA notice](../../runtime/src/gs/gpu/smaa/LICENSE.txt)
- The Sharpen Only WGSL port of AMD FidelityFX CAS retains the AMD copyright and MIT notices. [CAS notice](../../runtime/src/gs/gpu/cas/LICENSE.txt) · [AMD implementation and attribution](../../runtime/src/gs/gpu/cas/README.md)

## Native build and test packages

These are the versions used in the Windows audit or pinned by the original download scripts. Install the tools separately and retain the notices provided with their packages.

| Package | Tested version or requirement | License and distribution notes |
|---|---|---|
| llvm-mingw and Clang | 20260908 UCRT x86-64 with Clang 23.1.1. C++20, `<format>`, and Clang vector builtins are required. | LLVM code uses Apache-2.0 with the LLVM exception. The toolchain archive includes components with their own notices. [Release](https://github.com/mstorsjo/llvm-mingw/releases/tag/20260908) |
| CMake | 4.4.4 in the audit. The project requires 3.20 or newer. | [CMake terms](https://github.com/Kitware/CMake/blob/v4.4.4/Copyright.txt) |
| Ninja | 1.13.2 | [Apache-2.0 notice](https://github.com/ninja-build/ninja/blob/v1.13.2/COPYING) |
| Python | 3.12.10 in the audit. Python 3.11+ for the checks. | The portable installer pins 3.13.16. [Python terms](https://docs.python.org/3.12/license.html) |
| SDL2 | 2.32.10, MinGW x86-64 on Windows | Optional for the windowed runner. [zlib license](https://github.com/libsdl-org/SDL/blob/release-2.32.10/LICENSE.txt) |
| wgpu-native | v29.0.1.1, Windows x86-64 GNU package | Optional for headless CPU checks, needed for GPU and 4× tests. Inspect the [release terms](https://github.com/gfx-rs/wgpu-native/tree/v29.0.1.1), including Apache-2.0 and MIT. |
| DirectX Shader Compiler | Package v1.9.2609. Local DLL reported 1.9.2609.5. | Optional Windows package. The release supplies Microsoft binary terms and LLVM notices. [Release](https://github.com/microsoft/DirectXShaderCompiler/releases/tag/v1.9.2609) |
| Node.js | 24.22.3 in the audit | Required for the optional Wasm test execution. [Node distribution notices](https://github.com/nodejs/node/blob/v24.22.3/LICENSE) |
| Git and shell tools | Git for Windows 2.56.1.windows.1 in the audit. Bash 4+ for the upstream shell build path. | The optional portable setup pins MinGit 2.56.0.2 and BusyBox FRP-6075. Their binaries and notices are distributed by their own projects. [Portable installer pins](../../scripts/portable.ps1) |

The upstream `fetch_sdl2.sh`, `fetch_wgpu.sh` and `fetch_dxc.sh` scripts list the exact package digests. The browser build uses its separate Emscripten setup in the upstream scripts. This audit qualifies the native Windows runner and tests emitted Wasm VU modules in Node.js. It does not qualify a full browser build.

## Prepare the local SMAA tables

For a WebGPU build, place the upstream `AreaTexDX10.dds` and `SearchTex.dds` in a local folder. Use the exact upstream revision below. [Pinned SMAA textures](https://github.com/iryoku/smaa/tree/71c806a838bdd7d517df19192a20f0c61b3ca29d/Textures)

```powershell
python scripts/prepare_smaa.py .local/smaa-upstream
```

The helper verifies both input SHA-256 hashes and writes the two raw tables under ignored `.local/smaa/`. CMake verifies the resulting hashes. If you use a different destination, set `PS2_SMAA_LUT_DIR` when configuring CMake. The public checkpoint ships the shader, the preparation script and the license. Headless CPU checks can run without those tables.
