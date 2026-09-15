# TruthRaw Open-World Native Integration v0.3

Status: **RESEARCH — NOT MAIN-PROMOTED**

This step moves the open-world foundations from Python-only orchestration contracts into native execution/provenance corridors around CICM and Room Capsule, while adding stronger Structure Evidence derivation, route-bound calibration registry semantics, branching conservation/restoration provenance, and a stricter Camera-5 200MP CFA-topology gate.

## 0. Scope rule: sealed evidence, open world

The source RAW/CFA and capture metadata remain immutable evidence. The reconstructed world is **not** a sealed room and is not required to have a global spatial boundary.

A local Room Capsule is only a bounded **computational work domain**. A sample outside that capsule is outside the current local operation, not outside the represented world. The world may continue through interiors, exteriors, streets, landscapes, sky, distant geometry and future scene-graph regions.

Permanent rule:

> **Evidence is finite and immutable. World representation may be open-ended. Authority may not silently increase.**

## 1. Native illumination authority corridor

Implemented in:

- `native/open_world_native_v03.h`
- `native/open_world_native_v03.cpp`
- `tests/test_open_world_native_v03.cpp`

The native corridor carries the same four illumination authorities introduced by v0.1:

- `MEASURED`
- `CALIBRATED_ESTIMATE`
- `INFERRED`
- `COUNTERFACTUAL`

Each binding carries a record id, an unrestricted descriptive spatial scope and the provenance required by that authority. Measured illumination requires a hash-bound evidence record; calibrated estimates also require a calibration id; inferred illumination requires an inference method; counterfactual illumination requires an explicit parent state.

The corridor can execute:

- CICM relative-world prediction;
- CICM calibrated sensor-forward prediction;
- Room Capsule relative-light evaluation.

For all three, the **output remains counterfactual**. Even a measured lamp or independently calibrated sensor forward model does not turn a simulated new observation into another admitted exposure. The corridor asserts:

- `independentEvidenceAdded = false`;
- `mayModifyScientificMaster = false`;
- input illumination authority survives in provenance;
- output authority is `COUNTERFACTUAL`.

The adapter additionally checks the existing CICM/Room-Capsule evidence ledgers and rejects an upstream result if it were ever to claim that counterfactual light became evidence, modified the scientific master or modified the zero-line binding.

## 2. Structure Evidence derivation from pre-appearance support

Implemented in `tools/open_world_integration_v03.py` as `StructureMeasurementInputs` and `derive_structure_evidence()`.

v0.3 does not infer scientific detail support from rendered RGB contrast. Instead it accepts separately produced support quantities from the scientific pipeline:

- measured-CFA structural support;
- reconstructed-topology support;
- optical MTF support;
- uncertainty confidence;
- censoring risk;
- evidence hashes;
- optional orientation and spatial frequency.

The first derivation policy is a conservative lower envelope:

`admissible = min(MTF support, uncertainty confidence, 1 - censoring risk)`

`measured support = min(measured-CFA support, admissible)`

`reconstructed support = min(reconstructed-topology support, admissible)`

This is deliberately one-way: no semantic recognition, texture contrast or later sharpening may increase the scientific support values. The output can then pass through the existing v0.2 Detail/Acutance authority gate, where transformed pixels remain appearance rather than measured detail.

## 3. Native-tele Calibration Registry binding

`bind_registry_to_native_tele()` binds a Calibration Registry to the exact target route:

- device `HONOR BKQ-N49`;
- physical Camera `5`;
- `SENSOR_PIXEL_MODE_MAXIMUM_RESOLUTION` domain;
- `16320 x 12288`;
- Bayer `BGGR`.

Every admitted record must declare that same identity/geometry/CFA domain. By default every record must also have an independent hold-out `PASS`. The resulting registry fingerprint and route binding receive a deterministic SHA-256.

This does **not** manufacture calibration. It prepares the registry so future black/offset, linearity, gain/noise, PRNU, shading, MTF/PSF, colour, illuminant and spectral-response measurements cannot silently migrate between camera routes or sensor modes.

## 4. Branching conservation/restoration graph

`RestorationGraph` replaces the v0.2 linear-stack limitation for new research work.

Each restoration node is hash-bound to:

- immutable root source;
- explicit parent layer (or source root);
- mask;
- payload;
- restoration class;
- evidence hashes;
- method/confidence/hypothesis metadata.

Visual taint follows **ancestry**, not the whole project. Therefore this is valid:

`source -> evidence-supported scratch repair -> hypothetical historical colour`

while an independent sibling can still do:

`source -> evidence-supported tear repair`

What is forbidden is:

`hypothetical visual node -> later node relabelled evidence-supported`

This matches professional conservation logic: an interpretive visual treatment on one branch does not destroy the untouched source or another clean scientific branch.

## 5. Camera-5 200MP CFA topology binding v0.3

Implemented in `tools/camera5_200mp_evidence_bundle_v03.py`.

The previous v0.2 chain already requires:

`runtime capture -> canonical RAW raster -> DNG CFA sample identity`

v0.3 additionally requires:

- Camera2 reports a recognized Bayer CFA arrangement;
- the DNG reports a recognized Bayer CFA pattern;
- Camera2 CFA and DNG CFA agree exactly;
- canonical sample count is exactly `16320 * 12288 = 200,540,160`;
- CaptureResult camera id is `5`;
- CaptureResult reports `MAXIMUM_RESOLUTION`;
- `SENSOR_RAW_BINNING_FACTOR_USED` is not explicitly `true`.

The target classification is:

`CAMERA5_200MP_CAPTURE_CHAIN_CFA_TOPOLOGY_BOUND_APP_VISIBLE`

A PASS still does **not** prove one untouched ADC sample per physical photodiode, absence of sensor/HAL remosaic, physical pixel pitch, electron calibration, optical truth, spectral response or colour truth. It proves a much narrower but stronger fact: the app-visible Camera-5 maximum-resolution CFA raster, its DNG representation and their reported Bayer topology are internally consistent.

## 6. Validation

The open-world integrity workflow now executes:

1. all v0.1 foundation Python tests;
2. all v0.2 integration Python tests;
3. all v0.3 integration/topology Python tests;
4. the Camera-5 CFA-identity tests;
5. native CICM v1 regression compilation/execution;
6. native Room Capsule v0.1 regression compilation/execution;
7. native open-world authority-corridor v0.3 compilation/execution.

Workflow run `35020581837` on commit `910bf3a74ea9fd28168d0bd83fa185d345bbeaba` completed successfully for the implementation before this documentation commit.

## 7. Next scientific gates

The next stage should derive the Structure Evidence inputs from actual v4.7i reconstruction support, uncertainty/censoring fields and independently measured MTF rather than fixtures; populate the route-bound registry from real Camera-5 calibration campaigns; bind spectral/artificial-light calibration to illumination provenance; and run a real `16320 x 12288` Camera-5 manifest + RAW_SENSOR + DNG set through v0.2 followed by the v0.3 CFA-topology gate.

No change in this module promotes the research branch to canonical/main status.
