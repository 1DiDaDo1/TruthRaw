# TruthRaw FotoGraaf World Studio v0.1

Status: **RESEARCH ARCHITECTURE — extends CICM/Room Capsule without changing sealed evidence or Scientific Master authority**

## 1. Why this exists

TruthRaw's original FotoGraaf idea should not be limited to a brightness slider or one local relight room. Once the Scientific Master exists, FotoGraaf may explore a very large counterfactual photography space: different lights, directions, softness, local visibility, materials, atmosphere, virtual exposure and camera choices.

The word **unlimited** means the hypothesis/descriptor space does not need to inherit RAW10, SDR, one exposure, one lamp or one fixed rendering. It does **not** mean infinite measured information or infinite resident RAM.

The governing rule remains:

**Representation can exceed the source. Knowledge claims cannot exceed the evidence.**

## 2. Two FotoGraaf roles that must stay separate

### A. Capture Metrologist

Lives before or at evidence admission. It establishes what camera/lens/mode/sample domain actually made the capture. C0, Camera2 route proof and later physical calibration belong here.

### B. World Photographer

Lives downstream of the admitted Scientific Master. It explores how the reconstructed scene could be photographed or illuminated under explicitly counterfactual conditions.

The World Photographer cannot rewrite C0, Direct-CFA evidence, Scientific Master values, zero-line identity or uncertainty as if a counterfactual world had been captured.

## 3. World Studio data model

A World Studio state is a small graph of **descriptors and hypotheses**, not another full-resolution image.

### SceneDomain

A selected spatial domain/ROI with a vector boundary and optional hierarchy. This avoids confusing a selected scene region with a Building Runtime execution `Room`.

### LightEmitter

May describe, where supported:

- directional light / distant sun-like source;
- point source;
- finite area source;
- environment illumination;
- emissive surface;
- position/direction;
- angular extent / apparent source size and therefore softness;
- relative intensity in EV or a calibrated radiometric quantity when available;
- spectrum/SPD binding or `UNKNOWN_SPECTRUM`;
- polarization descriptor when independently supported;
- temporal envelope for changing light.

A source with only relative intensity cannot be silently promoted to a physical-radiometry claim.

### GeometryHypothesis

May carry compact depth, surface normal, visibility, occlusion and confidence. Unknown or occluded geometry stays unknown. FotoGraaf may create multiple hypotheses rather than collapse ambiguity into one invented shape.

### MaterialHypothesis

May carry only properties justified by the current authority level, such as:

- diffuse/specular balance;
- roughness;
- index-of-refraction / transmission hypothesis;
- absorption/transmittance hypothesis;
- anisotropy hypothesis;
- subsurface-scattering hypothesis;
- confidence/support provenance.

Water, glass, polished metal, skin, feathers and translucent material are explicit stress cases because simple Lambertian relighting is not sufficient for them.

### AtmosphereHypothesis

Reserved/optional descriptors for extinction, haze, fog, airlight and scattering. A single photograph normally underdetermines these quantities; default status is therefore weak/reconstructed/counterfactual rather than measured.

### VirtualCapture

A counterfactual camera view may vary exposure/shutter, focus, aperture/PSF, crop/virtual focal framing and sensor encoding. Physical sensor-noise/SNR claims require an exact-bound calibrated forward model. Otherwise the result is a representation/appearance experiment only.

## 4. Authority ladder

World Studio deliberately has multiple levels rather than one misleading `physical=true` flag.

### WS0 — Relative Appearance

Current CICM-style relative radiance/EV changes. No claim of exact physical illumination, BRDF or SNR.

### WS1 — Geometry-Aware Relative Relighting

Uses reconstructed geometry/normals/visibility to make directional and local lighting spatially plausible. Still counterfactual and not independently calibrated physical relighting.

### WS2 — Calibrated Neutral Photometric Forward

Permitted only when the exact camera/sensor mode and required signal/noise/radiometric calibration are bound. Can predict quantities such as expected electron-domain behavior within that calibration envelope. Spectrum/material truth remains limited.

### WS3 — Spectral / Material / Participating-Media Physical Model

Reserved until independent geometry/material/spectral/optical evidence supports it. This is where physically stronger water/glass/material and atmosphere work would eventually live. Missing state must not be replaced by a plausible-looking generative guess and labelled measured.

## 5. Unlimited worlds without unlimited memory

A user may define hundreds or thousands of light/camera variants. The system stores mostly small descriptors:

`WorldState = baseMasterHandle + zeroLineBinding + sceneScaleBinding + hypothesisIds + lightDescriptors + virtualCaptureDescriptor`

A rendered full image is not stored for every WorldState.

When the user opens one world:

1. resolve only the affected SceneDomains;
2. build/reuse bounded Room Capsules;
3. materialize geometry only at the resolution required by the active tile/quality level;
4. render tile/ROI output lazily;
5. cache only rebuildable state allowed by the device Resource Governor;
6. stream final output when export is requested.

This preserves the existing TruthRaw rule that stronger hardware increases throughput/concurrency, not truth.

## 6. Device behavior

### Low-resource phone

- one active WorldState/expensive domain at a time;
- coarse admissible geometry LOD;
- small tiles;
- CPU correctness path;
- descriptor graph retained, rendered caches aggressively evicted.

### Mid-resource phone

- multiple compatible SceneDomains may be prepared;
- moderate geometry LOD and tile size;
- bounded cache reuse.

### High-resource phone

- several independent WorldState/domain jobs may be evaluated concurrently;
- higher admissible geometry LOD;
- larger bounded caches;
- optional accelerated backend after parity validation.

All three must produce the same result for the same explicitly selected quality/authority contract.

## 7. Photographer Notebook / Hypothesis Ledger

Every nontrivial World Studio assumption should be inspectable:

- source of the hypothesis;
- authority class: measured / reconstructed / counterfactual / appearance-only;
- support/confidence;
- affected SceneDomains;
- parent hypothesis if derived;
- whether changing it affects only rendering or also a calibrated forward simulation;
- reasons for `UNKNOWN` or non-intervention.

This is the photography equivalent of restoration provenance: if evidence is inadequate, keeping the Scientific Master unchanged is a successful result, not a failure.

## 8. Light behavior that FotoGraaf must eventually model

Simple scalar brightening is insufficient for serious photography. The research path should distinguish:

- direct versus indirect illumination;
- source angular size and shadow penumbra;
- inverse-distance effects for local sources;
- surface orientation and projected irradiance;
- visibility/occlusion and cast shadows;
- specular reflection direction and roughness;
- transmission/refraction for water/glass;
- wavelength-dependent source/material response;
- flare/veiling light as an optical observation effect rather than scene radiance;
- atmospheric scattering at distance;
- exposure/focus/motion as capture effects rather than world-light changes.

These effects should be added by authority tier, never all approximated at once and called physical truth.

## 9. Existing room binding

World Studio is not a replacement for the 12-room Building Runtime.

Suggested binding:

`Scene Registry / Surveyor -> World Studio descriptor graph -> Lighting Studio/CICM -> Room Capsule -> Colorist/Finisher -> Exporter`

The Architect/Restorer remain upstream reconstruction authority. The World Studio may consume their Scientific Master/support fields but may not write counterfactual results back into them.

## 10. Immediate implementation order

1. descriptor-only `WorldState` + `LightEmitter` + `SceneDomain` + authority enum;
2. bind WS0 exactly to current CICM relative-radiance semantics;
3. reuse Room Capsule for local materialization;
4. add Hypothesis Ledger and backplane-compatible world provenance handle;
5. add WS1 directional geometry-aware prototype with synthetic and held-out material stress tests;
6. only after independent calibration, open WS2;
7. keep WS3 fail-closed until geometry/material/spectral evidence exists.

## Permanent boundary

**FotoGraaf may explore an unlimited number of photographic worlds. None of those worlds becomes history unless a camera actually measured it.**
