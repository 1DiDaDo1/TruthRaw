# D.RAW 44489 — Radiometric Encoding Consistency v0.1 checkpoint

Date: 2026-10-11
PR: #131 `feat/draw-workspace-free-raster-v01`
Status at checkpoint: open, draft, unmerged; no scientific promotion performed.

## Exact heads

Previous documentation checkpoint:

`66af959bf8dfde7fc8fcdd7d1398d1a6984ce58f`

Staging / gate head:

`ba0013bfaea2838e3dfa55e97b6de4ab927054b0`

Exact validated runtime/code head:

`6b244b0496cd84b66454c23443aa7e03e5fd3bb8`

Runtime commit:

`android: add shadow DNG radiometric encoding consistency v0.1`

The guarded runtime commit changed exactly two runtime files:

- `suite_android/app/src/main/java/com/truthraw/adaptiveui/SourceCapabilityEnvelopeV02.kt`
- `suite_android/app/src/main/java/com/truthraw/adaptiveui/SourceCapabilityRadiometricEncodingConsistencyValidatorV01.kt`

The existing TIFF/DNG parser, source profiler, pixel routes, CFA evidence, Scientific Master and Appearance code were not changed by this runtime commit.

## Validation evidence

Dedicated workflow:

`D.RAW Source Capability Evidence Ledger v0.2`

Run:

`38100858612`

Job:

`114356322403`

Result: **SUCCESS**

Relevant exact evidence from the job:

- passive occurrence-ledger state retained;
- passive traversal-status state retained;
- shadow radiometric semantic validator retained;
- shadow Raw-IFD format-domain validator retained;
- radiometric encoding-consistency v0.1 patch applied;
- evidence-ledger static invariants: PASS;
- sealed Full-Frame Streaming integrity: `FULL_FRAME_STREAMING_V0_1_INTEGRITY_PASS`;
- complete Android unit-test suite: `BUILD SUCCESSFUL in 1m 57s`;
- `23 actionable tasks: 23 executed`;
- guarded runtime commit/push: SUCCESS.

No separate push-triggered Source Capability workflow exists on the bot runtime commit itself. Broad PR workflows may still run independently and must not be conflated with this dedicated gate.

## New shadow-only capability

Schema:

`D.RAW/SourceCapabilityRadiometricEncodingConsistency/0.1`

The validator consumes only metadata occurrences already decoded by the existing classic-TIFF/DNG parser. It performs no image-payload read and introduces no second parser, second source hash route, source mutation or pixel-processing route.

Per-IFD states:

- `VALIDATED_RADIOMETRIC_ENCODING_CONSISTENCY_SHADOW_ONLY`
- `CONTRADICTED_RADIOMETRIC_ENCODING_CONSISTENCY_FAIL_CLOSED`
- `UNRESOLVED_RADIOMETRIC_ENCODING_CONSISTENCY_FAIL_CLOSED`

## Validated format scope

The v0.1 layer checks only format/encoding consistency around:

- `SamplesPerPixel`;
- `BitsPerSample`;
- `SampleFormat`;
- explicit or default `BlackLevel` representability;
- explicit or default `WhiteLevel` representability;
- prior validated Raw-IFD format-domain status.

Rules implemented from the DNG 1.7.1.0 + TIFF 6.0 format contracts:

- absent `SamplesPerPixel` resolves to the TIFF default 1 without creating a source occurrence;
- absent `SampleFormat` resolves to TIFF default unsigned integer (`1`) without creating a source occurrence;
- DNG `BitsPerSample` is constrained to 8–32 bits;
- multi-sample DNG data must use the same bit depth for each sample;
- DNG floating-point SampleFormat uses only 16, 24 or 32 bits;
- absent TIFF `BitsPerSample` would resolve to TIFF default 1, which is explicitly classified as contradictory to the DNG 8–32-bit contract rather than silently accepted;
- absent `BlackLevel` resolves to DNG default zero without minting physical evidence;
- absent `WhiteLevel` resolves per sample to `2^BitsPerSample - 1` for unsigned integer encoding or `1.0` for floating-point encoding, again without minting a source occurrence;
- explicit unsigned BlackLevel/WhiteLevel values are checked against the representable unsigned encoding range;
- floating-point BlackLevel/WhiteLevel values are not incorrectly constrained by the integer `2^bits-1` ceiling;
- duplicate dependency occurrences never get a silent first-found winner.

## Explicit scientific boundary

The runtime emits and preserves:

- `defaults_mint_source_occurrence=false`
- `duplicate_occurrence_selection_allowed=false`
- `declared_dng_version_compatibility_validated=false`
- `black_level_delta_h_v_included=false`
- `effective_normalization_denominator_validated=false`
- `image_sample_payload_read=false`
- `record_lifecycle_effect=false`
- `scientific_admission_effect=false`
- `pixel_route_effect=false`
- `scientific_master_pixel_effect=false`
- `appearance_pixel_effect=false`
- `creates_new_evidence=false`
- `scientific_writeback_allowed=false`

Per IFD it also records:

- `black_white_ordering_validated=false`
- `source_values_modified=false`
- `domain_validation_is_scientific_admission=false`

Therefore this checkpoint does **not** establish that BlackLevel/WhiteLevel are scientifically admitted calibrations and does **not** apply them to source pixels or Scientific Master.

## Deliberately not claimed

v0.1 does not validate:

1. whether the file's declared `DNGVersion` / `DNGBackwardVersion` is compatible with every used encoding feature;
2. `BlackLevelDeltaH` / `BlackLevelDeltaV`;
3. the complete effective black field;
4. the effective `(WhiteLevel - BlackLevel)` normalization denominator;
5. `BlackLevel < WhiteLevel` as a general scientific rule;
6. image-sample payload values against the declared encoding;
7. scientific admission, calibration promotion or Scientific Master writeback.

The Black/White ordering is intentionally not inferred from baseline `BlackLevel` alone because the DNG normalization model can include row/column black-level deltas. Knowledge must not exceed the evidence and dependencies actually validated.

## Targeted tests added

The new unit coverage includes:

1. valid 12-bit unsigned Raw-IFD with explicit BlackLevel/WhiteLevel;
2. unsigned WhiteLevel beyond a 12-bit encoding range -> fail closed;
3. floating LinearRaw with absent WhiteLevel -> float default 1.0;
4. invalid 20-bit floating encoding -> fail closed;
5. duplicate BitsPerSample -> unresolved/no selection;
6. absent BitsPerSample -> TIFF default 1 but DNG contradiction;
7. multi-sample equal versus unequal bit-depth vectors;
8. rational BlackLevel outside the unsigned encoding range;
9. radiometric metadata in a reduced/non-Raw IFD -> contradicted while ledger admission stays unchanged.

## Next safe boundary

Do not apply BlackLevel or WhiteLevel to pixels yet.

The next narrow metadata-only gap is declared DNG-version compatibility: determine whether encoding features used by a Raw-IFD are legal for the file's declared DNG version/backward-version without altering evidence, admission or pixels. After that, the separate effective-black dependency layer can incorporate `BlackLevelDeltaH` / `BlackLevelDeltaV` before any normalization or admission discussion.

Permanent project laws remain unchanged:

`MEASURED != CALIBRATED_ESTIMATE != RECONSTRUCTED != CENSORED != UNKNOWN != APPEARANCE`

**One Free World. Many sealed observations. One evidence law.**
