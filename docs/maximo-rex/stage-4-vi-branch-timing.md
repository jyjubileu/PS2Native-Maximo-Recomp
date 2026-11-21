# Stage 4 — VI branch timing

## Implemented rule

VU1 records the integer value before the write, the VI index and the effective issue cycle. A conditional branch reads that backup only when fewer than two issue cycles have elapsed. A consecutive write can extend the active same-register backup. The scoreboard accounts for `ILW` and `ILWR` dependencies before the branch reads. The interpreter activates the necessary timing even when the loaded microcode never consumes clip/MAC/status flags or Q. The VU0 branch path retains its prior policy.

The AOT generator applies the rule after its computed stalls and handles transfer after supported delay slots. Dynamic continuations retain pending VI, VF, Q and flag state. Unsupported pairs go through the interpreter from the entry. Native live compilation invalidates the old generator cache. The Wasm emitter enforces the same timing within its supported programs and leaves indirect entries to the interpreter. Shader and vertex output code is untouched by the Stage 4 architecture patch. [Architecture diff](patches/stage-4-architecture.diff)

## Differential results

| 1,664-byte fixture condition | Result |
|---|---:|
| `ILW` pending, VI2 written, `IBEQ` | 7 |
| Adjacent write and branch with no stall | 9 |
| `ILW` pending and `IBNE` | 9 |
| `NOP` separating the VI2 write and branch | 7 |
| `ILW` outside the branch operands | 9 |
| `XTOP` and `XITOP` | `0x50` and `3` |

Exact, fast and AOT paths agreed with the PCSX2 oracle for the minimal stream, its marker-free variant, eight additional regressions and two continuation cases. The regressions exercise zero, one and two extra stall cycles, `ILWR`, consecutive writes, delay slots, the end of a backup chain and different integer operands. Real Wasm modules ran in Node, and the Windows native live path was exercised in the original Stage 4 lab. The historical `ps2vifdefertest` exact-mode accounting discrepancy still affects its fast-entry counter. The default and explicitly disabled VIF-deferral checks passed.

## Gameplay and geometry

In the target strip, the candidate matched 28/28 reference RGBA values with the native XYZ2 and ADC preserved. STQ bytes were identical in 27/28 native vertices and normalized UV matched all 28. Across 30 uniquely matched strips, 840/840 colors coincided with PCSX2. The full GS-Dump replay already contained the candidate colors and had identical bytes when the candidate values were substituted.

The wider geometry difference was also localized. With the state of 52 PCSX2 entries replayed, the first extra mismatch in `0x0110` was at `IBEQ 0x2010`. One stall expired the backup and the candidate took the same geometry path as the reference. The maximum absolute differences across 1,088 XYZ2 vertices dropped from `(1831,7427,72713)` to `(1,1,3)` in packed GIF units. Across the 4:3 capture window, AOT and the fast interpreter emitted 9,138 byte-identical packets. The reference interpreter has minor rounding differences.

The lab exercised 235 frames with movement in the village in each aspect ratio and reopened candidate checkpoints. The triangle disappeared in the inspected frames. The historical 4:3 headless CPU-renderer AOT medians were 83.59 ms/frame before and 98.41 ms/frame after. The 16:9 medians were 145.25 and 125.63 ms/frame. The 4:3 performance cost remains open. These measurements came from separate Windows runs at the same settings per aspect ratio. They are throughput measurements and do not establish full-speed gameplay.

The build-preparation retest is recorded in the [publication audit](publication-audit.md). The source patch is confined to the VU runtime and generator and contains no per-game color, clipping or vertex substitution.
