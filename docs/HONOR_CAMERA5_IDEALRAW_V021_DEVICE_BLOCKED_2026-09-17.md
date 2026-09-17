# HONOR Camera-5 IdealRAW v0.21 device result — blocked fail-closed

Date: 2026-09-17  
Device: HONOR Magic 8 Pro / BKQ-N49  
Experiment: v0.21 `EnableIdealRAW` single-variable session-parameter probe

## Result

The device reached live logical-camera-0 preview with physical Camera 5 confirmed, then stopped before the v0.21 vendor intervention could be attached or submitted.

Observed UI state:

- live frame: `120`
- logical camera: `0`
- zoom: `3.7x`
- active physical camera: `5`
- tele route: confirmed
- ISO: `153`
- exposure: `10.000 ms`
- AF state: `2`
- AE state: `1`
- Stage 3 classification: `BLOCKED_IDEALRAW_RUNTIME_TYPE_UNAVAILABLE`

The app displayed:

`Geen vendor-key gok; v0.20 control blijft onaangeroerd.`

## Scientific interpretation

This is a useful negative/blocked experiment, not a failed provenance event.

v0.21 proved that its safety gate behaves as designed: the `EnableIdealRAW` key is present on the previously recorded Camera2 request/session surface, but the runtime-value-type reflection path used by v0.21 could not establish a safe concrete Java value class on this device. Therefore v0.21 did not write the vendor key, did not attach a modified session parameter and did not submit a capture under the intervention.

The result does **not** refute the hypothesis that this QTI/HONOR control could affect RAW routing. It only says that the first reflection-based type-discovery method is insufficient on this device.

The v0.20 capture remains the experimental control and current physical evidence authority.

## Next bounded experiment: v0.22

v0.22 separates type discovery from route intervention.

It uses Android's public `CaptureRequest.Key(String, Class<T>)` testing/custom-field constructor to test candidate representations only inside disposable app-side `CaptureRequest.Builder` instances. For each candidate it records local set/get/build/request-readback success.

Crucially, v0.22 deliberately performs:

- no `SessionConfiguration.setSessionParameters` call for the candidate;
- no vendor-modified session submission;
- no capture request submission;
- no RAW pixel access or mutation.

A unique passing representation, if one exists, is classified only as an **app-side Camera2 marshalling candidate**. It is not vendor-semantic proof. A later separate build may use that representation for the actual single-variable intervention.

Permanent boundary remains:

`APP_VISIBLE_CAMERA2_RAW_SENSOR_NOT_UNTOUCHED_PHOTODIODE_ADC_PROOF`
