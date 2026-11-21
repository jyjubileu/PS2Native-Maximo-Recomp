# Stage 1 — Renderer and scene baseline

The controlled scene was the village in *Maximo vs. Army of Zin* PAL (`SLES_517.59`, CRC `86E533E0`). The original 4:3 F9 V19 state exposed a large brown triangle across the terrain. The control reproduced it at output frame 4116 with the CPU renderer, Vulkan and D3D12. The historical 16:9 run exhibited related geometry near the entrance. The same camera conditions were retained within each native A/B comparison.

PCSX2 Software 2.9.184 drew the same region without the triangle. A software GS Dump replay and a fresh game run established reference outputs. The cameras and the timing of the rain differed between native and reference states, which made full-screen pixel alignment qualitative at this stage. The issue persisted through changes of the native host renderer, so the next stage compared the VU and GIF vertex streams.

The terrain primitive was identified by a packed triangle strip spanning 28 vertices. Its vertex order, texture coordinates and ADC pattern allowed vertex-by-vertex comparison beyond frame indices. The archived runs and visual captures remain local because their images contain game material.
