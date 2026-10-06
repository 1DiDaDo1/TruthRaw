# D.RAW 44489 — Free Raster v0.2 partial device acceptance — 2026-10-06

Continuation code: **44489**

This document records the first real-device acceptance evidence for the completed downstream Unified Output -> Workspace -> Free Raster cable on PR #131.

## Repository/runtime reference

Runtime/build parent immediately before this documentation record:

`7f7d3f896d5e48d31a5b0ab833b9fbfe53cca390`

PR #131 remains open, draft and unmerged. PR #130 / `4e4f358ac5d6fb4a8562286257b8edb76ae5d33f` remains the frozen scientific/audit reference and is not modified by this acceptance record.

The tested runtime cable is:

`UnifiedOutputPreviewResult.Ready -> MainActivity common state boundary -> UnifiedOutputPresentationBridge.publishReady(...) -> Workspace fail-closed consume -> Free Raster`

No second renderer, RAW/DNG decoder, Scientific Master, reconstruction route, T5 evaluation or output truth is introduced.

## Device evidence received

User-provided real-device screenshots on 2026-10-06 show the internal D.RAW output in `Output / Vrije Raster · v0.2`.

Visible runtime binding in the Fit screenshot:

- canvas status: `D.RAW_UNIFIED_OUTPUT_PRESENTATION`;
- route: `PRO`;
- output label: `D.RAWnegative v0.1 · Authority-bound Appearance View`;
- source display name: `IMG_BNC_TRUTHRAW20260907_094414_423.dng`;
- source raster: `4080×3072 px`;
- decoded Unified Output preview: `192×145 px`;
- display rotation: `0°`;
- source SHA-256 is present in the UI, shown with prefix `578fad42dad6819b…`;
- presentation classification: `VIEW_ONLY_COPY`;
- `createsNewEvidence=false`;
- `scientificWriteback=false`.

The Fit screenshot also shows the decoded `192×145` preview scaled to the available presentation canvas. This is presentation geometry only.

A second user-provided screenshot tests **Preview 1:1**. The intended and accepted meaning is strictly:

`1 display pixel = 1 decoded preview pixel`

It is **not**:

- one display pixel per sensor/CFA sample;
- one display pixel per Scientific Master sample;
- proof of optical resolution;
- new measured detail.

## Acceptance status

Physical product/runtime checks proven in this round:

- [x] internal real D.RAW observation reaches Workspace through existing Unified Output Ready -> bridge -> Free Raster;
- [x] source/job/route metadata survives into the presentation snapshot;
- [x] source SHA is transported when already known upstream;
- [x] `VIEW_ONLY_COPY` boundary is visible on device;
- [x] `createsNewEvidence=false` remains visible;
- [x] `scientificWriteback=false` remains visible;
- [x] Fit presentation works on device;
- [x] Preview 1:1 works on device with preview-pixel semantics.

Still open before **full Free Raster v0.2 physical acceptance**:

- [ ] normal JPG/PNG/WebP through external `EXTERNAL_PRESENTATION_RASTER / PRESENTATION_ONLY` path;
- [ ] pan/zoom interaction round;
- [ ] non-zero orientation round;
- [ ] route-mismatch fail-closed round;
- [ ] source switch/reprocess stale-state clearing round;
- [ ] explicit no-source-overwrite / no-Scientific-Master-writeback / no-candidate-application acceptance check across the full workbench interaction;
- [ ] exact installed-package identity readback from the physical device if byte-exact installed APK provenance is required.

Therefore the correct status is:

`PARTIAL_DEVICE_ACCEPTANCE_INTERNAL_D_RAW_FIT_PREVIEW_1_TO_1_PASS`

This is **not yet** `FULL_V0_2_DEVICE_ACCEPTANCE`.

## Provenance precision

The current exact branch build checkpoint recorded by PR #131 is:

- source head `7f7d3f896d5e48d31a5b0ab833b9fbfe53cca390`;
- `D.RAW Suite Universal Intake v0.1` run `37439775107` = SUCCESS;
- artifact ID `11401290002`;
- artifact ZIP SHA-256 `5ae0e7d9d86db230c2d76e5d86cb0521511c07dd7615c695e78782ba56e791a4`;
- APK bytes `8,702,803`;
- APK SHA-256 `d8baafc8715f41a5585a9e1687e8ac56c4ddb6d5f900321bb091b7fb16f1c8ee`;
- signing certificate SHA-256 `a6288a4b7e9d18e908eeba37540cd962b687f2290f7e45d7c7ad3390ad61fd44`;
- versionCode `26100127`;
- versionName `0.54-v0.84.2-open-world-authority-corridor-v01`.

The screenshots prove the runtime feature on a physical device. This document does **not** claim that the installed package hash was independently read back from the device during this chat. Functional device acceptance and byte-exact installed-package provenance remain distinct evidence classes.

## Scientific boundary

This acceptance record is product/runtime evidence only. It creates no observation-specific scientific evidence and grants no scientific promotion.

Permanent boundaries remain:

- sealed source/CFA evidence is immutable;
- `MEASURED != CALIBRATED_ESTIMATE != RECONSTRUCTED != CENSORED != UNKNOWN != APPEARANCE`;
- Free Raster is downstream View/Output/Projection;
- `UNKNOWN` remains valid;
- `SOURCE_MUTATION_ALLOWED=false`;
- `SCIENTIFIC_MASTER_WRITEBACK_ALLOWED=false`;
- `OVERWRITE_SOURCE_ON_EXPORT_ALLOWED=false`;
- no UI, performance, appearance or presentation path may create scientific authority.

## Next physical test

Continue from this exact partial acceptance point. Do not re-test the already-proven internal Fit/Preview-1:1 path unless a new runtime build changes it.

Next highest-value acceptance round:

1. external normal JPG through `PRESENTATION_ONLY`;
2. pan + pinch zoom;
3. non-zero orientation;
4. route mismatch fail-closed;
5. switch/reprocess source and verify stale internal output disappears;
6. confirm source file remains untouched and no scientific/candidate/writeback state changes.

PR #131 remains draft until the remaining physical acceptance checks are complete.