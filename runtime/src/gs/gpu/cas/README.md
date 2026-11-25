# AMD FidelityFX CAS

The WGSL presentation shader ports the 32-bit `CasFilter` sharpen-only path from [AMD's header at 9fabcc9](https://github.com/GPUOpen-Effects/FidelityFX-CAS/blob/9fabcc9a2c45f958aff55ddfda337e74ef894b7f/ffx-cas/ffx_cas.h). It uses the reference options `CAS_SLOW`, `CAS_GO_SLOWER` and `CAS_BETTER_DIAGONALS`. Texture loads are clamped to the game viewport. The shader uses an sRGB texture view for linear-light input and returns display-encoded RGB through the existing sRGB helper.

For positive percentages the host passes the `CasSetup` coefficient `const1[0]` with `sharpness = percent / 100`. The 1:1 path only needs that coefficient. The application's 0% and off states bypass the pass. The AMD API's native sharpness of zero still produces a nonzero coefficient.

[AMD copyright and MIT notice](LICENSE.txt).
