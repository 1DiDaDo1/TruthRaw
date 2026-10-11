# D.RAW 44489 — DNG Version Compatibility v0.1 checkpoint

Date: 2026-10-11
PR: #131 `feat/draw-workspace-free-raster-v01`
Status at checkpoint: open, draft, unmerged; no scientific promotion performed.

## Exact heads

Previous 44489 documentation checkpoint:

`557559f002f48c41f18f8ca9ff44f75c98add6e1`

Previous exact validated radiometric-encoding runtime:

`6b244b0496cd84b66454c23443aa7e03e5fd3bb8`

DNG-version staging / gate head:

`300dd763d5dcf8d4d492b8dd6c33d425ee8a73f1`

Exact validated DNG-version runtime/code head:

`821cb91728c201ef4d175f1cb411321bbc4b58d0`

Runtime commit:

`android: add shadow DNG version compatibility v0.1`

The guarded runtime commit changed only:

- `suite_android/app/src/main/java/com/truthraw/adaptiveui/SourceCapabilityEnvelopeV02.kt`
- `suite_android/app/src/main/java/com/truthraw/adaptiveui/SourceCapabilityDngVersionCompatibilityValidatorV01.kt`

The existing TIFF/DNG parser, sealed-source profiler, CFA evidence, image-sample routes, Scientific Master, reconstruction and Appearance code were not modified by this runtime commit.

## Validation evidence

Dedicated workflow:

`D.RAW Source Capability Evidence Ledger v0.2`

Run:

`38101309816`

Job:

`114357627447`

Result: **SUCCESS**

Exact relevant evidence:

- occurrence-ledger state retained;
- traversal-status state retained;
- radiometric semantic validator retained;
- Raw-IFD format-domain validator retained;
- radiometric encoding-consistency validator retained;
- DNG-version compatibility v0.1 patch applied;
- static evidence/firewall invariants: PASS;
- sealed Full-Frame Streaming integrity: `FULL_FRAME_STREAMING_V0_1_INTEGRITY_PASS`;
- complete Android unit-test suite: `BUILD SUCCESSFUL in 1m 54s`;
- `23 actionable tasks: 23 executed`;
- guarded runtime commit/push: SUCCESS.

Broad PR workflows triggered by the bot runtime push are independent and must not be conflated with this dedicated Source Capability gate.

## Specification scope

Anchor: Adobe DNG Specification 1.7.1.0.

The validator uses the already-decoded physical TIFF/DNG occurrences for:

- `DNGVersion` (tag 50706, BYTE x4, required in IFD0);
- `DNGBackwardVersion` (tag 50707, BYTE x4; specification default derives from DNGVersion with the last two version bytes zero);
- already validated Raw-IFD domain state;
- already validated SampleFormat/encoding state.

No second TIFF/DNG parser or second source-hash route was introduced.

## New shadow-only capability

Schema:

`D.RAW/SourceCapabilityDngVersionCompatibility/0.1`

States:

- `VALIDATED_SCOPED_DNG_VERSION_COMPATIBILITY_SHADOW_ONLY`
- `CONTRADICTED_SCOPED_DNG_VERSION_COMPATIBILITY_FAIL_CLOSED`
- `UNRESOLVED_SCOPED_DNG_VERSION_COMPATIBILITY_FAIL_CLOSED`

### DNGVersion

- exactly one IFD0 identity is required for this scoped interpretation;
- DNGVersion is required;
- its physical occurrence must be BYTE x4 and decoded;
- duplicate occurrences never get a silent first-found winner;
- a declared version before DNG 1.0.0.0 is contradictory;
- future DNGVersion numbers are not automatically claimed conformant.

### DNGBackwardVersion

- an explicit occurrence must be unique BYTE x4 and decoded;
- if absent and DNGVersion is valid, the specification default is derived as `<major>.<minor>.0.0`;
- this default never mints a source occurrence;
- the validator's reader-spec anchor is DNG 1.7.1.0, so a backward-version requirement above 1.7.1.0 is fail-closed for this validator rather than guessed compatible.

### Floating-point Raw feature floor

DNG floating-point image data is treated as a DNG 1.4.0.0-or-later feature.

The validator detects the feature only when:

1. the IFD is already a validated Raw-IFD format domain; and
2. the existing encoding-consistency layer has identified floating-point SampleFormat.

For such an IFD:

- declared DNGVersion must be at least 1.4.0.0;
- the DNGBackwardVersion >= 1.4.0.0 floor is applied only when the main-image relation is actually proven by the current model;
- in v0.1, a validated Raw IFD that is exactly `IFD0` is treated as the proven main-image relation for this scoped test;
- a floating Raw SubIFD without that proof becomes `UNRESOLVED_MAIN_IMAGE_RELATION_NOT_PROVEN`;
- the validator explicitly never assumes that the first Raw candidate is the main image.

## Explicit non-claims / firewall

The runtime records:

- `floating_point_feature_floor_validated=true`
- `compression_feature_floors_validated=false`
- `opcode_feature_floors_validated=false`
- `whole_file_dng_conformance_validated=false`
- `future_unknown_feature_set_exhaustively_validated=false`
- `main_image_ifd_inferred_by_first_raw_candidate=false`
- `defaults_mint_source_occurrence=false`
- `duplicate_occurrence_selection_allowed=false`
- `record_lifecycle_effect=false`
- `scientific_admission_effect=false`
- `image_sample_payload_read=false`
- `pixel_route_effect=false`
- `scientific_master_pixel_effect=false`
- `appearance_pixel_effect=false`
- `creates_new_evidence=false`
- `scientific_writeback_allowed=false`

Feature detection additionally records:

`feature_detection_is_scientific_admission=false`

Therefore this v0.1 result is only a scoped format-compatibility statement. It is not whole-file DNG conformance, not proof of a physically correct calibration, not pixel evidence, and not scientific admission.

## Targeted tests added

1. unsigned Raw with DNG 1.3 validates within this scoped version model and derives BackwardVersion without a physical occurrence;
2. floating Raw in IFD0 with DNGVersion 1.4 and BackwardVersion 1.4 validates;
3. floating Raw with DNGVersion 1.3 is contradicted;
4. floating main IFD with BackwardVersion 1.3 is contradicted;
5. floating Raw SubIFD does not guess main-image identity and remains unresolved for the backward-version feature floor;
6. missing required DNGVersion fails closed;
7. duplicate DNGVersion occurrences remain unresolved/no selection;
8. DNGBackwardVersion above the validator's 1.7.1 reader basis fails reader-side closed without claiming whole-file nonconformance;
9. absent BackwardVersion for DNGVersion 1.4.2.3 resolves to the specification default 1.4.0.0 without minting a source occurrence.

## Current Source Capability chain

The metadata-only path now has these independently bounded layers:

`sealed source binding -> physical TIFF occurrence ledger -> traversal integrity -> Raw-IFD format domain -> radiometric semantic contracts -> radiometric encoding consistency -> scoped DNG version compatibility`

None of these layers changes source samples or grants scientific pixel authority.

## Next safe boundary

Do not apply BlackLevel or WhiteLevel to pixels yet.

For the radiometric path, the next useful metadata-only step is effective-black dependency completeness:

- `BlackLevelDeltaH`;
- `BlackLevelDeltaV`;
- their type/count/dimension dependencies;
- the spatial effective-black field relative to the relevant Raw-IFD/active image domain;
- only after that, a fail-closed check of the effective `(WhiteLevel - BlackLevel)` normalization denominator can be designed.

Compression/JPEG XL/opcode version floors remain a separate compatibility workstream and should not be silently folded into radiometric admission.

Permanent evidence law remains unchanged:

`MEASURED != CALIBRATED_ESTIMATE != RECONSTRUCTED != CENSORED != UNKNOWN != APPEARANCE`

**One Free World. Many sealed observations. One evidence law.**
