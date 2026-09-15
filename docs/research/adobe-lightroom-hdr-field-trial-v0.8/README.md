# TruthRaw ↔ Adobe Lightroom real HDR field trial v0.8

**Status:** RESEARCH — prepared, not yet executed inside Adobe Lightroom  
**Branch:** `research/open-world-foundations-v01`  
**Source:** one real HONOR BKQ-N49 tele DNG, no HDR merge, no synthetic exposure stack

## Purpose

v0.7 established the interoperability rule: Adobe Lightroom can edit a single-exposure RAW in HDR mode, so TruthRaw does not need to invent bracketed exposures or fake `Merge to HDR` metadata.

v0.8 binds that rule to one real, already-audited project source:

`IMG_BNC_TRUTHRAW20260907_094449_565.dng`

SHA-256:
`7930ba5db0fe1b7b9dcc09e9337efbca76b8877fb78a6dc4667761bf25159b67`

The source is HONOR BKQ-N49, physical camera 5, 22.48 mm f/2.6, ISO 638, 4080×3072 BGGR, WhiteLevel 1023, one physical RAW_SENSOR frame. Existing TruthRaw evidence records the frozen tele uncertainty decision as `BACKEND_BOUND_UNCERTAINTY_PROSPECTIVE_PASS`.

## Existing real dynamic evidence

Existing project analysis for this exact source reports:

- Stage-2 minimum: `-0.006340971682220697`
- Stage-2 maximum: `1.2977731227874756`
- Stage-2 samples above 1: `62,643 / 12,533,760` = `0.499794%`
- source samples at WhiteLevel: `217` = `0.001731%`
- pre-display q99: `0.9892458261670347`
- historical SDR preview scalar: `0.72782718001423`

Two diagnostic headroom numbers follow from those values:

```text
log2(Stage2_max / 1.0) = 0.376038 EV
log2(Stage2_max / q99) = 0.391637 EV
```

These are **not Adobe HDR headroom measurements**. They only prove that this real source/pipeline contains scene-linear overrange above a chosen Stage-2 reference. Adobe's HDR histogram/headroom is a presentation result after Adobe's RAW development, exposure mapping, profile, tone processing and display mapping.

The historical SDR preview scalar maps the Stage-2 maximum to about `0.944555`, below unity. That is direct evidence that an SDR-oriented preview can compress away visible overrange even while the Scientific Master/source-domain representation still contains it.

## Censored highlights

The 217 source samples at WhiteLevel remain censored lower-bound evidence:

```text
actual scene signal >= censor bound
```

They are not exact recovered radiance and are not allowed to be filled with invented HDR brightness. Lightroom highlight rendering may create a visually smooth result, but that output remains presentation processing and cannot upgrade scientific authority.

## Exact Adobe field-trial procedure

1. Verify the DNG SHA-256 before import.
2. Import the original DNG unchanged into Lightroom desktop or Lightroom Classic.
3. Do **not** use `Merge to HDR`, exposure bracketing, AI relighting, generative editing or synthetic exposure copies.
4. Open Edit/Develop and enable **HDR** on this single-exposure RAW.
5. Record the Lightroom version, process version, profile, exposure value and whether any tone controls are non-default.
6. Enable `Visualize HDR` and record the HDR Limit setting and the visible histogram extent above SDR white.
7. Record any display-headroom indicator Adobe exposes. This is display/presentation capability, not source evidence.
8. Save or export Adobe settings/metadata so the exact HDR state can be inspected later. Do not overwrite the evidence root.
9. Export at least one true HDR rendered file if available: preferably 16-bit TIFF plus AVIF or JPEG XL; an optional gain-map JPEG is useful for compatibility testing.
10. Hash every returned artifact and compare the rendered Adobe headroom against TruthRaw's source/Scene-Master diagnostics without treating Adobe's display mapping as new evidence.

## What a successful result proves

A successful test can prove that:

- Adobe accepts the real single-exposure HONOR DNG in HDR edit mode;
- Adobe exposes HDR tones above SDR white from one physical capture;
- the real dynamic scene can be projected into Adobe HDR without a fake exposure stack;
- an HDR TIFF/AVIF/JXL/gain-map export can preserve presentation headroom through an Adobe round-trip.

It does **not** prove that every bright Adobe pixel was directly measured by the sensor, that clipping was recovered exactly, or that Adobe's HDR Limit equals TruthRaw's evidence-supported dynamic range.

## Adobe documentation basis checked 2026-09-15

Official Adobe documentation states that HDR editing can be used with a **single exposure raw file** and that HDR mode exposes tones above normal SDR white. Adobe also documents HDR export for non-RAW formats such as AVIF, JPEG XL, TIFF, PSD/PSB and PNG; JPEG can use a Gain Map. DNG/raw conversion preserves raw information and can still be edited in HDR mode.

References:

- https://helpx.adobe.com/lightroom/desktop/edit-photos/hdr-output.html
- https://helpx.adobe.com/lightroom-classic/desktop/process-and-develop-photos/hdr-output.html
- https://helpx.adobe.com/lightroom-classic/desktop/export-photos/exporting-photos-basic-workflow.html
- https://helpx.adobe.com/camera-raw/using/hdr-output.html

## Implementation

Code: `tools/adobe_hdr_field_trial_v08.py`  
Tests: `tests/test_adobe_hdr_field_trial_v08.py`  
Bound real-source manifest: `REAL_TELE_CANDIDATE.json`

The runtime rejects multi-frame evidence, exposure merge, fake HDR-merge metadata and AI scene editing in the scientific field trial. Returned Adobe headroom is classified as presentation evidence only and cannot write authority back into the Scientific Master.

## Next gate

The only missing part of this field trial is the Adobe application execution itself. Once Lightroom produces the HDR state/XMP or exported metadata plus one HDR render, TruthRaw can ingest those observations and quantify the exact relationship between Adobe's HDR zones and the source-bound open-world dynamic representation.
