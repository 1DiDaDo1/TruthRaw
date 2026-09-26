# D.RAW current project state — 2026-09-26

Status: **CURRENT ACTIVE CODE-BEARING DEVELOPMENT STATE**

This document supersedes the 2026-09-25 project-state document for current
navigation. Older dated files remain immutable historical provenance.

## Repository / branch truth

Repository:

`1DiDaDo1/TruthRaw`

Current active architecture branch:

`architecture/lens-independent-free-world-observation-v01-2026-09-26`

Latest Android-code branch:

`research/appearance-highlight-headroom-sweep-v02-2026-09-26`

Latest fully Android-validated code checkpoint:

`402c72d6804f6cc393a5eab93c9d94d0687be111`

Real-device Appearance v0.1 evidence checkpoint layered above that code:

`b9e613832d8736d436fd2b25776ea27fafe58be3`

The repository default branch `main` is **not** the current development
line. At this date it still points to:

`514f2f4bde6aba5a6709e176c03b22c3b9aea912`

Do not treat default `main` as the latest D.RAW implementation until a
deliberate promotion is performed.

## Permanent rule

> **Representation can exceed the source. Knowledge claims cannot exceed the evidence.**

Companion rule:

> **Measured where measured. Reconstructed where necessary. Never invented.**

## Current architecture

```text
sealed physical observation / Source Evidence
 -> D.RAW Observation Contract
 -> Source Capability Envelope
 -> Float64 measurement/calibration/reconstruction
 -> Scientific Master
 -> validated Float32 scientific storage where admitted
 -> TruthNegative Continuous per observation
 -> Free World Observation Graph
 -> Deep Scene / geometry-radiometry separation
 -> Light Transport
 -> View Contract / Appearance
 -> finite projection
```

The Free World is not bounded by RAW code range, WhiteLevel, source raster,
Float32, SDR or DNG. Scientific authority remains bounded by admitted evidence.

Permanent current invariants include:

- `physicalFrameCount=1`;
- `independentEvidenceCount=1`;
- Direct CFA immutable;
- Scientific Master immutable to appearance/N2 diagnostics;
- TruthNegative immutable to appearance/N2 diagnostics;
- `createsNewEvidence=false`;
- `scientificWritebackAllowed=false`;
- CENSORED is a bound, never an invented exact radiance;
- UNKNOWN does not gain scientific authority from appearance;
- display/appearance never upgrades scene authority;
- Honor/GCam/computational rendering is never D.RAW scientific evidence.

# Lens-independent Free World Observation Architecture v0.1

Status: **SEALED CANONICAL ARCHITECTURE OVERLAY**

The Free World is no longer allowed to be interpreted as a Camera-5/telephoto
world. A lens, sensor, CFA/readout and capture pipeline define the procedure of
one observation; they do not define the world boundary.

Canonical law:

> **One Free World. Many sealed observations. One evidence law.**

Read:

- `docs/CORE_VISION_LENS_INDEPENDENT_FREE_WORLD_OBSERVATION_ARCHITECTURE.md`;
- `docs/research/lens-independent-free-world-observation-contract-v0.1/README.md`;
- `docs/research/lens-independent-free-world-observation-contract-v0.1/DRAW_OBSERVATION_CONTRACT_v0_1.json`;
- `docs/research/lens-independent-free-world-observation-contract-v0.1/SEAL_MANIFEST_v0_1.json`;
- `state/LENS_INDEPENDENT_FREE_WORLD_V01_STATE_2026-09-26.json`.

## Observation contract

Every admitted RAW/DNG becomes one explicit observation lineage binding Source
Evidence, acquisition procedure, source topology, calibration/capability scope,
gauge, authority/uncertainty, Scientific Master, TruthNegative and provenance.

Main, ultra-wide, telephoto and future cameras use the same architecture.
Source-specific facts stay in the Source Capability Envelope.

No calibration is transferred merely because two sources share a phone,
manufacturer, nominal ISO, lens family, raster size or file format.

## Zero-Line across observations

The positive-light companion coordinate remains:

`T = log2(L/L0)`

The coordinate family may be shared, but equal numeric T values from two
observations are not automatically equal radiometry. Cross-observation
comparison/fusion requires an admitted gauge relation.

This preserves both ideas at once:

- the Free World coordinate has no source-defined bright/dark ceiling;
- every real observation still has finite evidence and a bounded calibration
  claim.

## Float64 / Float32 scientific RAW state

The sealed contract formalizes:

```text
exact packed/integer Source Evidence
 -> Float64 branch-sensitive measurement/reconstruction/calibration
 -> validated Float32 canonical scientific storage where admitted
 -> later Float64 compute again when required
```

Precision never upgrades authority.

The D.RAW scientific RAW/state may store signed scene-linear values,
TruthRange estimate/bounds, uncertainty, authority, censor state, source
footprint, Observation ID, calibration/gauge identities and separate geometry
and radiometry authority. It is a derived scientific state and may never
impersonate the original sensor RAW.

## TruthNegative and multi-lens composition

TruthNegative is now explicitly the raster-independent evidence-aware
scientific negative of **one admitted observation lineage**.

It does not implicitly fuse lenses.

Multiple main/ultra-wide/tele or external-camera observations compose one
level higher in the Free World Observation Graph, retaining separate source
identities, frame counts, correlation assumptions, authority and provenance.

## Byte seal

The v0.1 canonical files are byte-sealed by SHA-256. Silent edits fail the
dedicated verifier. Semantic changes require a versioned successor.

This architecture-only branch does not alter the current Scientific Master,
TruthNegative implementation, N2, Appearance route, or validated APK pixel
code.

# First executable lens-independent observation record

`DRAWObservationRecord v0.1` now turns the sealed architecture into a
machine-validated source record.

Current real example:

`docs/research/draw-observation-record-v0.1/examples/CAMERA5_LAMP_SCENE_OBSERVATION_v0_1.json`

It records the lamp-scene Camera-5 source as one TELEPHOTO observation while
keeping:

- the source gauge local;
- `shared_free_world_gauge_id=null`;
- cross-observation radiometric equality/fusion disabled;
- optics scientific use disabled while optics support is UNKNOWN;
- TruthNegative bound to exactly this observation lineage.

Validation run `36258955138`: **SUCCESS**.

The workflow accepts the valid record and deliberately rejects a mutated record
that tries to enable cross-observation fusion without a common admitted gauge.

This is the first concrete step toward plugging main and ultra-wide into the
same Free World without creating camera-specific worlds.

# N2 — current validated research line

N2 remains **appearance/audit research only**. It is not production-promoted
into Scientific Master or TruthNegative.

## Bounded residual estimator v0.1

The estimator decomposes:

`observation = localEstimate + residual`

and:

`residual = removedResidual + retainedResidual`.

Suppression remains bounded by the Structure Preservation Gate. The estimator
does not define its own neighborhood predictor and never grants authority.

## Full-colour A/B route

The current diagnostic route is:

```text
Direct CFA
 -> v0.1 N2 candidate
 -> private Stage-2 copy
 -> same measured-preserving Float64 full-colour reconstruction
 -> exact Scientific-Master baseline gate
 -> temporary appearance-only B
 -> A/B/delta Risk/Quality audit
```

The earlier incorrect 1:1 TruthNegative Continuous baseline comparison was
rejected because a continuous area-resolve is not the same object as an exact
Scientific-Master source pixel.

The corrected baseline gate requires the unmodified reconstruction to be
Float32 bit-identical to the exact Scientific Master source pixel.

Repeated real-device tests have produced `baseline-mismatch=0`.

## Reconstruction-support protection closure

Source-site protection alone was proven insufficient: an allowed neighboring
CFA correction can propagate through the reconstruction halo into a protected
full-colour output pixel.

The current closure therefore:

1. identifies protected output cores;
2. dilates the guard over the reconstruction backend's exact
   `requiredHalo()`;
3. suppresses candidate CFA corrections inside that support;
4. reconstructs full colour;
5. requires every protected-core output RGB pixel to remain Float32
   bit-identical to baseline.

Real-device diagnostics repeatedly show:

- support guard actively suppressing candidate sites;
- reconstruction radius = `3 px` for the current backend;
- `protected-changed=0`;
- `baseline-mismatch=0`.

This is a necessary safety gate, not proof that unprotected denoise is ideal.

## Center-excluded predictor v0.2 and whole-frame v0.2.1

The center observation is excluded from neighborhood selection and prediction.

The whole-frame sidecar retains exact v0.1 candidate parity and records
directional/multiscale support, center-only sigma residuals, variance ratios
and CFA phase behavior.

Noise independence is deliberately **not admitted**.

## N2 Confidence Field v0.3

v0.3 is vector-valued. It keeps candidate density, predictor coverage,
pair/scale coherence, center residual bins, protection load, variance relation
and CFA phase as separate measurements.

It does not create a scalar denoise probability and
`promotion_eligible=false`.

## N2 Factored Confidence State v0.3.1

v0.3.1 keeps exact facts separate rather than forcing every useful region into
the old strict `FULLY_COHERENT` label.

Facts include:

- candidates present;
- all candidates predictable;
- center-outlier-free;
- pair rejection free;
- scale rejection free;
- structure/censor/boundary protection present;
- max predictor variance <= center variance.

No thresholds were relaxed into 90/95% rules, no weights were invented, and
the legacy class is explicitly non-authoritative.

# Appearance / highlight finding — real device

The user supplied a real D.RAW Appearance Highlight Detail v0.1 JSON from an
admitted Camera-5 DNG.

Evidence summary:

`docs/research/truthnegative-appearance-highlight-detail-audit-v0.1/evidence/DEVICE_RESULT_2026-09-26.md`

Uploaded JSON whole-file SHA-256:

`ff9ec10c6012cc75578b21c9c8df694817086552a0474022d512ccb19e95e6df`

## Tested PRO display state

The existing PRO preview used:

- display reference white = `100 nit`;
- display peak = `100 nit`;
- sRGB transport;
- neutral appearance policy.

The audit reports:

- `no_highlight_headroom=true`;
- `mapped_peak_collapse_observed=true`;
- source scene remains immutable;
- no new evidence;
- no scientific writeback.

## Global device measurements

For the 192x145 diagnostic raster:

- samples = `27,840`;
- source samples above 100 nit = `502`;
- samples mapped exactly to 100-nit peak = `502`;
- source CENSORED samples = `0`;
- distinct adjacent source-luminance pairs = `55,343`;
- distinct pairs collapsed to exactly the same display peak = `851`;
- bright collapsed pairs = `851`;
- collapse fraction ~= `1.5377%`;
- source absolute luminance-gradient sum = `136222.244756152795`;
- mapped gradient sum = `131815.222728787980`;
- mapped/source gradient retention ~= `0.9676483`;
- aggregate gradient reduction ~= `3.2352%`;
- maximum source-luminance difference hidden in one collapsed adjacent pair =
  `18.955463394113 nit`.

This proves a many-to-one property of the current Appearance/Display mapping
for this tested scene. It does not claim that every numeric difference is
individually perceptible.

## Strongest measured highlight tile

At preview tile `x=80,y=48,16x16`:

- 206/256 samples are above 100 nit;
- all 206 map to display peak;
- source CENSORED = 0;
- 383/480 distinct neighbor pairs collapse to the same peak;
- source gradient sum ~= 1868.53;
- mapped gradient sum ~= 1202.03.

This localizes the strongest numeric collapse to the bright exterior region.

## Honor-camera reference boundary

A normal Honor camera-app image was supplied as a visual example showing
visible structure in a bright white exterior surface.

Its classification is permanently:

`VISUAL_REFERENCE_ONLY`

It is not Source Evidence, calibration, a color target, an authority source or
proof of the correct physical scene radiance.

# Appearance Highlight Headroom Sweep v0.2

v0.2 has now been implemented as a parallel diagnostic.

The normal PRO preview remains unchanged.

The physical SDR display peak stays fixed at 100 nit. The sweep changes only
the appearance shoulder start:

- `baseline_100_100`;
- `shoulder_90_100`;
- `shoulder_80_100`;
- `shoulder_70_100`.

Every variant uses the existing Free-World Appearance Resolve v0.7.

Per variant, v0.2 records:

- mapped-at-peak count;
- peak-collapsed distinct neighbor pairs;
- collapse fraction;
- collapse reduction versus baseline;
- source/mapped luminance-gradient sums;
- gradient retention;
- existing gamut/display-clamp count;
- source CENSORED count;
- samples at/below the candidate knee;
- lower-range samples whose mapped luminance changed.

v0.2 deliberately selects **no automatic winner**.

Permanent v0.2 invariants:

- Scientific Master unchanged;
- TruthNegative unchanged;
- N2 unchanged;
- normal PRO preview unchanged;
- `automatic_winner_selected=false`;
- `source_scene_mutated=false`;
- `creates_new_evidence=false`;
- `scientific_writeback_allowed=false`.

# Validation

Appearance Headroom Sweep v0.2 standalone validation:

- run `36254075689`;
- GCC: SUCCESS;
- Clang: SUCCESS;
- ASan/UBSan: SUCCESS.

Full signed ARM64 Android build at code checkpoint
`402c72d6804f6cc393a5eab93c9d94d0687be111`:

- run `36254294042`: **SUCCESS**;
- N2 v0.2/v0.2.1/v0.3/v0.3.1 gates: SUCCESS;
- Appearance Highlight Detail v0.1 gate: SUCCESS;
- Appearance Headroom Sweep v0.2 gate: SUCCESS;
- stable signing identity: SUCCESS;
- `assembleDebug`: SUCCESS;
- new JNI symbol verification: SUCCESS;
- artifact upload: SUCCESS.

Artifact:

- ID: `10910177012`;
- name: `draw-appearance-highlight-headroom-v02-debug-arm64`;
- artifact ZIP digest:
  `sha256:d3a7fcebdf726790151ce27ee129473e3ec37ed855a958c1ba276720692f1255`;
- APK bytes: `6,951,735`;
- APK SHA-256:
  `d46b40c7650ad0b1102438bc1a31b1c4f8ee7c4c14050af633c435c5b0405e8b`;
- signing certificate SHA-256:
  `a6288a4b7e9d18e908eeba37540cd962b687f2290f7e45d7c7ad3390ad61fd44`.

# Immediate next gates

## Appearance

The real lamp-scene v0.2 sweep is now recorded at:

`docs/research/truthnegative-appearance-highlight-headroom-sweep-v0.2/evidence/DEVICE_RESULT_LAMP_SCENE_2026-09-26.md`

The 100/100 baseline produced 941 exact collapsed distinct neighbor pairs with
`source_censored=0`; 90/100 reduced that exact collapse count to zero while
leaving below-knee mapped luminance unchanged.

No default PRO promotion has occurred. A later appearance-only A/B/Delta may
compare 100/100 and 90/100. The unchanged 1058 gamut/display clamps are a
separate remaining issue.

## Lens-independent Free World

The next implementation step should instantiate the sealed Observation
Contract for additional real source routes, beginning with existing main,
ultra-wide and tele RAW/DNG admissions.

Each route must:

- retain separate sealed Source Evidence;
- build its own Source Capability Envelope;
- bind its own TruthNegative lineage;
- report gauge relation authority;
- refuse cross-observation radiometric fusion while common gauge is UNPROVEN.

No additional camera is required to validate the architecture itself; real
captures are required only to promote source-specific capability/calibration
claims.

# Default-main promotion boundary

Default GitHub `main` is intentionally not changed by this research step.

The current development line is coherent and Android-buildable, but promotion
of ~two weeks of accumulated research history into the default branch should
be a separate deliberate repository operation rather than an implicit side
effect of one scientific experiment.
