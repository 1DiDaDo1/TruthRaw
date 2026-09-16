# FotoGraaf 200MP staged preview v0.7

Status: research/integration candidate, not canonical promotion.

## Device observation that triggered v0.7

On the HONOR BKQ-N49, the v0.6 dedicated Camera-5 test could open the tele-preview path without producing a visible live image. This followed an earlier v0.5 observation where the combined preview + RAW session also produced no useful preview.

This observation is **not** promoted to a claim that physical camera 5 cannot preview. It only proves that the specific application preview topology used by v0.6 did not yield useful live frames on the tested device state.

## New separation of facts

v0.7 splits the test into three explicit stages:

1. **Capability stage**
   - no camera is opened on Activity startup;
   - the user explicitly asks to read Camera2 characteristics;
   - the app checks logical 0 contains physical 5 and that exact `16320x12288 RAW_SENSOR` is advertised through the high-resolution or maximum-resolution maps.

2. **Live-preview stage**
   - opens logical camera 0 only;
   - one ordinary `SurfaceTexture` preview output;
   - deliberately does **not** call `OutputConfiguration.setPhysicalCameraId(5)`;
   - requests zoom ratio approximately `3.7x` when supported;
   - reports `LOGICAL_MULTI_CAMERA_ACTIVE_PHYSICAL_ID` from live capture results;
   - visible preview and active physical selection remain observations only and create no sensor evidence.

3. **200MP RAW stage**
   - preview session is closed first;
   - a separate RAW-only session is created;
   - exact `16320x12288 RAW_SENSOR`, `maxImages=1`;
   - the RAW output is explicitly bound to physical camera 5;
   - the still request asks for `SENSOR_PIXEL_MODE_MAXIMUM_RESOLUTION`;
   - admission still requires exact dimensions, 200,540,160 samples, one RAW plane, 2-byte pixel stride, physical Camera-5 result, timestamp identity, and capture-result MAX pixel mode.

## Why logical preview is scientifically acceptable

The preview is not the evidence source for the 200MP claim. It exists only for framing and 3A feedback. Therefore it may use a more compatible logical-camera path without weakening the 200MP capture gate.

If the logical preview reports `activePhysicalId=5`, that is useful runtime confirmation that Android selected the tele physical camera for preview. If it reports another ID or no ID, the app must show that fact rather than relabel the preview as Camera 5.

The subsequent RAW capture remains independently bound to physical camera 5 and must pass its own fail-closed evidence gate.

## Scientific boundary

Maximum permitted successful capture interpretation remains:

`APP_VISIBLE_MAXIMUM_RESOLUTION_RAW_SENSOR_CFA_CANDIDATE`

until host Step-3B replay/promotion is run on the saved DNG/evidence package.

It does not prove untouched native photodiode/ADC output, full 200MP optical detail, or 200MP-specific color/noise calibration.

## Color provenance retained

The v0.7 evidence record retains capture-time:

- `CONTROL_AWB_STATE`;
- `COLOR_CORRECTION_GAINS`;
- `COLOR_CORRECTION_TRANSFORM`.

These are source-bound metadata observations only. They do not upgrade calibration authority.
