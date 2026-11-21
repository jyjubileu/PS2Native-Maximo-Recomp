# Stage 3 — Synchronized DMA1, VIF1 and VU1

The paired DMA1 strip payload was 832 bytes in each engine and had SHA-256 `595be26e070903912fd8d19543b797efba362a534cb02e3390f42690bec368e3`. The comparison matched VIF input before UNPACK, double-buffer state, VU microcode, executed PCs and `XGKICK`. The reference was PCSX2 2.9.184 with the VU1 interpreter and MTVU disabled.

For the `IBEQ` at `0x0D00` and its precursor at `0x0E00`, `IADDIU` wrote VI2 in modeled cycle 18. An earlier `ILW` held VI1 in the scoreboard for two extra cycles before the branch could issue in cycle 21. PCSX2 expired the two-cycle backup before reading the operands. The native control retained the previous-issue view. The fresh value `1` matched VI1 and took `0x01C8`. The stale operands sent the native control down a path that overwrote `m3BE.z` with `2` and later ran the lighting code.

PCSX2 also had overwritten matrix slots in `m363–m365` before the subsequent UNPACK restored them. Both engines rotated the VIF banks consistently in the matched case. Disabling native VIF deferral alone preserved the bad branch. The EE payload, UNPACK and buffer differences were eliminated as the first causal divergence. Expiring the stale VI view restored the PCSX2 selector and strip colors while leaving the three qwords unchanged.

The stage produced a 1,664-byte synthetic stream to test these edge cases independently of Maximo. Its expected outputs appear in [the test oracle](../../tests/vi_branch_expected.json), and the stream can be regenerated with the [fixture builder](build.md#run-checks-without-a-disc). [Reference VU1 interpreter](https://github.com/PCSX2/pcsx2/blob/v2.9.184/pcsx2/VU1microInterp.cpp)
