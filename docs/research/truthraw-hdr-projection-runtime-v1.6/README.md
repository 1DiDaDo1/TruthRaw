# TruthRaw HDR Projection Runtime v1.6

**Status:** RESEARCH — runtime contract implemented; real Scientific Master / Dynamic Authority binding still pending.

## Purpose

v1.6 turns the v1.5 HDR projection authority contract into a bounded streaming RGB projection runtime. It is the first TruthRaw-owned path that can take hash-bound Scientific-Master RGB samples, preserve their authority/uncertainty and deterministically map them to PQ RGB code values without writing presentation state back into science.

This module is **not yet an AVIF/JPEG XL/TIFF encoder** and it does not create a Scientific Master. A real image render is admissible only after exact real Scientific Master and Dynamic Authority Field hashes are supplied.

## Authority boundary

The immutable source for the current Adobe/HDR experiment remains:

- `IMG_BNC_TRUTHRAW20260907_094449_565.dng`
- source SHA-256 `7930ba5db0fe1b7b9dcc09e9337efbca76b8877fb78a6dc4667761bf25159b67`
- decoded CFA SHA-256 `883cbe13fd2a5afe7e9f3f4147df3a8ed3be681340a3c0422933f42d0f4d719c`
- 217 source-white samples remain censored lower-bound evidence.

The v1.5 state currently has no exact real Scientific Master SHA or Dynamic Authority SHA bound for this source. v1.6 therefore implements and tests the runtime while explicitly keeping the real-render gate closed.

## Runtime model

`tools/truthraw_hdr_projection_runtime_v16.py` adds:

- `ColorimetricProjectionBindingV16` — a 3x3 Scientific-Master-RGB to target-linear-RGB matrix plus immutable transform SHA and unchanged color-authority label;
- `ScientificRgbPixelV16` — three scene-linear values, three authority classes and per-channel p95 uncertainty;
- `ProjectionRuntimeBindingV16` — joins the v1.5 source/master/DAF binding to the color transform;
- `project_rgb_pixel_v16()` — applies target-primary luminance, scene-EV to display-EV mapping, luminance-preserving scaling and PQ encoding;
- `StreamingHdrProjectionAccumulatorV16` — produces chunk-boundary-independent projected-record hashes in global raster order.

Supported target primaries remain P3-D65, Rec.709 and Rec.2020. Transfer is PQ/ST-2084. v1.6 emits RGB PQ codes as a scientific/presentation boundary artifact, not a container file.

## Scientific rules

Only `MEASURED`, `CALIBRATED_ESTIMATE` and `RECONSTRUCTED` channels may produce an exact projected RGB sample. Their original authority classes and p95 uncertainties are carried unchanged.

If any RGB channel is `CENSORED` or `UNKNOWN`, exact pixel projection is withheld. No display code is allowed to turn a lower bound or missing value into exact radiance.

`COUNTERFACTUAL` and `APPEARANCE_ONLY` are rejected from this scientific projection path. They require a separate presentation layer.

Negative/out-of-gamut target RGB components are clipped only in the presentation layer and flagged as `negative_gamut_clip`. Values exceeding the requested display peak are presentation-clipped and flagged as `channel_peak_clip`. Neither event changes the Scientific Master.

`Adobe HDR Limit`, PQ code values and MaxCLL are presentation quantities. They never define sensor dynamic range or scientific headroom.

## Relation to the Lightroom experiments

The v1.3/v1.4 experiments proved that the same exact CFA can be rendered into very different HDR distributions. The counterbalanced Lightroom edit (`Exposure -5`, `Highlights -100`, `Whites -100`, steep tone curve) moved only the extreme highlight tail upward while leaving the image bulk slightly darker. That is used here only as empirical evidence that presentation redistribution must be modeled separately from scene authority.

v1.6 does **not** copy Adobe's tone curve as a scientific law and does not claim Adobe's 10,000-nit PQ ceiling is scene luminance.

## Validation

`tests/test_truthraw_hdr_projection_runtime_v16.py` verifies:

- supported RGB projects while authority and uncertainty remain unchanged;
- censored/unknown channels fail closed for exact projection;
- counterfactual/appearance input is rejected;
- negative-gamut and display-peak clipping are explicit presentation events;
- target primaries must match the bound color transform;
- Adobe HDR-limit metadata does not change projected pixels;
- projected content hash is independent of streaming chunk boundaries;
- empty output cannot be finalized.

## Next real gate

1. Produce or locate the deterministic Scientific Master artifact for the exact immutable tele source and bind its SHA-256.
2. Persist the matching real Dynamic Authority Field artifact and bind its SHA-256.
3. Bind the exact source-specific Scientific-Master-RGB -> target-P3 transform and its authority scope.
4. Stream the real 4080x3072 Scientific Master through v1.6.
5. Encode the projected RGB into a true P3-D65/PQ HDR exchange file.
6. Re-import that TruthRaw-owned output into Lightroom and verify that Adobe can alter only presentation state, never scientific authority.

Until those gates are complete, v1.6 is a validated runtime contract rather than a completed real HDR image render.
