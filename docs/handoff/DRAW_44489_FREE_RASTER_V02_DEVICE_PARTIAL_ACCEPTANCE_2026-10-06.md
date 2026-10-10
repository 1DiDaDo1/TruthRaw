# D.RAW 44489 — Free Raster v0.2 partial device acceptance — 2026-10-06

Continuation code: **44489**

This document records real-device acceptance evidence for the downstream Unified Output / external presentation raster -> Workspace -> Free Raster product path on PR #131.

## Repository/runtime reference

Runtime/build parent for the tested APK:

`7f7d3f896d5e48d31a5b0ab833b9fbfe53cca390`

PR #131 remains open, draft and unmerged. PR #130 / `4e4f358ac5d6fb4a8562286257b8edb76ae5d33f` remains the frozen scientific/audit reference and is not modified by this acceptance record.

The internal tested runtime cable is:

`UnifiedOutputPreviewResult.Ready -> MainActivity common state boundary -> UnifiedOutputPresentationBridge.publishReady(...) -> Workspace fail-closed consume -> Free Raster`

No second renderer, RAW/DNG decoder, Scientific Master, reconstruction route, T5 evaluation or output truth is introduced.

## Internal D.RAW device evidence

User-provided real-device screenshots on 2026-10-06 show internal D.RAW output in `Output / Vrije Raster · v0.2`.

Visible runtime binding:

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

Internal Fit works. Internal **Preview 1:1** works with the strict meaning:

`1 display pixel = 1 decoded preview pixel`

It is not one display pixel per sensor/CFA or Scientific-Master sample, not proof of optical resolution, and not new measured detail.

## External presentation raster device evidence

A later real-device round reloaded the normal black/white dog JPG in the same Workspace/app environment.

Observed external Fit state:

- canvas status: `EXTERNAL_PRESENTATION_RASTER`;
- source raster: `4080×3072 px`;
- decoded presentation preview: `2040×1536 px`;
- sample: `2×`;
- classification: `PRESENTATION_ONLY`;
- UI explicitly states no scientific authority/writeback;
- observed Fit scale: approximately `0.502x`.

Observed external Preview 1:1 state:

- canvas status: `PREVIEW_RASTER_1_TO_1`;
- semantics remain `1 display-pixel per decoded preview-pixel`;
- observed scale: `1.000x`;
- source/scientific sampling remains unchanged;
- classification remains `PRESENTATION_ONLY`.

A third screenshot proves interactive zoom/pan behavior:

- observed zoom scale: approximately `3.980x`;
- x/y transform values changed strongly relative to the 1:1 position;
- the enlarged eye region is visibly translated in the viewport;
- classification remains `PRESENTATION_ONLY`.

The raster was successfully reloaded in the same app/Workspace environment. This proves **same-environment reload**, not cold-start/process-death persistence. No claim is made that the Android process was killed or that persisted URI restoration after process death has been physically tested.

## Acceptance status

Physical product/runtime checks proven so far:

- [x] internal real D.RAW observation reaches Workspace through existing Unified Output Ready -> bridge -> Free Raster;
- [x] source/job/route metadata survives into the internal presentation snapshot;
- [x] source SHA is transported when already known upstream;
- [x] `VIEW_ONLY_COPY` boundary is visible on device;
- [x] `createsNewEvidence=false` remains visible;
- [x] `scientificWriteback=false` remains visible;
- [x] internal Fit;
- [x] internal Preview 1:1 with preview-pixel semantics;
- [x] external JPG/JPEG through `EXTERNAL_PRESENTATION_RASTER / PRESENTATION_ONLY`;
- [x] external Fit;
- [x] external Preview 1:1;
- [x] pan/zoom interaction;
- [x] same-environment external raster reload.

Still open before **full Free Raster v0.2 physical acceptance**:

- [ ] non-zero orientation round;
- [ ] route-mismatch fail-closed round;
- [ ] source switch/reprocess stale-state clearing round;
- [ ] explicit no-source-overwrite / no-Scientific-Master-writeback / no-candidate-application / no-new-evidence acceptance check across the full interaction;
- [ ] cold-start/process-death external raster restoration if that behavior is required as an accepted product property;
- [ ] exact installed-package identity readback from the physical device if byte-exact installed APK provenance is required.

Therefore the current correct status is:

`PARTIAL_DEVICE_ACCEPTANCE_INTERNAL_EXTERNAL_FIT_PREVIEW_1_TO_1_PAN_ZOOM_PASS`

This is **not yet** `FULL_V0_2_DEVICE_ACCEPTANCE`.

## Provenance precision

The exact branch build checkpoint recorded by PR #131 is:

- source head `7f7d3f896d5e48d31a5b0ab833b9fbfe53cca390`;
- `D.RAW Suite Universal Intake v0.1` run `37439775107` = SUCCESS;
- artifact ID `11401290002`;
- artifact ZIP SHA-256 `5ae0e7d9d86db230c2d76e5d86cb0521511c07dd7615c695e78782ba56e791a4`;
- APK bytes `8,702,803`;
- APK SHA-256 `d8baafc8715f41a5585a9e1687e8ac56c4ddb6d5f900321bb091b7fb16f1c8ee`;
- signing certificate SHA-256 `a6288a4b7e9d18e908eeba37540cd962b687f2290f7e45d7c7ad3390ad61fd44`;
- versionCode `26100127`;
- versionName `0.54-v0.84.2-open-world-authority-corridor-v01`.

The screenshots prove runtime behavior on a physical device. This document does not claim that the installed package hash was independently read back from the device during this chat. Functional device acceptance and byte-exact installed-package provenance remain distinct evidence classes.

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

Do not re-test the already-proven internal/external Fit, Preview-1:1 or pan/zoom paths unless runtime code changes.

Next highest-value acceptance round:

1. test a non-zero orientation source;
2. force a route mismatch and confirm Workspace rejects stale/mismatched presentation fail-closed;
3. switch/reprocess the D.RAW source and confirm stale internal output disappears;
4. confirm the source file remains untouched and no scientific/candidate/writeback/new-evidence state changes;
5. optionally test cold-start/process-death raster restoration and installed APK hash readback if those provenance/product properties are required.

PR #131 remains draft until the remaining physical acceptance checks are complete.
