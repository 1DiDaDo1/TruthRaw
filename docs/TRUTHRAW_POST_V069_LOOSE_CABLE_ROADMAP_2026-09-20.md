# TruthRaw — post-v0.69 loose-cable roadmap

Date: 2026-09-20

Branch context:

`integration/truthraw-suite-v0-69-tn3-open-scene-projection`

Purpose: make the remaining disconnected or only partially bound project lines explicit so later chats do not silently reopen old architecture or treat research modules as already closed.

## Current connected spine

The active integration spine is now:

`SOURCE_EVIDENCE`
→ `MEASUREMENT / DE-ISP`
→ `SCIENTIFIC_MASTER`
→ `DYNAMIC_AUTHORITY / UNCERTAINTY / SUPPORT`
→ `TN-3 FULL OPEN SCENE STATE`
→ optional `FULL-RES RETREATABLE RESTORATION`
→ `DNG / TIFF / EXR PROJECTION`.

Permanent law remains:

> Measured where measured. Reconstructed where necessary. Never invented.

Representation may exceed the source; knowledge claims may not exceed evidence.

## Direct cable A — canonical Open Scene State binding

v0.69 TN-3 stores a dense full-frame Open Scene State bound to the same Scientific Master and Dynamic Authority lineage. This is now operational, but the Android TN-3 representation still has to be made one canonical schema with the recovered v0.7 Open Scene Region Runtime and v0.8 Open Scene State Stream.

Closure requirement:

- one versioned Open Scene schema;
- identical spatial coordinate identity across TN-3, Advanced, Restoration and downstream projections;
- colour, illumination, HDR, detail and restoration authority represented once rather than independently re-described;
- deterministic full-state digest;
- no counterfactual or appearance state may upgrade measured/reconstructed authority;
- fail closed on missing uncertainty/support needed for any stronger claim.

Status: **PARTIAL v0.70 — TN-3 and normal DNG/TIFF/EXR projection now share one native canonical Open Scene algorithm + artifact identity. Advanced runtime and the v0.67 .trr container still need the exact same artifact hash propagated.**

## Direct cable B — Restoration role-mask export binding

The `.trr` container keeps the full per-pixel Restoration role map inseparable from the Float32 derivative:

- role 0 = `PRESERVE_SCIENTIFIC_MASTER`;
- role 1 = `AESTHETIC_REINTEGRATION_ONLY`;
- role 2 = `UNRESOLVED_LOSS`.

v0.69 normal DNG/TIFF/EXR projections carry lineage and role-count provenance, but the complete per-pixel role mask still primarily lives in `.trr`.

Closure requirement:

- every normal export must cryptographically bind to the exact role-mask digest;
- where the target format safely supports it, embed the role mask or a lossless sidecar reference;
- if a format cannot preserve the mask internally, emit a versioned companion sidecar and bind both artifacts by hashes;
- flattening role-1 pixels into an ordinary image must never erase their derivative-only status;
- role-2 unresolved pixels must never be presented as successfully restored scientific truth.

Status: **PARTIAL v0.70 — DNG, TIFF and EXR now bind the exact full role-mask SHA-256. The complete mask bytes still live in .trr; self-contained embedding or a bound companion sidecar remains open.**

## Direct cable C — DNG / TIFF / EXR conformance

v0.69 includes full-resolution projection writers, but self-verification is not equivalent to external-format conformance.

Closure requirement:

- DNG: validate TIFF/DNG structure, LinearRaw/Float32 semantics and private TruthRaw provenance in independent readers;
- TIFF: validate tiled Float32 RGB, tags, orientation and metadata in at least one independent TIFF implementation;
- EXR: validate magic/header/channel layout, Float32 scanline data and custom provenance in an independent OpenEXR-compatible reader;
- round-trip pixel digest checks where the external reader exposes exact Float32 values;
- keep negative and >1 components where the format allows them;
- no hidden clipping/tone mapping;
- device-side saved file must match verified staging whole-file SHA-256.

Status: **OPEN / requires host + real-device artifact validation**.

## Direct cable D — effectful full-resolution Restoration validation

The real-device v0.68 4080×3072 artifact validated full container generation, lineage and finalization, but that source had:

`censored_source_pixels = 0`.

Therefore no role-1 or role-2 pixels were exercised.

Closure requirement:

- obtain or capture an admitted DNG with CFA samples at/above WhiteLevel;
- require `censored_source_pixels > 0`;
- only role-1 pixels may differ from the base Scientific Master;
- role-2 pixels must remain base/unresolved when support is insufficient;
- valid support pixels may never be overwritten;
- derivative digest and role-mask digest must be reproducible;
- projected DNG/TIFF/EXR must preserve the same derivative identity and role-mask binding.

Status: **OPEN / empirical source required**.

## Larger remaining project lines

These are important but are not prerequisites for declaring the v0.69 DNG-route spine internally connected.

### Current PTC / certificate schema

Historical `TRCERT01` remains legacy verify-only. A new production certificate must bind, at minimum:

- sealed source identity;
- Scientific Master identity;
- Dynamic Authority identity;
- canonical Open Scene State identity;
- optional Restoration derivative + role-mask identity;
- final projection identity;
- frame/evidence counts;
- forbidden flags and versioned semantics.

Do not reuse historical certificate semantics as if they already certify TN-3/Open Scene/Restoration.

### Multi-vendor scientific admission

DNG is the current complete scientific route. Nikon NEF has partial real decoding/radiometric gates, but still lacks the complete source-bound noise/uncertainty + colour + held-out validation package for Scientific Master admission. Canon/Sony/Fuji/other proprietary RAW routes remain separate adapter/calibration work.

### Full physical Scene Physics

Open Scene State can carry geometry/material/visibility/illumination variables, but single-frame physical relight authority must remain bounded by evidence. Do not promote inferred illumination or geometry into exact physical truth without separately admitted evidence.

## Required development order

1. finish and validate v0.69 Android build;
2. canonicalize TN-3 with Open Scene v0.7/v0.8;
3. bind full role-mask identity into DNG/TIFF/EXR outputs;
4. run independent format-conformance validation;
5. run an effectful clipped/censored Restoration device test;
6. then version a current PTC/certificate over the now-connected lineage;
7. continue multi-vendor and deeper physical Scene Physics independently.

## Do not regress

- v0.63 PURE Float32 scientific route remains frozen unless a separate evidence-backed migration is made;
- v0.67 Restoration science remains downstream and retreatable;
- v0.68 transactional foreground commit discipline remains the required Android long-export pattern;
- TN-3/Open Scene must not create a second scientific world;
- DNG/TIFF/EXR are projections, not new evidence;
- appearance/restoration/counterfactual state never writes authority backward into Scientific Master.
