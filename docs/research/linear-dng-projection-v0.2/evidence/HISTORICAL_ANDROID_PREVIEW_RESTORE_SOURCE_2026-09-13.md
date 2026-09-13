# Historical Android preview restoration source — 2026-09-13

This evidence note records the previously solved TruthRaw DNG preview regression that must be reapplied to the current RGB / LinearRaw restoration branch.

## Historical source documents recovered

Primary historical records:

- `TRUTHRAW_V07_ANDROID_PREVIEW_FIX_REPORT.md`
- `TRUTHRAW_COMPLETE_HANDOFF_2026-09-10.md`
- historical v0.7 real-life LinearRaw validation records

These records establish that the earlier preview failure was a **container / preview-discovery regression**, not a failure of the reconstructed RGB pixels.

## Historical failure mode

The earlier v0.7 real-life DNG placed a large `1600x1203` RGB image uncompressed in IFD0.

Android/piex preview discovery did not reliably expose that as the DNG preview. The historical project concluded that a large DNG preview intended for Android discovery needed the JPEG path.

## Historical corrected layout

The successful corrected structure was:

- **IFD0**: `385x512` reduced RGB thumbnail, uncompressed, `NewSubFileType=1`;
- **SubIFD0**: full `4080x3072x3` 16-bit TruthRaw LinearRaw primary payload;
- **SubIFD1**: `1600x1203` JPEG-compressed Colorimetric-V3 L1 preview;
- preview `PhotometricInterpretation=YCbCr`;
- preview `PreviewColorSpace=sRGB`.

The preview is appearance/display data only. It never becomes scientific evidence and never changes the LinearRaw payload.

## Payload preservation proof in the historical fix

The full LinearRaw arrays were re-read from the old and preview-corrected files and compared exactly. Historical result: exact equality for all three tested captures.

Recorded LinearRaw payload SHA-256 values:

- capture `094414`: `d50ffe9a3ea1731120de19a117c13c4261be17e9acfc58c36f45f4848632aa51`
- capture `094416`: `493144fcb0d46e6e7c507d1e4d4551d4ddd2f0a7a0409c9b3bd1b44ee31f69c8`
- capture `094423`: `261dbbbd74ace9b2eaf9f3c7ee90a3e8ac86dda94c79beeb5fec5b2f47a80a96`

## Exact historical corrected DNG hashes

- `094414`: `2d5898e917e31beb087b775d398b65658d2602cbad66342d705c1cf629c6783b`
- `094416`: `fa5aeb5f1d2dd4cd62b3dddb1f444bae97d79aca5ef8f82dc1e41a50968723b4`
- `094423`: `0df692e24af17e8c8c7b9dc06783500258fbd7d021f646421ccb571e22ce4d44`

Historical DNG SDK result for every corrected file:

- Adobe DNG SDK `dng_validate` version `1.7.1.2724`;
- exit code `0`;
- errors `0`;
- warnings `0`;
- `Validation complete`;
- embedded JPEG preview was read back from the final DNG and opened successfully.

This is interoperability evidence for those historical exact files, not Adobe certification.

## Relationship to current v0.2 restoration

The current 2026-09-13 Work-derived v0.1 writer writes only a single primary LinearRaw IFD and therefore omitted the already discovered Android-preview container solution.

The current restoration sequence is therefore:

1. restore RGB finite headroom and source camera identity without changing Scientific Master semantics;
2. keep the primary RGB LinearRaw payload authoritative only as a compatibility projection;
3. restore the historical multi-IFD preview layout downstream of the same finalized source-bound release;
4. generate preview pixels through a display-only color route;
5. prove the LinearRaw payload is identical before and after adding the preview container structure;
6. validate the exact new file in Android/piex/Lightroom and independent DNG readers.

## Preview color authority

The historical preview was described as Colorimetric-V3 L1 / source-bound color. That distinction remains important:

- source DNG metadata may provide a valid source-bound preview transform;
- it is not independent per-lens physical calibration;
- it does not upgrade `FULL_PHYSICAL` color;
- exact spectral color cannot be inferred from three camera channels;
- appearance/display preview cannot feed back into the Scientific Master.

## Do not regress these historical lessons

Do not:

- make the large preview uncompressed again and expect Android discovery to behave the same;
- replace the primary RGB LinearRaw with the JPEG preview;
- rebayer the Scientific Master merely for Lightroom compatibility;
- let preview generation modify the LinearRaw payload;
- claim the preview is independently calibrated physical color;
- silently discard the historical v0.1 single-IFD failure state.

The desired restoration is the old proven **container pattern** applied to the newer bounded/finalized architecture.
