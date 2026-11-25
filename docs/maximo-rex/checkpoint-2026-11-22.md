# Maximo Rex · checkpoint through Stage 10D

The 22 November 2026 development checkpoint is a consolidated source snapshot of Stage 10D, `445fea07ab3e8bf2939ba28b5aad3d646ef7801a`. Development and validation used AI tools, including Codex. The F1 theme also uses original ImageGen ornaments. This remains experimental. A full playthrough and a public playable release are pending.

## Development history

| Stage | Local branch or reference | Commit | Result |
| --- | --- | --- | --- |
| 4 | `maximo-rex-stage4-candidate-1` | `b2bbe57` | VU1 VI timing fix and the verified PAL source baseline |
| 5 | `stage5/maximo-rex-settings` | `1646f5b` | Host F1 settings, remapping and local profile launcher |
| 6 | `stage6/widescreen-video-input` | `e80a673` | Widescreen validation and controller reference settings |
| 7A–7B | `stage7a/image-clarity`, `stage7b/image-clarity-isolation` | `5d41270`, `4387e8c` | Field alignment, adaptive composition and optional area reduction |
| 8A | Existing `--memcard` runtime path | `445fea0` contains the tested runtime | Persistent slot 1 with a local `.ps2` card |
| 9–9B | `stage9/weapon-wheel`, `stage9b/true-style-switch` | `3b8974e`, `44521ea` | Weapon wheel and direct equipment via the original game logic |
| 9C–9E | `stage9c/l2-weapon-wheel`, `stage9d/running-style-switch`, `stage9e/dpad-style-switch` | `e0ed394`, `937ebc0`, `b648ed9` | Configurable L2, validated grounded locomotion and four-way D-Pad selection |
| 10A–10B | `stage10a/residual-blur-diagnosis`, `stage10b/dual-parity-fields` | `b648ed9`, `99ceb8a` | Residual blur study and adaptive composition that compares two field pairs |
| F1 medieval | `experiments/medieval-f1` | `6369192` | Native theme, cancellable preferences and classic fallback |
| 10C | `stage10c/fxaa-smaa` | `0203293` | Alternative output-resolution FXAA and SMAA 1x |
| 10D | `stage10d/fidelityfx-cas` | `445fea0` | Adjustable AMD FidelityFX CAS, Sharpen Only |

The snapshot uses the audited Stage 4 ancestor and the Stage 10D source tree. The original stage commits and checkpoints remain in the local development archive. The support documentation and funding configuration came from `origin/main` at `29038b84641b69dfd0f9b6422f53c5e495b52784`.

## Publication audit

Run `python scripts/audit_publication.py` in a checkout containing only the candidate and the tag to publish. The script audits the whole reachable history of the refs present in that checkout and writes a file and hash inventory. The checkpoint excludes the gameplay screenshots from the public main history, game inputs, generated game units, build products, local evidence and saves. The Style Switch guard verifies the expected local code by hash. The SMAA tables are produced locally from the pinned upstream data.

Only the 18 theme PNGs read by F1 were approved by exact path and SHA-256. They reproduced byte for byte from the archived art generator and the original ornament masters. The master renders and the concept references remain in the local art archive. [Theme provenance](../../assets/ui/maximo_f1/source/ART-SOURCES.md) · [Third-party notices](dependencies.md)

## Validation and limits

The publication checkout rebuilt the Windows GPU runner and the SDL test harness with the pinned SMAA tables and a locally generated PAL input. The equipment bridge generated the same unit as Stage 10D. The build passed 62 controller checks and 41 settings checks in each theme. The disc-free VI differential suite and its native tests passed.

- Local Stage 10D runs applied and persisted CAS strength through F1 and exercised all five field composition choices.
- The PAL 16:9 replay equipped all four unlocked weapons through the D-Pad and tested a radial swap on the ground. Equipment is gated by the game state and unlocks. Attacks, airborne states and every possible gameplay transition are outside the validated scope.
- The replay exercised D3D12 and Vulkan at 4× and 1920×1080 on the Radeon RX 6600. With CAS disabled, 42 frames matched Stage 10C byte for byte. Six paired comparisons verified that CAS enabled at 0% matched CAS disabled across the three AA modes. The CAS shader matched 967,032 sampled RGB channel values from the AMD CPU reference within one 8-bit code.
- Two-field-pair composition remains the experimental default. CAS and additional AA are disabled by default for new preferences. CAS at 30% sharpened the ground and armor in the tested scenes. Higher strengths also sharpened texture noise and existing steps. Local measurements characterize the tested scenes and GPU passes.
- Wider validation remains for full gameplay and level transitions, long save/load sessions, audio continuity, other system configurations and languages, and performance across more scenes. [Build and test locally](build.md)
