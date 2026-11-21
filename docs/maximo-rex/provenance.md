# Source provenance

The installed updater marker and its historical log both name upstream `a50471a823c64e0e2893329527820a7c4deed608`, the November 15, 2026 merge of PS2Native PR #109. The official Git tree was retrieved and audited. In the updater's source scope, after accounting for line endings, 240 of 259 files matched. Its first parent `ac215782081a989d31a702e704df461ab3c26479` matches the two renderer files that differ in the merge. The independent runtime snapshot differed from the updater revision in 19 files overall. No single upstream commit matched the validated local engine. [Updater revision](https://github.com/exoteric33/ps2-native-recomp/commit/a50471a823c64e0e2893329527820a7c4deed608) · [First parent](https://github.com/exoteric33/ps2-native-recomp/commit/ac215782081a989d31a702e704df461ab3c26479)

The Git import contains the official source snapshot and its notices, including six upstream disc-toolkit sources missing from the historical installation folder. The next commit imports the verified 19-file runtime delta needed to match the pre-Stage-4 source, including CDVD seek completion, the earlier renderer and existing guard and diagnostic behavior. Three backup files stayed outside the import. New commits were created for this preparation. Earlier development is documented by technical milestones.

The Stage 4 archive was verified at SHA-256 `edee82aaf74743d06cad9bcef052fe02d32795373d5a951b66c40fd86f532ae9`. Four architecture files in the Git tree match its bytes and the fifth differs only in newline encoding. The sealed archive stays local. The [architecture diff](patches/stage-4-architecture.diff) and the [bancada diff](patches/stage-4-bench-and-tests.diff) are retained as audit artifacts. The active CMake build keeps the original source-directory path for generated game code, adds the asset-free test target, and resolves Windows synchronization for CPU-only clients. It does not carry the Stage 4 imported-game-archive workaround.

| Runtime delta against the updater revision | Paths |
|---|---|
| Memory and timing utilities | `runtime/include/ps2/memory.hpp`, `runtime/include/ps2/pace.hpp`, `runtime/src/memory.cpp` |
| Disc and CDVD | `runtime/src/disc.cpp`, `runtime/src/iop/cdvd.cpp` |
| GPU painter | `runtime/src/gs/gpu/gpu.cpp`, `runtime/src/gs/gpu/gs.wgsl` |
| IOP services | `runtime/src/iop/iop.hpp`, `runtime/src/iop/kernel.cpp`, `runtime/src/iop/kernel.hpp`, `runtime/src/iop/libs.cpp`, `runtime/src/iop/sif.cpp`, `runtime/src/iop/sif.hpp` |
| IPU | `runtime/src/ipu/ipu.cpp`, `runtime/src/ipu/ipu.hpp` |
| EE kernel and quick keys | `runtime/src/kernel.cpp`, `runtime/src/kernel.hpp`, `runtime/src/quick_keys.inc` |
| VU diagnostic code inherited by Stage 4 | `runtime/src/vu_micro.cpp` |

The candidate tag marks the audited source, tests and documentation. Original PS2Native installations, laboratory case folders, game executables and states remain independent of this Git lineage.
