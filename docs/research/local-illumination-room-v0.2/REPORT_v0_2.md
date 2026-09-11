# TruthRaw Illumination Room v0.2 — report

Status: **RESEARCH_CANDIDATE_CI_PASS**

Branch: `research/illumination-room-v0.2-2026-09-11`

Base `main`: `514f2f4bde6aba5a6709e176c03b22c3b9aea912`

Initial validated candidate head: `59fa38ca4706e9c1b0c1b0c05e3499b4e7956c29`

Initial GitHub Actions run: `34546376103` — **SUCCESS**

Pre-ISO-hardening documented head: `7ef58e443bab79767a7de3754dfcf86e1cd9e819`

GitHub Actions run: `34546466380` — **SUCCESS**; Documentation Governance run `34546466379` — **SUCCESS**.

The later ISO-boundary hardening preserves the same research-candidate scientific scope and is required to pass the same CI before its head is treated as the latest validated candidate.

## What passed

1. CICM v1 upstream integrity.
2. Local Illumination Room / Room Capsule v0.1 upstream integrity.
3. Room ABI v0.1 upstream integrity.
4. Technical Backplane v0.1 upstream integrity.
5. GCC Release build + test.
6. Clang Release build + test.
7. Clang ASan + UBSan build + test.
8. Documentation Governance on the documented candidate state.

## Implemented v0.2 contracts

- Building Runtime `ResourcePolicy` is authoritative for room budget and tile size.
- The Illumination Room does not derive a second independent memory-class policy.
- Existing Room ABI v0.1 leases own the concrete packed-geometry, vector-boundary and tile-workspace allocations.
- No hidden full-frame image allocation is introduced.
- Technical Backplane source/master/zero-line/scene-scale identities are converted into the CICM `SceneBinding` only after backplane validation.
- Missing/corrupt lineage identity fails closed.
- Relative day/night/custom scenario labels are explicit counterfactual semantics.
- Relative EV is delegated to CICM `RELATIVE_RADIANCE_SCALE_ONLY`.
- Physical SNR remains unavailable on the relative path.
- The v0.1 local normal/visibility/confidence light solver is reused rather than forked.
- The same local sample/light input yields the same lighting equation result independent of low/high resource plan.
- `CalibratedIntrinsicRelightReserved` remains fail-closed.
- Counterfactual light states cannot increase evidence count, modify the scientific master, or modify the TruthRange zero line.

## ISO removal / scene boundary

ISO is absent by contract from the new-scene side of Illumination Room v0.2.

The following room/relative-world types are compile-time guarded against a `nominalIso` member:

- `RelativeScenario`;
- `AdaptivePlanRequest`;
- `AdaptivePlan`;
- CICM `CounterfactualWorldSpec`;
- CICM `RelativeCaptureSpec`;
- CICM `RelativeWorldPrediction`.

The runtime-exposed `IsoBoundary` additionally declares:

- scene ISO axis absent;
- relative scenario ISO parameter absent;
- relative EV independent of ISO;
- separately calibrated CICM sensor-forward mode may retain nominal ISO;
- that sensor-forward nominal ISO may never promote into scene state.

CICM `SensorModeCalibration` and `SensorPrediction` deliberately retain `nominalIso` because they belong to the separate calibrated hypothetical sensor/camera-mode forward model. This does **not** make ISO a property of the Scene Master, TruthRange, zero-line, local illumination equation, or relative day/night world.

Therefore the project boundary is:

`capture ISO provenance != scene coordinate != relative EV != TruthRange zero-line`

## Mobile adaptation exercised by tests

The test constructs a 16320x12288 local-domain planning case with two runtime policies:

- low tier: 32 MiB total working-set budget, 8 MiB heavy-room budget, tile 128;
- high tier: 256 MiB total working-set budget, 64 MiB heavy-room budget, tile 512, optional Vulkan preference.

The low tier must choose a geometry representation no finer than its budget permits; the high tier may retain finer geometry. Both execute the same counterfactual lighting equations.

This is a resource/fidelity adaptation, not a change in truth authority. Full rendered appearance may still differ when the retained geometry resolution differs; such differences must remain inside the counterfactual/appearance domain.

## Scientific boundary

This candidate does **not** establish calibrated physical sun, moon, sky spectrum, intrinsic reflectance, BRDF, global illumination or exact new shadows from one frame.

It also does not convert relative EV into a physical ISO/noise claim. ISO is absent from scene state. Physical electron/SNR capture prediction remains restricted to the separately calibrated CICM forward path, where nominal ISO is only a sensor-mode property.

The sealed source, scientific Scene Master and global TruthRange zero-line remain immutable.

## Promotion boundary

`RESEARCH_CANDIDATE_CI_PASS` means the implementation and tested contracts passed host CI. It does not make the module canonical, does not prove on-device Android memory/RSS/thermal behavior, and does not close FULL_PHYSICAL blockers.

The next architectural use is as an input contract for Room ABI v0.2 / Adaptive All-Room Binding.
