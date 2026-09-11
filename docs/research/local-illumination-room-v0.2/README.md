# TruthRaw Illumination Room v0.2

Status: **RESEARCH_CANDIDATE_CI_PASS — ADAPTIVE_RUNTIME_BOUND_RELATIVE_DAY_NIGHT_ISO_FREE_SCENE**

Illumination Room v0.2 continues the photographer-selected Local Illumination Room without changing its scientific claim boundary. It is an orchestration/binding layer over the proven v0.1 Room Capsule light solver.

Validated ISO-hardened implementation: `d4afd4ceb6ff69c09bc27551a07ab41c3d6c77b5`.

Validation: Illumination Room run `34547369624` — **SUCCESS**; Documentation Governance run `34547369523` — **SUCCESS**.

## Purpose

The room studies counterfactual light/dark states only inside the photographer-selected local domain while keeping the sealed evidence, admitted scientific Scene Master, TruthRange zero-line, and evidence count immutable.

The key v0.2 change is resource ownership: the room no longer derives its own device budget. `Building Runtime::ResourcePolicy` supplies the admissible room budget and tile size, and `Room ABI v0.1` supplies all concrete execution leases.

This lets a cheaper phone use coarser local geometry, smaller tiles and one bounded room working set while a stronger phone may use finer local geometry and larger tiles. The local lighting equations are independent of that resource tier.

## Architecture

`sealed evidence -> scientific Scene Master -> Technical Backplane identities -> photographer room selection -> Building Runtime resource policy -> Illumination Room adaptive plan -> Room ABI leases -> Room Capsule v0.1 local light solve -> CICM relative world/EV study -> appearance output`

No full-frame allocation is introduced by this module.

## ISO boundary — absent from the new scene

ISO is **not** a coordinate, scale, identity, sensitivity axis, or processing parameter of the TruthRaw Scene Master, TruthRange, zero-line, Illumination Room, or relative day/night world.

Capture ISO belongs to the sealed capture history/provenance together with the physical sensor mode and original exposure conditions. It may help interpret how the original evidence was acquired, but it does not label or rescale the reconstructed scene.

Consequently:

- `RelativeScenario` has no ISO member;
- `AdaptivePlanRequest` and `AdaptivePlan` have no ISO member;
- CICM `CounterfactualWorldSpec`, `RelativeCaptureSpec`, and `RelativeWorldPrediction` have no nominal-ISO member;
- relative EV is computed only from illumination and relative shutter scale;
- room planning, local light incidence, zero-line binding and resource adaptation are ISO-independent.

The only retained nominal ISO in this dependency graph is inside CICM's **separate calibrated sensor-forward model** (`SensorModeCalibration` / `SensorPrediction`). There it describes a hypothetical calibrated camera/sensor mode. That value never promotes into scene state, never changes `L0`, and is not an input to the Illumination Room relative-light path.

Therefore:

`capture ISO provenance != scene coordinate != relative EV != TruthRange zero-line`

This boundary is compile-time tested so a future change cannot silently add `nominalIso` to the relative scene/room API without breaking CI.

### All-room handoff invariant

Room ABI v0.2 / Adaptive All-Room Binding must preserve this same boundary for every room. A room may read immutable capture-ISO provenance only when its scientific contract explicitly needs source-acquisition context; it may not copy ISO into Scene Master identity, TruthRange coordinates, room-to-room authority, resource policy, appearance EV, counterfactual world scale, or zero-line state. Resource tier must never alter this rule.

## Day / night semantics

v0.2 adds explicit scenario labels:

- `RELATIVE_DAYLIGHT_STUDY`
- `RELATIVE_NIGHT_STUDY`
- `CUSTOM_RELATIVE`

These labels are counterfactual study semantics, not calibrated sun, moon, sky-spectrum, BRDF or global-illumination claims.

Each scenario has a positive neutral illumination scale. That scalar is passed through CICM v1 `RELATIVE_RADIANCE_SCALE_ONLY`, so the photographer can obtain a rigorous relative EV comparison:

`deltaEV = log2(illuminationScale * shutterScale)`

Examples used by tests:

- `4x` neutral illumination, unchanged shutter -> `+2 EV`
- `1/16` neutral illumination, unchanged shutter -> `-4 EV`

CICM deliberately reports no physical SNR for this path. A nominal ISO may appear only in the separately calibrated CICM sensor-forward path as a sensor-mode property; it is not part of this scene/world calculation.

## Local colour and light incidence

Local appearance modulation still uses the v0.1 Room Capsule solver:

- ambient boundary illumination;
- directional light;
- point light with softened inverse-square falloff;
- surface normal incidence;
- visibility;
- geometry confidence.

Low-confidence geometry fails toward the compact boundary illumination envelope rather than inventing hard relight structure.

The local RGB multiplier is appearance/counterfactual state. It is not intrinsic reflectance and is not promoted to the scientific master.

## Runtime-adaptive invariant

Resource tier may change:

- tile size;
- local geometry downsample;
- workspace/lease size;
- optional GPU preference.

Resource tier may not change:

- source evidence identity;
- scientific-master identity;
- zero-line identity;
- scene-scale identity;
- physical-frame count;
- independent-evidence count;
- the local illumination equations;
- the physical-relight claim boundary;
- the absence of an ISO axis from scene state.

The test suite evaluates the same local sample/light state under low- and high-tier plans and requires identical relative-light math.

## Technical Backplane binding

The room validates Technical Backplane v0.1 and converts its four immutable SHA-256 identities into the CICM `SceneBinding`:

- source evidence;
- scientific master;
- zero line;
- scene scale.

A missing/corrupt zero-line or other invalid backplane state fails closed before CICM simulation.

## Physical relighting remains blocked

`CalibratedIntrinsicRelightReserved` remains fail-closed. A physical geometry/BRDF/spectral sun/night relight requires evidence/state that v0.2 does not claim to possess.

Counterfactual observations never become independent source evidence and never modify the sealed source, scientific master or zero line.

## Dependencies

- Local Illumination Room / Room Capsule v0.1
- Counterfactual Illumination & Capture Manifold (CICM) v1
- Building Runtime v0.1 resource policy
- Room ABI v0.1 lease authority
- Technical Backplane v0.1 lineage identities
- Manifold Conditioning v1 only because it is part of the linked Room ABI v0.1 implementation
- canonical reconstruction v4.7i header only through the existing Room ABI v0.1 interface

## Intended next step

The validated adaptive planning/lease and ISO-free scene contracts should become inputs to Room ABI v0.2 / Adaptive All-Room Binding rather than remaining special-case Illumination Room behavior.
