# Build the PAL runner

The audit build used Windows x64, llvm-mingw and a Release Ninja build. The upstream CMake project also supports Linux. The PAL helper uses standard Python and CMake and does not depend on Git Bash. See [Dependencies](dependencies.md) for the packages used in the audit. A Git checkout also disables the inherited launcher updater.

## Run checks without a disc

```powershell
python scripts/check_vi_branch.py --cxx clang++ --jobs 6
```

For tools installed outside `PATH`, pass their full paths with `--cxx`, `--cmake` and `--ninja`. The checker uses a new build directory, compiles the CPU runtime directly, and does not link wgpu-native or load a game. It generates four synthetic VIF1 streams and compares entire result qwords against the recorded PCSX2 results. It then regenerates the AOT test objects and exercises the compiled entry points. Node.js on `PATH` enables the Wasm stage. A custom `--work` must name a new directory. The test prints a summary and writes `summary.json` there.

The synthetic generator also emits an ELF that uploads the stream through DMA1. To capture new reference values, generate a fixture and boot its ELF in your own PCSX2 setup with VU1 interpretation enabled and MTVU disabled. Read the `REX3` lines from the EE SIO log. Use the same generator for both engines.

```powershell
python tests/vi_branch_fixture.py build/reference-minimal
python tests/vi_branch_fixture.py build/reference-no-marker --without-marker
python tests/vi_branch_fixture.py build/reference-regressions --regressions
python tests/vi_branch_fixture.py build/reference-continuations --continuations
```

## Prepare a local build

1. Put a PAL ISO made from your own copy of the game in a local folder. The helper reads `SYSTEM.CNF` and independently checks the boot ELF and CRC. The [upstream disc tools](../../docs/disc-boot.md) support more inputs.
2. Point `--wgpu` to the extracted native package root containing `include/` and `lib/libwgpu_native.a` for GPU rendering. The default is `third_party/wgpu-native`. On Windows the helper also accepts `--sdl2` pointing to the directory containing `sdl2-config.cmake`. The project searches `third_party/sdl2` by default on Windows. On Linux, Clang needs a standard library providing C++20 `<format>`. Set `CXXFLAGS=-stdlib=libc++` if your Clang installation uses libc++.
3. For 16:9, provide your own `SLES-51759_86E533E0.pnach` with the `Widescreen 16:9` section. The upstream helper `python scripts/pnach.py fetch <ISO> --out .local/patches` can obtain the separately maintained patch when network access is available. Inspect the file with `pnach.py sections`. Keep it local.
4. For WebGPU, prepare the local SMAA lookup tables before running the build helper. [SMAA data setup](dependencies.md#prepare-the-local-smaa-tables)

```powershell
$iso = 'D:\local\maximo-pal.iso'
python scripts/build_maximo.py --profile 4x3 --disc $iso --work build/pal-4x3
python scripts/build_maximo.py --profile 16x9 --disc $iso --pnach '.local/patches/SLES-51759_86E533E0.pnach' --section 'Widescreen 16:9' --work build/pal-16x9
```

For each profile the helper extracts the ELF, applies the requested patch locally, builds fresh EE and VU tools, regenerates C++, builds the runtime from this checkout, and links the runner. `runner.json` stores the executable and its arguments. The 16:9 command includes `--widescreen` and the local runtime patch list.

```powershell
$command = Get-Content -Raw build/pal-4x3/runner.json | ConvertFrom-Json
$runner = $command[0]
$runnerArgs = $command[1..($command.Count - 1)]
& $runner @runnerArgs --window
```

Point the same launch commands to `build/pal-16x9/runner.json` for 16:9. For 4×, set `PS2_GS=gpu` and `PS2_SCALE=4` in the launch environment. The Windows app also offers an internal-resolution setting. It needs a WebGPU-capable adapter. See the [compatibility table](../../README.md#compatibility) for the tested scope.

## Checkpoint options

Use a local copy of a format-compatible `.ps2` Memory Card for slot 1. To attach the card, append `--memcard` and its path to the arguments in the example above.

```powershell
& $runner @runnerArgs --window --memcard '.local/Slot1.ps2'
```

To enable the experimental PAL equipment bridge in a local SDL build, reconfigure its existing CMake directory and rebuild the runner. The build verifies the expected entry point in the locally generated EE code.

```powershell
cmake -S . -B build/pal-16x9/native -DPS2_MAXIMO_STYLE_SWITCH=ON
cmake --build build/pal-16x9/native --target ps2run
.\scripts\start_maximo.ps1 -Profile 16x9 -BuildDirectory build/pal-16x9
```

Open **F1 → Graphics** for field composition, FXAA, SMAA 1x and CAS. Apply updates the active display settings and persists the selection. Use **F1 → Controls** to choose the weapon-switch method in a build with the PAL bridge. To launch with the classic F1 theme, set `$env:PS2_MAXIMO_F1_THEME = 'classic'` in the launching shell. [Checkpoint and validation](checkpoint-2026-11-22.md)

## Optional local recordings

A first build can use the disc alone. VU programs without static recordings use the interpreter, and the desktop runtime can translate newly discovered overlay code locally. The upstream workflow `bash scripts/port.sh <label> <ISO> --native-only` also explores the game and collects entries and VU recordings, using the checkout's toolchain. Its [runtime inspection notes](../../docs/aot-window-capture.md) describe local overlay captures.

To rebuild the same coverage as an existing legitimate local play session, pass `--seeds <folder>` with its `entries.txt`, `overlays/`, `vu0/` and `vu1/`. The helper reads those inputs to regenerate EE and VU code. The historical test checkpoints and shaders' rendered frames belong in local test data. The audit used private recordings for the game builds and zero game data for the generic VI/VIF checks. Exact parity with the historical test scene depends on the camera, checkpoint, region, local patch and recorded entries.
