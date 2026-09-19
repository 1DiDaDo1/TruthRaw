# TruthNegative current research architecture — 2026-09-19

Status: **CURRENT TRUTHNEGATIVE RESEARCH-BRANCH ARCHITECTURE / v0.2 ALIGNMENT / NOT MAIN PROMOTION**

Branch:

`research/truthnegative-v0-1-scientific-negative-foundation`

## 1. Correction from the first v0.1 draft

The first v0.1 draft described TruthNegative as if it were a new reconstruction stage inserted between Measurement and the Scientific Master.

Project review and recovery of the previous two chats show that this would duplicate architecture TruthRaw already had.

The project already contains the complete conceptual route:

`sealed source RAW/CFA`
`-> measurement/de-ISP`
`-> Latent Camera Scene / measured-preserving reconstruction`
`-> camera-native scene-linear Scientific Master`
`-> Dynamic Authority / uncertainty / support`
`-> Open Scene / Free Scientific Space`
`-> appearance / export`

TruthNegative v0.2 therefore binds to the **existing reconstructed house** rather than building another one.

## 2. Any-brand source architecture

TruthNegative is not HONOR-specific.

An admitted RAW may enter through one of two scientific ingress classes:

### Native/certified source

A directly supported source reaches the Main House through a source adapter such as the strict Tile-Native DNG Source / `IRawTileSource` route.

### External/vendor RAW

A non-native RAW format uses the recovered Gatehouse architecture:

`external/vendor RAW -> isolated decoder/Gatehouse -> sealed Decoded Measurement Handoff -> IRawTileSource / Main House`

The Gatehouse is an intermediate admission/translation route only. It is never a second truth endpoint.

The current branch contains native DNG tile-source and generic URI-ingress code. The full external-RAW Gatehouse is an architectural requirement recovered from prior project work and is not falsely claimed as fully reimplemented here.

## 3. Existing new house

The existing new house is already float/scientific.

Current project facts:

- v4.7i reconstructs camera-native full-colour scene-linear RGB before camera-to-XYZ and before appearance;
- the Scientific Master digest contract identifies exact IEEE-754 binary32 camera-native RGB sample bits;
- branch-sensitive reconstruction, calibration, optimization and covariance use F64 where required;
- controlled F32 storage is allowed only after the F64-compute/storage validation gate;
- higher precision is a validator, not new evidence;
- Free Scientific Space can exceed RAW10, WhiteLevel, ISO scale, [0,1], the source Bayer lattice, SDR and DNG limits.

Therefore the user’s “sealed whole RAW house -> new TruthRaw Float32/Float64 rawsensor-filled open world” is fundamentally already present in the project, with one terminology correction:

**the richest scientific house is not required to remain a rawsensor/Bayer mosaic. Its current authoritative master is camera-native reconstructed RGB.**

## 4. TruthNegative v0.2 role

TruthNegative has two coupled but distinct objects.

### 4.1 TruthNegative Core

A scientific-negative binding of the already reconstructed house.

It binds:

- source evidence identity;
- ingress/admission class;
- measurement/reconstruction lineage;
- Scientific Master identity;
- precision policy;
- Dynamic Authority / uncertainty / support identity;
- one-frame/one-evidence invariants.

It creates no new scientific world.

### 4.2 TruthNegative Sensor Projection

An optional reconstructed sensor-like negative derived from the bound reconstructed state.

It may be:

- source-sized;
- 2x linear;
- 4x linear;
- `16320x12288`;
- another finite projection grid.

It can carry a CFA-like arrangement if a specific projection contract requires that representation.

But every newly created target site remains `RECONSTRUCTED`.

The projection:

- is not the original RAW;
- is not the Scientific Master;
- does not prove hidden photodiodes;
- does not increase physical frame count;
- does not increase independent evidence count.

## 5. Coordinate domains

TruthNegative keeps three coordinate concepts separate:

1. **Source Evidence Grid** — original admitted sample lattice.
2. **Reconstructed Camera/Scene Domain** — the existing latent/full-colour scientific state.
3. **Sensor-Negative Projection Grid** — an optional finite resampling/remosaic of that state.

For Camera 5:

source candidate domain:
`4080x3072`

dense research projection:
`16320x12288`

The 4x linear relation is not physical pixel-location proof.

## 6. Precision inheritance

TruthNegative does not invent a new precision policy.

Inherited policy:

`exact packed/integer source evidence`
`-> F32 only where proven safe`
`-> F64 branch-sensitive reconstruction`
`-> F64 calibration / optimization / covariance`
`-> validated F32 Scientific-Master storage`
`-> higher/arbitrary precision reference validation`

A future F64 storage profile may be researched, but more storage bits may not be described as more evidence.

## 7. Implementation status

### TN-0 — authority foundation

Implemented:

- `tools/truthnegative_foundation_v01.py`
- `tests/test_truthnegative_foundation_v01.py`

Purpose: evidence-count and dense-projection fail-closed rules.

### TN-0.2 — existing-house binding

Implemented:

- `tools/truthnegative_existing_house_binding_v02.py`
- `tests/test_truthnegative_existing_house_binding_v02.py`
- `docs/TRUTHNEGATIVE_EXISTING_HOUSE_ALIGNMENT_AUDIT_2026-09-19.md`
- `docs/research/truthnegative-v0.2/README.md`
- `state/TRUTHNEGATIVE_V0_2_EXISTING_HOUSE_BINDING_STATE_2026-09-19.json`

Purpose: bind TruthNegative to the existing Scientific Master rather than duplicate it.

## 8. Revised implementation ladder

### TN-1 — real source + master binding — IMPLEMENTED / CI PASS

The already frozen 4080x3072 lineage is now bound without changing pixels or authority:

- source SHA-256: `7930ba5d...159b67`;
- decoded CFA SHA-256: `883cbe13...d719c`;
- Scientific Master SHA-256: `a86034da...4640`;
- Dynamic Authority SHA-256: `7678a0b1...8098`.

Files:

- `state/TRUTHNEGATIVE_TN1_REAL_SOURCE_MASTER_BINDING_2026-09-19.json`;
- `docs/research/truthnegative-v0.2/TN1_REAL_SOURCE_MASTER_BINDING_2026-09-19.md`;
- `tools/validate_truthnegative_tn1_binding.py`.

Dedicated CI passes. TN-1 creates no new pixel reconstruction; it proves TruthNegative can attach to the existing float reconstructed house without redefining it.

### TN-2 — source-resolution scientific-negative identity

Create a no-resampling TruthNegative representation at source spatial extent from the existing master and prove exact deterministic identity/provenance.

This is **not** a Bayer remosaic requirement; the core representation should remain camera-native full-colour unless a sensor-projection output is explicitly requested.

### TN-3 — continuous/dense reconstruction projection

Define a deterministic resampling/reconstruction operator from the existing scientific state to a denser finite grid.

### TN-4 — uncertainty/support projection

Project/propagate Dynamic Authority, uncertainty/covariance, censor and unknown status onto the dense negative.

### TN-5 — optics-aware constraint

Bind PSF/MTF/CA/shading only when independently calibrated.

### TN-6 — 16320x12288 sensor-negative experiment

Produce a reconstructed `16320x12288` sensor-like negative while preserving:

- source lineage;
- master identity;
- authority map;
- uncertainty/support;
- no physical-pixel claim.

### TN-7 — regression against Scientific Master

Prove the negative can never mutate/redefine the master or source evidence.

### TN-8 — compatibility/export

Optional DNG/EXR/TIFF/rawsensor-like export remains downstream.

## 9. Validation gates

Before a dense TruthNegative is scientifically useful:

- exact source/master identity binding;
- deterministic output under fixed source/master/config;
- no source mutation;
- no master mutation;
- no measured-authority inflation;
- correct uncertainty/support propagation;
- no appearance dependence;
- no generative semantic fill in scientific mode;
- precision regression;
- source-resolution/reprojection consistency;
- held-out synthetic forward-model tests;
- real-source CFA/RGB/noise/edge residual tests.

## 10. Parallel HONOR route

The independent HONOR route continues to investigate the real acquisition pipeline.

TruthNegative may consume stronger evidence later if HONOR exposes it.

TruthNegative may not be used to prove that HONOR had hidden samples upstream.

## 11. Current next implementation

Proceed with **TN-2 source-resolution scientific-negative identity**.

TN-1 is now closed as an identity/lineage binding.

TN-2 must serialize or expose the existing source-resolution camera-native Scientific Master as a TruthNegative view with:

- zero pixel changes;
- exact Scientific Master hash binding;
- exact Dynamic Authority binding;
- no remosaic requirement;
- no spatial enlargement;
- deterministic identity independent of runtime tile size.

Only after TN-2 is exact should dense spatial reconstruction begin.