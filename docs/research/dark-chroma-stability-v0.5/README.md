# D.RAW Dark Chroma Stability v0.5 — fine structure-support refinement

Date: 2026-09-29

## Motivation

Dark Chroma v0.4 successfully bound deterministic frontside regions to the
existing N2 Factored Confidence v0.3.1 field from the same sealed observation.

Three real-device cases validated that binding:

1. near-black tele: 432/432 visible candidates, globally
   `DARK_UNINFORMATIVE`, all correction blocked;
2. structured ultra-wide: 0 visible candidates, no invented candidate;
3. selective main/wide `1790637640867`: 6 visible candidates, all 6 locally
   bound.

The third case exposed a new limitation. The N2 factored 64×64 field had
`structure_protection_present=true` in 3072/3072 tiles although only about
16.56% of sampled source points were protected for structure. The v0.4 rule
"any protected point in any overlapping 64×64 tile" is therefore conservative
but spatially too coarse.

v0.5 measures the protection density at finer resolution without weakening the
existing protection law.

## New field

`TruthNegative N2 Structure Support Field v0.1` reruns the same N2 CFA audit
using:

- tileEdge = 32
- samplingPeriod = 8

No N2 threshold, structure rule or candidate rule is changed.

The field is source-bound and reports sampled structure/censor preservation
counts and fractions per 32×32 source tile.

## Frontside binding

`N2StructureSupportBindingAudit` maps each 16×16 frontside analysis region
into the source raster and gathers the overlapping 32×32 structure-support
tiles.

For every frontside region it records two complementary views:

1. **whole-overlap context** — all fine tiles intersecting the mapped source
   rectangle;
2. **fully-contained interior evidence** — only fine tiles completely inside
   that rectangle.

Boundary tiles are not silently fractionally assigned. This keeps the meaning
inspectable and conservative.

Reported diagnostics include:

- overlap sampled count;
- overlap structure-protected count/fraction;
- fully-contained interior sampled count;
- interior structure-protected count/fraction;
- number of fine tiles with/without sampled structure protection;
- maximum structure-protection fraction among overlapping fine tiles;
- censor and censor-boundary counts.

## Selective execution

The additional fine audit is intentionally not run for every source.

It runs only when:

- the source is a native-ready DNG;
- the frontside has one or more visible Dark-Chroma candidates;
- v0.3 is not globally `DARK_UNINFORMATIVE`.

Therefore:

- the validated near-black tele case is already blocked and skips the expensive
  refinement;
- the structured ultra-wide zero-candidate case skips it;
- the selective main/wide case runs it.

## Dark Chroma v0.5 contract

v0.5 does **not** reinterpret the new density values as permission.

Hard invariants:

- legacy v0.4 protection semantics are not reduced;
- fine metrics are not a scalar probability;
- unsampled pixels are not inferred;
- fine metrics cannot reduce protection;
- fine metrics cannot enable correction;
- `DARK_UNINFORMATIVE` cannot be overridden;
- `chroma_correction_supported=false`;
- `private_ab_delta_allowed=false`;
- `candidate_applied=false`;
- `scientific_writeback_allowed=false`.

The purpose of v0.5 is measurement: determine whether the six selective
main/wide candidates contain dense structure support, sparse structure support,
or structure-free sampled interiors at 32×32 reporting resolution.

## Device validation target

Primary target:

`DRAW_CAPTURE_1790637640867_wide_main_4096x3072.dng`

Expected:

- v0.1 visible candidates = 6;
- v0.3 non-degenerate;
- v0.4 local binding = true;
- fine structure support field runs;
- all six candidates receive fine structure metrics;
- correction-supported remains zero.

The result of that device test will determine whether a later version should
introduce a distance/support geometry gate. v0.5 itself does not introduce one.
