# D.RAW 44489 — Preview-independent full-resolution JPEG checkpoint — 2026-10-06

Continuation code: **44489**

This checkpoint supersedes the older build-checkpoint details in the same-day Workspace / Free Raster handoff. It does **not** replace the Free Raster device-acceptance record, and it does not promote scientific authority.

## Repository boundary

- repository: `1DiDaDo1/TruthRaw`
- active product/runtime PR: **#131**
- branch: `feat/draw-workspace-free-raster-v01`
- PR state: open, draft, unmerged
- frozen scientific/audit reference: PR #130 / `4e4f358ac5d6fb4a8562286257b8edb76ae5d33f`
- exact runtime source head for this checkpoint: `de0d49ef9f9abda777a8126a2a98a028030eafa1`

## Preview-independent JPEG cable

Full-resolution JPEG is now bound to the admitted DNG/source/output context instead of to `TilePreviewUiState.Ready`.

Stable outer binding:

`admitted DNG observation -> DrawPhotoOutputCableV01 binding -> FullResJpegExporter -> new derived JPEG`

The UI preview is a sibling presentation output only. It is neither the pixel source nor authority for this full-resolution export.

Runtime anchors:

- `suite_android/app/src/main/java/com/truthraw/adaptiveui/DrawPhotoOutputCableV01.kt`
- `suite_android/app/src/main/java/com/truthraw/adaptiveui/MainActivity.kt`

Hard properties:

- `previewRequired=false`
- `createsNewEvidence=false`
- `scientificWritebackAllowed=false`
- `sourceMutationAllowed=false`
- output authority = `DERIVED_PRESENTATION_OUTPUT`
- source authority = `EXISTING_ADMITTED_DNG_OBSERVATION`
- adapter = `jpeg.full_resolution.native_dng.v1`

Before Android's document picker opens, the request freezes source job, source URI, PURE/ADVANCED/PRO route, route flags and downstream quarter-turn orientation. When the picker returns, those values are revalidated fail-closed. A changed source/job/route/appearance/orientation blocks export instead of silently retargeting it.

The exporter call remains:

`FullResJpegExporter.renderToPrivateJpeg(contentResolver, job, flags, quarterTurns, dir)`

After a successful committed JPEG, loading a small saved-JPEG preview is best-effort presentation feedback only.

## Exact Android build checkpoint

`D.RAW Suite Universal Intake v0.1` run **37486202104**: **SUCCESS**.

- exact runtime source head: `de0d49ef9f9abda777a8126a2a98a028030eafa1`
- artifact: `DRAW_Full_Suite_Universal_Intake_v0.1_debug_arm64`
- artifact ID: `11423262790`
- artifact ZIP bytes: `3,286,459`
- artifact ZIP SHA-256: `1339acd7093d51fb5964d5a95d07a091e77b192dbb649c0bd0f14753628dbb34`
- APK bytes: `8,719,187`
- APK SHA-256: `9887085f71a24ceb0057f2503728e886502d11b92f908f23a00f107eab3ec860`
- stable signing certificate SHA-256: `a6288a4b7e9d18e908eeba37540cd962b687f2290f7e45d7c7ad3390ad61fd44`
- versionCode: `26100127`
- versionName: `0.54-v0.84.2-open-world-authority-corridor-v01`

The build completed in 2m48s and the workflow's APK verification confirmed the expected arm64 native export symbol and the stable signing certificate.

## Exact-head CI interpretation

On runtime source head `de0d49…`, the Android build and the returned scientific/contract workflows were successful except Documentation Governance. That single documentation failure was **not** a runtime/scientific failure: the verifier could no longer discover preserved historical bootstrap pointers after the current 44489 bootstrap had been modernized.

The compatibility-only documentation repair was committed at:

`c2e5e7372b62afed6769d7ce11f35e5f85cc6d14`

Documentation Governance run **37496655736** then completed **SUCCESS**, including both document-authority verification and current-state JSON validation.

No sealed evidence, Scientific Master, T5, reconstruction result, measured anchor, candidate state or scientific promotion state was changed by that repair.

## Real-device validation still required

Use the APK from runtime source head `de0d49…`.

Primary new acceptance test:

1. Import/admit a real DNG.
2. Invoke **JPG · full resolution** without depending on a completed `TilePreviewUiState.Ready` preview; the strongest check is to start the export while the normal preview is still idle/loading.
3. Save to a new JPEG document.
4. Confirm status reports `JPG full-resolution gereed` and expected full-resolution dimensions/orientation.
5. Confirm the original DNG still exists unchanged and the export is a new derivative.
6. Confirm no Scientific Master writeback, candidate application or new-evidence claim occurs.

Do not treat the post-export 384px/small preview as export evidence; it is presentation feedback only.

The earlier Free Raster v0.2 physical-acceptance remainder is still open: non-zero orientation, route-mismatch fail-closed, source-switch/reprocess stale-state clearing, and the explicit no-mutation/no-new-evidence interaction round.

Keep PR #131 draft until those physical checks are completed.

## Permanent boundary

`MEASURED != CALIBRATED_ESTIMATE != RECONSTRUCTED != CENSORED != UNKNOWN != APPEARANCE`

**Stable outside. Flexible inside. Evidence law unchanged.**

Preview independence improves product architecture only. It does not create scientific authority, fabricate sensor detail, increase optical resolution, modify sealed CFA evidence or redefine the Scientific Master.
