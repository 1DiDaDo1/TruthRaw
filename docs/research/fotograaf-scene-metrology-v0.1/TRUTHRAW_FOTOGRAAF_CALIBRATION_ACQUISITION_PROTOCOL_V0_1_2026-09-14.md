# TruthRaw FotoGraaf Calibration Acquisition Protocol v0.1 — 2026-09-14

**Status: RESEARCH PROTOCOL / FAIL-CLOSED / NO SCIENTIFIC-MASTER ROUTE CHANGE**

This document completes the acquisition side of `FotoGraafCalibrationPack v0.1`. The earlier pack document defines the authority boundary; this protocol makes the required controlled measurements concrete enough to build a machine-verifiable pack.

The purpose is not to make reconstruction more aggressive. The purpose is to decide, quantity by quantity, whether FotoGraaf may move from `INFERRED_SCENE` to `CALIBRATED_PHYSICAL`.

Permanent laws remain:

**Measured where measured. Reconstructed where necessary. Never invented.**

**Representation can exceed the source. Knowledge claims cannot exceed the evidence.**

Calibration captures constrain the camera/lens/mode measurement operator. They never become additional scene photons for a later photograph, so a normal photographed scene remains `physicalFrameCount=1` and `independentEvidenceCount=1`.

---

## 1. Scope lock: calibration belongs to an exact camera/lens/mode domain

Before any dark, flat or light sweep is accepted, C0 must bind the pack to one exact `CalibrationScopeKey`.

Required identity fields are:

- device make and model;
- Android camera/system identity and physical camera identity where exposed;
- lens role;
- capture API/source domain, for example MotionCam direct RAW versus vendor DNG;
- capture mode and RAW dimensions;
- CFA topology;
- sample representation / packing domain;
- `captureSampleDomainId`;
- firmware/build identity;
- focus-state class;
- stabilization state where it can alter the optical measurement;
- calibration protocol version.

Also record for every frame:

- exact SHA-256;
- exposure time;
- aperture/f-number where fixed or reported;
- ISO metadata and any gain/readout identifiers available;
- BlackLevel and WhiteLevel;
- DNG NoiseProfile identity;
- GainMap/opcode identity;
- raw IFD/sample topology;
- temperature observation and temperature sensor identity;
- parser/backend identity used to interpret the frame.

A scope mismatch is not rounded or interpolated by convenience. It is either handled by an independently validated interpolation model or the physical claim is downgraded/fails closed.

`captureSampleDomainId` is never inferred from ISO magnitude alone.

---

## 2. C1 — dark / offset / read-noise acquisition

A dark is valid only when photon input to the camera is physically blocked. A photographed black object or a dark room is not equivalent to a dark frame because it still contains scene photons, stray light and flare.

For **every captureSampleDomainId and every gain/readout state intended for calibrated use**, collect darks at no fewer than four exposure times:

1. the shortest exposure relevant to the intended domain;
2. one normal operating exposure;
3. one long operating exposure;
4. the longest exposure relevant to the intended domain.

Any detected gain/readout transition must be bracketed with states immediately below and above the transition.

For every `gain/readout × exposure × temperature` cell, acquire **at least 16 independent dark frames**. More frames may reduce estimator uncertainty; 16 is the minimum admission count, not a claim that 16 is universally optimal.

C1 must preserve, separately by CFA phase/channel where appropriate:

- black-offset location and distribution;
- row/column residual structure;
- read-noise statistics;
- dark-current dependence on exposure time where identifiable;
- hot/warm pixel frequency and persistence;
- temporal drift and non-Gaussian/outlier behavior.

No C1 model may overwrite a measured scene CFA value with a calibration mean. It changes the likelihood/uncertainty model only through a separately validated measurement-model binding.

### Temperature requirement

A temperature-dependent calibration claim requires at least **three measured temperature bins** spanning the declared operating domain. Each bin must satisfy a protocol-sealed soak/stability condition before acquisition.

Preferred temperature sources, in order of authority:

1. camera/sensor/HAL temperature if exposed;
2. a documented internal device sensor with known physical relation to the camera module;
3. an external probe physically coupled to the camera module with declared uncertainty.

Ambient temperature, battery temperature and CPU temperature are useful context but are not silently treated as sensor temperature.

If no suitable temperature observation exists, the pack may still validate a narrowly scoped state but may not claim a temperature-compensated physical model. If a pack does claim temperature calibration, every validated module used by that temperature-dependent claim must cover the declared temperature bins; a C1-only temperature sweep is not enough to justify temperature compensation of C2/C3/C4/C5/C6/C7 quantities.

---

## 3. C2 — linearity, gain, ISO-response and saturation acquisition

Use a stable spatially uniform source. The source may be an integrating sphere, calibrated flat-field source or another setup whose temporal stability and spatial uniformity are measured.

For every gain/readout state in scope:

- acquire at least **12 signal levels** from just above the noise floor through the high unsaturated region;
- acquire at least **8 repeats per signal level**;
- include low, mid and high unsaturated levels;
- include at least **3 levels that bracket saturation onset**;
- reserve at least **4 independent validation levels not used for fitting**;
- acquire matching dark references;
- continuously or repeatedly monitor source stability.

The sweep must change one controlled exposure quantity at a time. If exposure time is used to scale signal, source output, aperture, lens state, sensor mode and gain/readout state remain fixed. If source intensity is changed instead, the reference instrument must record the actual change.

C2 estimates may include:

- code-value versus exposure relationship;
- analog/digital gain relationship where identifiable;
- ISO metadata versus actual response;
- linearity and residual nonlinearity;
- saturation onset;
- repeatability;
- capture-sample-domain transitions.

### ISO authority

ISO remains capture provenance. FotoGraaf may calibrate the response associated with an ISO/readout state, but ISO itself is not a scene-light measurement and may not be erased from provenance.

### Actual capture dynamic range

FotoGraaf must keep **scientific capture dynamic range** separate from display HDR.

A calibrated capture-domain dynamic range claim requires C1 + C2 and reports at least:

- saturation/usable-unsaturated signal model;
- noise-floor model;
- an engineering `SNR=1` bound;
- a second usable-range result using an SNR threshold that was sealed in the protocol before final model fitting.

A valid form is:

`DR_stops = log2(usable_unsaturated_signal / noise_floor_signal)`

The exact numerator/denominator estimators and SNR criterion must be serialized with the model.

Calibration may tighten the saturation boundary. It does not turn a clipped scene sample into a finite measured highlight value; clipping remains `BOUNDED_CENSORED`.

---

## 4. C3 — flats, PRNU, lens shading and color shading

Use a uniform diffuse source or integrating sphere with characterized spatial uniformity.

For every lens/mode/focus-state class intended for calibrated use:

- capture at least **3 signal levels**, approximately low (~10%), middle (~50%) and high-unsaturated (~80%) of the usable range;
- capture at least **8 repeats per level**;
- capture matching darks;
- record focus state and any optical stabilization state;
- preserve CFA phase/channel statistics;
- record source uniformity measurement and uncertainty.

C3 may estimate:

- pixel/phase response non-uniformity;
- lens shading;
- color shading;
- spatial stability with signal level;
- dependence on focus/mode/temperature where included in scope.

If the source pipeline reports that lens shading was already applied upstream, C3 characterizes the **app-visible measurement operator**. TruthRaw must not apply a second inverse shading correction merely because a raw sensor model usually contains vignetting.

Per-lens/mode packs remain separate until controlled evidence proves that a model is shareable.

---

## 5. C4 — independent color / spectral calibration

DNG matrices remain source-bound metadata. Promotion to independent physical color requires an external color/spectral reference chain.

Minimum C4 acquisition:

- at least **3 characterized illuminants** covering warm, middle and cool/daylight regimes;
- nominal targets around 2850 K, 5000 K and 6500 K are useful anchors, but **measured SPD is mandatory**; CCT alone is insufficient;
- at least **24 target patches** including neutrals and chromatic patches;
- measured or independently specified spectral reflectance/transmittance for the target when claiming independent physical color;
- recommended spectral sampling interval no coarser than **10 nm** over the instrument/target usable band;
- at least **3 repeats per illuminant**;
- a held-out illuminant or held-out patch subset for validation;
- no clipped target patches.

External records must include instrument model, serial, calibration state, spectral range/bandpass and measurement uncertainty.

A chart without spectral reference data can test reproduction in the observed setup, but it does not identify complete sensor spectral sensitivities and does not remove metamerism.

---

## 6. C5 — relative scene-radiance calibration

C5 is the first module that may support a `relative_scene_radiance` promotion.

Dependencies: valid C0, C1, C2 and C3.

Acquire:

- at least **8 independently referenced radiance/light levels** across the intended domain;
- at least **5 repeats per level**;
- at least **2 held-out validation levels**;
- fixed and documented geometry;
- exact exposure/aperture provenance;
- measured reference stability over the run.

The external reference does not need SI-traceable absolute calibration for C5, but its stability, response and uncertainty must be characterized well enough to support the declared relative scale.

C5 never authorizes incident irradiance at arbitrary scene surfaces. Camera-observed directional radiance and incident irradiance remain different quantities.

---

## 7. C6 — absolute radiometric / photometric calibration

C6 is optional and substantially stronger. It is required before an absolute scene-radiance or absolute photometric quantity can become `CALIBRATED_PHYSICAL`.

Dependencies: valid C0, C1, C2, C3 and C5.

The external reference chain must provide:

- radiance/spectral-radiance or other quantity appropriate to the claim;
- instrument model and serial;
- calibration-certificate identity;
- certificate validity at the acquisition date;
- traceability statement;
- measurement uncertainty;
- spectral bandpass/response;
- geometry and angular conditions.

Acquire at least:

- **5 absolute reference levels** spanning the claimed domain;
- **5 repeats per level**;
- held-out validation observations.

The pack must serialize units and uncertainty. A lux meter alone cannot establish spectral radiance, and an image of a resolved surface does not by itself establish that surface's incident irradiance.

---

## 8. C7 — geometry / incident-light validation rig

C7 does not make arbitrary real-world lighting identifiable. It validates how well FotoGraaf's incident-light inference behaves in a controlled scene where the missing physical variables are independently known.

Minimum rig:

- measured scene geometry;
- known/measured surface normals;
- material reflectance or BRDF reference;
- measured light position and direction;
- a **cosine-corrected irradiance reference** at the tested surface;
- at least **3 distinct light directions**;
- at least **3 distinct light levels or source distances**;
- shadow/visibility ground truth.

C7 can validate dominant-light direction, relative illumination fields, shadow reasoning and local incident-irradiance inference inside the tested domain.

It may not promote a general unconstrained single-image lighting decomposition to certainty.

Counterfactual/CICM renders may be used as tests or hypotheses but can never be the sole evidence for an original-world calibrated claim.

---

## 9. Promotion matrix

A quantity may carry `CALIBRATED_PHYSICAL` only when every listed module is present, independently validated and scope-matched.

| Quantity | Required modules |
| --- | --- |
| black offset / read noise | C0 + C1 |
| calibrated gain/response | C0 + C1 + C2 |
| capture-domain dynamic range | C0 + C1 + C2 |
| lens/color shading | C0 + C2 + C3 |
| independent colorimetry | C0 + C2 + C3 + C4 |
| relative scene radiance | C0 + C1 + C2 + C3 + C5 |
| absolute scene radiance | C0 + C1 + C2 + C3 + C5 + C6 |
| validated incident-light inference | C0 + C2 + C7 |
| absolute incident irradiance | C0 + C1 + C2 + C3 + C5 + C6 + C7 |

For every calibrated claim, fit and validation datasets are disjoint. Final admission thresholds are fixed before the final fit is evaluated. A low training residual is never sufficient by itself.

---

## 10. What this means for ISO, real HDR and light incidence

This protocol lets FotoGraaf answer three previously ambiguous questions in a physically disciplined way.

**ISO:** after C1/C2, FotoGraaf can learn the actual gain/readout/noise behavior of the exact capture domain instead of treating ISO as scene brightness.

**Real HDR range:** after C1/C2, FotoGraaf can report a calibrated capture-domain dynamic-range envelope in stops, including uncertainty and censoring boundaries. This is scientific scene/capture range, not the JPG/HDR appearance toggle.

**Light incidence:** C0-C6 can improve radiance interpretation, but local incident-light values remain `INFERRED_SCENE` until a C7-like geometry/material/light reference validates the particular inference model. A relative/structural incident-light validation may use C7; an absolute incident-irradiance claim additionally requires the C6 absolute chain and a traceable cosine-corrected irradiance reference. Even then, authority is bounded to the validated domain.

---

## 11. Initial Honor BKQ-N49 execution order

For the current Honor program, calibration should be built separately per lens/mode/source domain. The telephoto path remains the first priority because it is the most scientifically useful stress case for low light, long focal length, stabilization and high-gain behavior.

The next acquisition sequence is:

1. C0 exact scope seal and capture-domain classification;
2. C1 photon-blocked dark cube across gain/exposure/temperature;
3. C2 uniform-source signal sweep including saturation brackets;
4. C3 three-level flat-field/focus-state sweep;
5. C4 characterized illuminant + spectral target measurements;
6. C5 relative radiance ladder;
7. C6 only when a traceable radiometric chain is available;
8. C7 controlled local-light rig.

Existing ordinary scene ISO ladders remain valuable **test evidence** for domain discovery and regression, but they do not substitute for the controlled C1/C2 calibration acquisitions above.

---

## 12. Machine admission

The companion file `TRUTHRAW_FOTOGRAAF_CALIBRATION_CONTRACT_V0_1.json` records these acquisition minima and dependencies.

`tools/verify_fotograaf_calibration_pack_v0_1.py` provides a fail-closed admission gate. It verifies scope identity, SHA-256 bindings, evidence-count invariance, fit/validation separation, module acquisition minima and claim dependencies.

Passing the validator means the **pack record is structurally eligible for the claimed authority**. It does not prove that a laboratory instrument was honest or that the physical measurements were performed correctly. Those remain empirical/traceability responsibilities.

Until a real pack passes both laboratory validation and this admission gate:

- Honor absolute radiometry remains **OPEN**;
- capture-domain dynamic range remains source/test-derived rather than `CALIBRATED_PHYSICAL`;
- local incident-light inference remains **INFERRED_SCENE**;
- the existing Scientific Master route remains unchanged.
