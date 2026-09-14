# TruthRaw FotoGraaf Calibration -> Scene Admission v0.1 — 2026-09-14

**Status: RESEARCH RUNTIME AUTHORITY GATE / FAIL-CLOSED / NO SCIENTIFIC-MASTER ROUTE CHANGE**

This document defines the next step after `FotoGraafCalibrationPack v0.1`: how a validated calibration claim may be attached to one real sealed scene capture without turning calibration data into extra photographic evidence.

Permanent laws remain:

**Measured where measured. Reconstructed where necessary. Never invented.**

**Representation can exceed the source. Knowledge claims cannot exceed the evidence.**

## 1. Problem being solved

A Calibration Pack can be internally valid yet still be wrong for the current photograph.

A tele calibration made for another sample domain, firmware, focus state, readout state, shutter range or temperature must not be applied merely because the EXIF ISO looks similar.

The runtime question is therefore:

`validated calibration pack + exact scene capture -> may this claim become CALIBRATED_PHYSICAL here?`

The answer is produced before any dependent reconstruction/metrology consumer is allowed to use the calibrated claim.

## 2. Admission route

The v0.1 route is:

`sealed Direct-CFA + CaptureMetrologyPacket`

`-> exact CalibrationScope match`

`-> exact captureSampleDomainId match`

`-> runtime valid-domain check`

`-> gain/readout + exposure + ISO provenance + aperture + optional temperature check`

`-> immutable CalibrationBindingPacket`

`-> dependent MeasurementLab / Architect / Restorer / Surveyor consumers`

If any required gate fails, the calibration is not attached.

The photograph may still continue through the ordinary source-bound/inferred route. A failed calibration match does not mean "return black" and does not justify inventing a replacement calibration.

## 3. Exact scope match

v0.1 requires exact equality for:

- device make/model;
- camera system ID and physical camera ID;
- lens role;
- capture API/source domain;
- capture mode;
- RAW dimensions;
- CFA pattern;
- sample representation;
- `captureSampleDomainId`;
- firmware/build identity;
- focus-state class;
- stabilization state.

Calibration protocol version is a property of the pack, not of the scene capture, so it is bound into pack provenance rather than compared as a scene property.

This deliberately favors false rejection over false physical authority.

## 4. ISO is provenance, not the calibration selector

The latest Honor experiments demonstrated why ISO alone is insufficient.

The same nominal neighborhood around ISO8192 produced two different serialized source domains: exact ISO8192 repeatedly selected the high-scale/censored domain while ISO8184 and ISO10244 remained ordinary.

Therefore scene admission requires `captureSampleDomainId` and a declared `gainReadoutStateId` in addition to checking that ISO metadata lies inside the pack's validated operating range.

ISO participates in proving that the current capture lies inside the calibrated measurement domain. It still does not become the identity of the Scientific Master.

## 5. Runtime valid domain

A pack may pass laboratory validation but still not be allowed to extrapolate arbitrarily.

For a scene-time `CALIBRATED_PHYSICAL` binding, the selected claim's `validDomain` must contain at least:

- `scopeBound=true`;
- finite inclusive exposure-time range;
- finite inclusive ISO-metadata range;
- finite inclusive f-number range;
- explicit list of admitted `gainReadoutStateId` values.

If the pack claims temperature-calibrated authority, the claim must also carry a validated temperature range and the scene must provide a compatible measured temperature.

No interpolation/extrapolation outside these declared bounds is silently permitted in v0.1.

## 6. Decision states

### `CALIBRATED_PHYSICAL_ADMITTED`

Exact scope matches, the pack itself passes CalibrationPack validation, the requested quantity has a validated `CALIBRATED_PHYSICAL` claim, and the current capture lies inside the claim's valid runtime domain.

A `CalibrationBindingPacket` is emitted.

### `NO_CALIBRATED_PHYSICAL_BINDING`

The image may continue, but that calibration claim is not used. Existing source-bound/inferred authority remains unchanged.

Typical causes:

- scope mismatch;
- wrong capture/sample domain;
- gain/readout state outside validated set;
- shutter/ISO/aperture outside validated range;
- required temperature unavailable/out of range;
- requested claim absent from the pack.

### `FAIL_CLOSED_REQUIRED_CALIBRATION_UNAVAILABLE`

Used when the caller explicitly requires calibrated physical authority for that operation. The requested physical claim must stop rather than silently downgrade while pretending to remain calibrated.

## 7. CalibrationBindingPacket

An admitted binding carries at least:

- exact source SHA-256;
- pack ID;
- calibration dataset-manifest SHA-256;
- calibration-scope SHA-256;
- quantity;
- model ID;
- uncertainty-model ID;
- valid-domain SHA-256;
- validation-report SHA-256;
- uncertainty-report SHA-256;
- acceptance-protocol ID;
- exact scene capture state used for admission;
- `physicalFrameCount=1`;
- `independentEvidenceCount=1`.

The packet does not contain additional scene pixels and is not a second evidence root.

Its digest intentionally excludes execution worker count.

## 8. Multiple photographers / workers

Calibration selection is scientific shared state, not per-thread policy.

The gate is resolved once before dependent parallel work. The resulting immutable binding is then shared by every admitted worker.

Therefore:

`1 photographer -> binding X`

`4 photographers -> binding X`

not:

`worker 1 chooses pack A, worker 2 chooses pack B`.

The v0.1 regression tests require the binding digest to remain identical when `executionWorkerCount` changes from 1 to 4.

This keeps the earlier law intact:

**More cores may give more photographers. They may never give more truth.**

## 9. HDR consequence

This gate also sharpens the HDR/ISO separation.

A calibrated `capture_dynamic_range` claim may feed scientific `SceneRangeEnvelope` reasoning after the capture measurement has been interpreted.

The product `HDR` appearance toggle is not a CalibrationPack quantity. It must not:

- invoke a second ISO/gain interpretation;
- choose a different calibration pack;
- create `CALIBRATED_PHYSICAL` authority;
- change the Scientific Master.

So the path is:

`capture ISO/gain/sample-domain interpretation`

`-> optional calibrated physical binding`

`-> Scientific Master / SceneRangeEnvelope`

`-> HDR appearance later`

not:

`HDR -> redo ISO calibration`.

## 10. Current implementation

Machine-readable contract:

`TRUTHRAW_FOTOGRAAF_CALIBRATION_SCENE_ADMISSION_CONTRACT_V0_1.json`

Verifier:

`tools/verify_fotograaf_calibration_scene_admission_v0_1.py`

Regression tests:

`tests/test_verify_fotograaf_calibration_scene_admission_v0_1.py`

The tests exercise:

- exact-scope admission;
- 1-worker versus 4-worker binding invariance;
- same ISO with different capture/sample domain rejection;
- gain/readout mismatch;
- exposure extrapolation rejection;
- required-calibration fail-closed behavior;
- temperature-calibrated pack requiring scene temperature;
- explicit proof that product `HDR` is not a calibration claim;
- preservation of 1/1 photographic evidence counts.

## 11. What this step still does not do

This gate does not yet apply calibration numerically to the Scientific Master.

Before that is allowed, a separate measurement-model integration step must prove:

1. exact numerical model application semantics;
2. uncertainty propagation;
3. source/master identity preservation;
4. worker-count invariance;
5. before/after physical validation on held-out captures;
6. no double application of source corrections such as upstream lens shading;
7. no backward authority upgrade.

Therefore the current Scientific Master route remains unchanged.

## 12. Next implementation gate

The next logical implementation after this admission layer is a **CalibrationModelBinding / MeasurementLab adapter** that can consume an admitted binding and expose calibrated black/noise/gain/dynamic-range parameters to the existing measurement likelihood without changing sealed source bytes.

That adapter must first operate in shadow/diagnostic mode: compute both the current source-bound result and the calibration-assisted result side by side, compare them on held-out calibration/scene tests, and only later be considered for scientific-route promotion.

## 13. Permanent sentence

**FotoGraaf may use a calibration only after the current sealed capture proves that it belongs to the same camera/lens/mode/sample/readout/operating domain; ISO alone never selects physical authority, worker count never changes the binding, and HDR display remains downstream of the one admitted measurement interpretation.**
