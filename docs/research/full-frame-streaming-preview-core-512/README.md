# Full-Frame Streaming preview core 512 equivalence — 2026-09-20

Status: **RESEARCH FALSIFICATION GATE — no Android promotion until exact equality passes**

## Motivation

After Scientific Master Streaming Binding v0.3, the 4080x3072 finalized-preview route is expected to reduce the scientific source-call schedule from 6144 logical tile requests to 240.

The remaining Full-Frame Streaming v0.1 preview still uses a 128x128 runtime core:

- 4080x3072 grid = 32 x 24 = 768 tiles/pass;
- two passes = 1536 source tile requests.

Full-Frame Streaming already defines tile core as an execution parameter. The v4.7i reconstruction requires a 3-pixel halo, Neutral Reference appearance requires 0, and HDR censor dilation requires 2; the Android preview currently provides a 16-pixel halo.

## Candidate

Change only the finalized-preview runtime core:

`128x128 -> 512x512`

Keep:
- halo = 16;
- workers = 1;
- HDR enabled;
- LUT size = 4096;
- the same v4.7i reconstruction;
- the same Neutral Reference appearance;
- the same bounded preview sink;
- the same 64 MiB logical resident budget.

For 4080x3072 this changes the preview grid to:

- ceil(4080/512) = 8;
- ceil(3072/512) = 6;
- 48 tiles/pass;
- 96 source calls across the two preview passes.

Combined with v0.3 scientific binding, the theoretical complete finalized-preview schedule becomes:

`240 + 96 = 336 logical source tile requests`

versus v0.73 measured `7680`.

This is not a latency claim.

## Required exact tests

128 and 512 must produce:
- bit-identical ExposurePlan floats;
- bit-identical full-resolution SDR float output;
- bit-identical half-resolution HDR log-gain floats;
- bit-identical Stage-2 diagnostics when enabled;
- identical bounded sRGB ARGB preview pixels;
- identical frame/evidence/provenance invariants.

Bounded preview equality is tested for orientations 1, 3, 6 and 8.

The 4080x3072 planning test must prove:
- 128 core = 768 tiles/pass;
- 512 core = 48 tiles/pass;
- 512 remains below the same 64 MiB logical resident budget.

GCC Release, Clang Release and Clang ASan/UBSan must all pass before Android integration.
