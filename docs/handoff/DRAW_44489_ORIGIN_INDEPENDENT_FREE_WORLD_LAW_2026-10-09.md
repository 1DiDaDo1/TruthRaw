# D.RAW 44489 — Origin-Independent Free World Evidence Law — 2026-10-09

## Purpose

This handoff records an explicit clarification of the existing D.RAW architecture. It does **not** create a new renderer, source format, scientific promotion path, calibration profile, reconstruction result, Appearance result or physical measurement.

Permanent project statement:

> D.RAW starts from a sealed readable observation. The observation may have arrived from a file, a camera/lens capture path, or another admitted source path. D.RAW does not require camera, lens, vendor or acquisition-origin identity in order to begin read-only structural inspection. What D.RAW may scientifically claim is determined by readable evidence, explicit relations, authority and uncertainty — not by a product name.

This strengthens the existing motto:

**One Free World. Many sealed observations. One evidence law.**

## Runtime contract change

Code commit:

`1e593ade87ab2f1246e1e2ea390e43aaad9fcd49`

Changed file only:

`suite_android/app/src/main/java/com/truthraw/adaptiveui/UniversalIdentityIndependenceV01.kt`

Additive contract rules now explicitly state:

- source origin may be UNKNOWN;
- unknown camera/lens/vendor identity does not block source sealing;
- unknown origin does not block read-only structural inspection;
- readable observation content drives admissible scientific interpretation;
- a claim that specifically needs origin-bound evidence remains UNKNOWN when that evidence is absent;
- missing origin identity may never be synthesized;
- Free World representation may be richer/denser than source sampling;
- denser representation does not create a new measurement;
- RECONSTRUCTED or UNKNOWN values may never be relabelled MEASURED;
- source lineage remains recoverable;
- sealed source values remain immutable;
- derived world state cannot scientifically write back;
- derived values require explicit authority and uncertainty;
- SOURCE/SENSOR, WORLD/SCENE and VIEW/APPEARANCE remain distinct.

No previous contract fields were removed.

## Relationship to existing D.RAW architecture

This is a clarification and hardening of existing structures, not a parallel architecture:

- `UniversalSourceProfiler` remains the universal read-only intake profiler and keeps UNKNOWN valid;
- `RasterIndependentSampleLatticeV01` keeps sealed source samples as exact measured anchors and all unanchored positions UNKNOWN until an explicitly versioned reconstruction provides a derived value with provenance/uncertainty;
- `FreeWorldSourceLatticeBridgeContractV01` still requires an independently validated world-to-source relation before a Free World query can reach a measured source anchor;
- `LightTransportAuthorityContractV01` keeps observed radiometry, geometry/normal, material, illumination, spectral hypothesis and visibility as separate authority axes;
- `TemporalFootprintV01` keeps exposure integration, capture order, rolling-shutter timing and motion as separate facts/unknowns and never treats a synthetic intermediate as a physical observation;
- `ConservationRestorationAuthorityRuntimeV01` keeps restoration reversible/replayable, preserves source/Scientific-Master identity and never turns loss compensation into MEASURED evidence.

## Interdisciplinary decision basis synchronized on 2026-10-09

Current primary/standard/university knowledge was rechecked across the domains the project depends on. The important project consequences are:

### Photography / sensor / RAW

Sensor sampling, black/white boundaries, noise and clipping are measurement properties. Container/vendor identity may help parsing or provenance but cannot substitute for measured/calibrated evidence. DNG, EMVA 1288 and camera-resolution standards are reference families, not authority generators.

### Optics / resolution

Scene radiance reaches the sealed samples through the optical PSF/MTF, sensor aperture/CFA and sampling chain. A denser Free World raster may improve representation and support reconstruction, but cannot manufacture independent optical measurements or measured high-frequency detail.

### Human vision / black-white / colour appearance

Perceived lightness, brightness, contrast and colour depend on adaptation, surround and viewing conditions. Therefore display black, highlight shoulder, chromatic adaptation and local brightness impression belong downstream in VIEW/APPEARANCE unless separately supported as scene evidence. Scientific zero/BlackLevel is not display black.

### Artificial illumination / light falloff / 3D rendering

Image-space brightness gradients may be consistent with illumination and geometry, but a single rendered observation does not by itself prove lamp power, surface reflectance, normal, spectral power distribution, distance or bounce-light paths. Physically based rendering equations may constrain hypotheses; plausible rendering is not measurement.

### Depth / architecture / geometry

Perspective, shading, texture gradients, occlusion, motion parallax and binocular disparity are depth cues with different authority. Single-view geometry can support bounded hypotheses but must not silently become metric 3D. Architectural lines and vanishing structure are evidence only to the degree their model assumptions are validated.

### Stop-motion / time

Exposure integrates over a finite interval. Frame order, readout timing, object motion and occlusion timing are separate quantities. Temporal interpolation or synthetic in-between states are derived representations, not extra captured observations.

### Restoration / conservation

Preserve the original, document intervention, distinguish loss from reconstruction, prefer replayable/removable transforms and never make aesthetic reintegration indistinguishable from original measured support. This maps directly to D.RAW's immutable sealed source plus reversible derived state.

Reference families rechecked for this synchronization include ISO 12233:2024, EMVA 1288 Release 4.0, Adobe DNG Specification 1.7.1.0, CIE 248:2022/CIECAM16, current Stanford imaging/graphics/radiometry material, current MIT vision/depth material, CMU projective/multiview geometry material and ICOMOS conservation/recording principles.

These sources may inform models, constraints and tests. They never create MEASURED authority.

## Current project state preserved

PR #131 remains the active **draft / unmerged** Workspace-Free-Raster continuation line.

The physically accepted Warm Illuminant Retention v0.1 result remains unchanged.

Natural Light Local Field Tone v0.1 remains **CANDIDATE / NOT YET REAL-DEVICE ACCEPTED**. Its physical lamp-scene test remains required before reclassification.

The Q100 / true-4:4:4 JPEG acceptance/regression boundary remains unchanged.

Scientific promotion remains closed unless explicit independent evidence and the existing promotion gates justify a future decision.

## Exact-head validation completed after handoff creation

The first exact-head Free Raster build exposed a **tooling idempotence defect**, not a runtime-science or image-output defect. `tools/apply_presentation_illuminant_warmth_retention_v0_1.py` still expected the old text layout in which the warmth stage appeared immediately after the ADVANCED/PRO `else` branch. Natural Light Local Field Tone v0.1 had legitimately been inserted before warmth, while the actual warmth runtime call was already present.

The generator was therefore hardened so an already-present `presentation_illuminant_warmth::apply(` call is treated as already applied, after which the existing ordering and one-call contract checks still run. The warmth regression was extended to lock the actual runtime order:

`Natural Light Local Field Tone -> Warm Illuminant Retention -> Near-White Highlight Chroma Guard -> Gamut Fit`

No image algorithm, `photo_export_bridge.cpp`, Scientific Master, sealed source, reconstruction authority or Local Field Tone parameter was changed by this tooling repair.

Exact validated code head:

`5d3fa07c4b55871cc82cc17d6bc0a4eb66b88177`

Exact diff relative to the preceding Local Field Tone runtime head `0a489190c474fa765431064513c0f6e52f7b63fd` changes only:

- this origin-independent evidence-law handoff;
- `UniversalIdentityIndependenceV01.kt`;
- the warmth patch generator;
- the warmth regression test.

The runtime image-processing bridge itself was not changed in this continuation.

### Free Raster / downstream output gate

Workflow: `D.RAW Free Raster v0.3 Finish APK`  
Run: `37918298982`  
Result: **SUCCESS**

Proven green on exact validated code head:

- deterministic Free Raster v0.3 integration contract;
- presentation gamut-fit regression;
- ADVANCED/PRO near-white highlight chroma-rolloff regression;
- Warm Illuminant Retention v0.1 regression;
- Natural Light Local Field Tone v0.1 regression;
- PURE Float32 headroom regression;
- presentation headroom wiring v0.2;
- sealed Full-Frame Streaming integrity;
- strict High-Fidelity JPEG codec regression with `pix_fmt=yuvj444p`;
- Kotlin/unit tests;
- NDK/C++ build;
- APK verification and artifact upload.

The workflow's runtime-wiring step reported:

`Runtime wiring and presentation output guards already applied.`

and retained exact HEAD `5d3fa07c4b55871cc82cc17d6bc0a4eb66b88177`, so it did not generate or push a hidden runtime rewrite.

Candidate APK from this exact run:

- bytes: `8,861,667`;
- SHA-256: `a0d15951af12caf3b756b46fe0a5090879618f50f4bfb499940d49e6b1d8528b`;
- artifact ID: `11611032828`;
- artifact name: `draw-free-raster-v03-fullres-candidate-apk`;
- artifact ZIP SHA-256: `382a3128d624161dd781d5fc0708b137512511f779fb0bfd4162c8a902ee02a3`.

### Universal Intake gate

Workflow run: `37918299238`  
Result: **SUCCESS**

The D.RAW vision contract, pinned Android/NDK environment, stable development signing identity, full arm64 APK assembly, APK verification and artifact upload all completed successfully on the same exact code head.

Therefore the origin-independent evidence-law hardening is now mechanically compatible with the existing Universal Intake and downstream Free Raster/output cables.

### PR state at validation

PR #131 remains:

- open;
- draft;
- unmerged;
- mergeable=true.

No scientific or product promotion follows automatically from these CI results.

## Next safe execution order

1. Keep `5d3fa07c4b55871cc82cc17d6bc0a4eb66b88177` as the exact validated code checkpoint for this origin-independent continuation.
2. Keep Local Field Tone runtime unchanged until its pending real-device ADVANCED + Natural Light and PRO + Natural Light lamp-scene test is supplied.
3. Compare that physical result to the accepted Warm Illuminant ADV2/PRO reference and inspect local luminous-field impression, black protection, halo/seam behaviour, RGB-ratio stability, near-white magenta regression and PURE isolation.
4. Only after physical evidence may Local Field Tone move from CANDIDATE to PASS or be revised.
5. Continue scientific reconstruction/geometry/light-transport work only through existing explicit authority, uncertainty, hold-out and promotion gates.
