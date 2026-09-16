# Scientific Master Streaming Binding v0.2

Status: **RESEARCH IMPLEMENTATION CANDIDATE — CI PROOF REQUIRED**

Parent optimization baseline:
`46cc590a24674d87fc6e5b2db67e198e40db23d9`

## Purpose

v0.2 is a bit-exact read-amplification optimization of Scientific Master Streaming Binding v0.1.

It does **not** change:
- admitted source samples;
- Stage-2 construction;
- reconstruction backend;
- Scientific Master digest format or content;
- TruthRange v0.2 eligibility rules;
- self-gauge quantile (`0.5`) or 10% border exclusion;
- zero-line/scene-scale semantics;
- physical-frame or independent-evidence counts;
- color/appearance/preview rendering;
- scientific authority.

The only algorithmic change is how the exact positive-float median is selected.

## Physical Honor baseline reconciliation

The first physical Honor/MotionCam finalized run reported **13,824 logical `IRawTileSource::readRawTile` calls**. This is not asserted to be 13,824 OS-level `pread()` syscalls; `TileNativeDngSource` can perform multiple bounded byte reads inside one logical tile request.

For the 4080x3072 lineage the existing code explains the logical count exactly:

- Scientific Master canonical core: 64x64
- tile grid: `ceil(4080/64) * ceil(3072/64) = 64 * 48 = 3,072`
- v0.1 exact median: 4 complete Stage-2 scans
- scientific binding reads: `3,072 * 4 = 12,288`
- Full-Frame preview core: 128x128
- preview grid: `ceil(4080/128) * ceil(3072/128) = 32 * 24 = 768`
- Full-Frame Streaming pass 1 + pass 2: `768 * 2 = 1,536`
- total: `12,288 + 1,536 = 13,824`

This exact reconciliation is why v0.2 targets the self-gauge radix schedule before attempting cache heuristics.

## Exact two-pass radix16 selection

v0.1 resolves the median float bits one byte at a time:

1. high byte during the Scientific Master pass;
2. second byte in another full Stage-2 scan;
3. third byte in another scan;
4. low byte in another scan.

v0.2 uses the same ordering property of **positive finite IEEE-754 binary32** values: for positive floats, unsigned bit-pattern ordering equals numeric ordering.

v0.2 therefore performs:

1. Scientific Master pass + a 65,536-bin histogram of the high 16 bits;
2. one refinement pass with separate 65,536-bin low-16 histograms for the exact lower and upper median ranks.

The lower and upper median float bit patterns are then converted exactly as in v0.1, and `L0` is computed with the same expression:

`lower + (upper - lower) * 0.5`

No approximate quantile or tolerance is introduced.

## Resource tradeoff

v0.1 uses two 256-bin `uint64_t` histograms (~4 KiB auxiliary radix state).

v0.2 reserves two 65,536-bin `uint64_t` histograms:

- 512 KiB each;
- maximum radix auxiliary state = **1 MiB**.

This remains bounded and independent of image megapixel count. The caller memory budget is checked before the histograms are allocated and continuously with the tile workspace/digest resident bound.

For the physical 4080x3072 lineage, the scientific read schedule becomes:

- v0.1 scientific: 12,288 logical tile reads;
- v0.2 scientific: 6,144 logical tile reads;
- preview unchanged: 1,536 logical tile reads;
- predicted total after integration: **7,680 logical tile reads**.

That is **44.44% fewer total logical tile requests** than the physical baseline. The actual RAW payload bytes, wall time, CPU time and PSS must be re-measured on the Honor; they are not inferred as proven from call-count reduction.

## Falsification gates

Before integration, CI must prove v0.1/v0.2 equivalence on multiple synthetic Stage-2 populations:

- pseudo-random signal with source-censored samples;
- constant signal;
- an even split whose lower and upper median intentionally fall in distant high-16 float prefixes;
- sparse positive evidence mixed with black-level and censored samples.

For every successful case v0.2 must equal v0.1 in:

- Scientific Master SHA-256;
- exact `double` bit pattern of `L0`;
- zero-line mode/id/flags;
- scene binding;
- eligible sample count;
- canonical master tile count;
- physical/evidence counts.

And it must show:

- v0.1 = 4 Stage-2 gauge scans;
- v0.2 = 2 Stage-2 gauge scans;
- exactly half as many scientific `readRawTile` calls/sample requests in the counting source;
- fail-closed before source reads when the caller memory budget cannot admit the v0.2 radix state.

GCC Release, Clang Release and Clang ASan/UBSan must all pass before integration into the finalized Scientific Preview route.

## Non-claims

Until those gates and a new physical Honor run pass, v0.2 does **not** claim:

- lower physical Honor latency;
- lower physical RAW payload bytes;
- lower energy use;
- lower thermal load;
- lower PSS than v0.1;
- Android integration;
- replacement of the validated v0.1 path.

Historical v0.1 bytes remain immutable.

**Measured where measured. Reconstructed where necessary. Never invented.**
