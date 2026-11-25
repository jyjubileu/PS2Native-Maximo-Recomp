# SMAA 1x lookup textures

The SMAA shaders use the reference area and search tables from [iryoku/smaa](https://github.com/iryoku/smaa/tree/71c806a838bdd7d517df19192a20f0c61b3ca29d).

Prepare these files locally before configuring a WebGPU build. [Preparation instructions](../../../../../docs/maximo-rex/dependencies.md#prepare-the-local-smaa-tables)

- `area.rg8` is the first 80 rows of `Textures/AreaTexDX10.dds`, in row order, with its red and green channels extracted from the BGR8 payload. This is the zero-offset plane used by SMAA 1x, with orthogonal patterns in columns 0–79 and diagonal patterns in columns 80–159.
- `search.r8` is the 64×16 eight-bit payload of `Textures/SearchTex.dds`, in row order.

The port and both tables are covered by [the SMAA license](LICENSE.txt).
