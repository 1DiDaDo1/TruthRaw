# TruthRaw FotoGraaf Calibration Pack v0.1 — 2026-09-14

**Status: RESEARCH CALIBRATION CONTRACT / FAIL-CLOSED / NO SCIENTIFIC-MASTER ROUTE CHANGE**

This document defines what evidence is required before FotoGraaf Scene Metrology may promote a quantity from `INFERRED_SCENE` or source-bound metadata to `CALIBRATED_PHYSICAL`.

It does **not** turn calibration captures into scene evidence for a later photograph. Calibration evidence constrains the measurement operator; the photographed scene remains one physical frame / one independent scene-evidence root.

Permanent laws remain:

**Measured where measured. Reconstructed where necessary. Never invented.**

**Representation can exceed the source. Knowledge claims cannot exceed the evidence.**

---

## 1. Why a Calibration Pack is needed

A DNG can contain useful BlackLevel, WhiteLevel, NoiseProfile, color matrices, illuminant tags, GainMap/opcodes, exposure time, aperture and ISO metadata. Those values can provide reproducible source-bound interpretation, but their presence alone does not prove independently calibrated physical accuracy.

FotoGraaf therefore separates:

1. **source metadata authority** — what the sealed file says;
2. **empirical repeatability** — what controlled captures repeatedly demonstrate;
3. **relative physical calibration** — validated response relationships inside a declared scope;
4. **absolute physical calibration** — traceable radiometric/photometric quantities where the complete chain supports them;
5. **scene inference** — what the current photographed scene supports after applying the validated measurement model.

No later stage may silently collapse these classes.

---

## 2. Calibration is allowed to use multiple controlled captures

TruthRaw's single-frame law governs the evidence root of a photographed scene. It does not forbid a calibration laboratory from collecting many controlled frames to estimate the camera/lens/mode measurement operator.

Calibration frames are therefore a separate evidence domain:

`calibration evidence -> calibrated measurement model -> scene interpretation`

not:

`calibration frames -> extra photons/evidence for the photographed scene`.

A calibration dataset must never increase `physicalFrameCount` or `independentEvidenceCount` for the later scene.

---

## 3. Calibration identity key

Every pack is bound to an explicit `CalibrationScopeKey` at minimum containing:

- device/model identity;
- camera/system/lens identity;
- capture API/source domain;
- sensor/readout mode and dimensions;
- CFA topology and sample representation;
- capture sample-domain identity where applicable;
- firmware/build identity when it can alter the pipeline;
- lens state relevant to the measurement operator;
- temperature/environment range if the calibrated quantity is temperature-sensitive;
- calibration protocol version;
- calibration dataset manifest hash;
- calibration model/backend identity.

A mismatch is not "close enough". It downgrades the quantity to source-bound/inferred authority or fails closed, depending on the consumer contract.

`captureSampleDomainId` must not be inferred from ISO alone.

---

## 4. Pack modules

### C0 — identity and source-domain characterization

Purpose: prove exactly what capture domain was calibrated.

Required outputs:

- exact file hashes for every calibration capture;
- parser/backend identity;
- dimensions, CFA, bit depth/sample representation;
- BlackLevel/WhiteLevel and relevant DNG metadata;
- exposure time, aperture, ISO/gain/readout provenance;
- source GainMap/opcode identity;
- capture-sample-domain classifier output;
- device/lens/mode/build identity;
- thermal/environment observations where available.

Authority after C0: source-bound identity only. C0 by itself creates no independent physical radiometric claim.

### C1 — dark / offset / read-noise / hot-pixel characterization

Controlled dark captures are collected across the intended capture domains, exposure times, gain/ISO states and temperatures.

The pack may estimate:

- phase-dependent black offset and drift;
- row/column residual structure;
- read-noise distribution;
- dark-current/exposure-time dependence where measurable;
- hot/warm pixel stability;
- temporal stability and outlier behavior.

Required guard: do not replace a measured scene sample with a calibration average. The dark model constrains the likelihood/uncertainty model; the sealed scene CFA remains immutable.

### C2 — linearity, gain and saturation characterization

Use a stable controlled light field and an exposure/irradiance sweep that spans the usable sensor response without changing the scene geometry.

The pack may estimate:

- code-value versus exposure response;
- gain/conversion relationships;
- linearity/nonlinearity by capture domain;
- saturation onset and censoring behavior;
- repeatability/hysteresis where present;
- relationships between metadata ISO and actual capture-domain response.

A numeric correction is admitted only inside the domain in which its residuals and uncertainty were validated. Extrapolation beyond that domain is explicitly labelled unsupported.

### C3 — flat-field, lens shading and color-shading characterization

Use a spatially uniform, stable field appropriate to the lens/capture mode.

The pack may estimate:

- pixel/phase response non-uniformity;
- radial/spatial lens shading;
- color shading;
- stability versus focus, aperture, temperature or mode where these materially change the field.

If the source declares that lens shading was already applied upstream, calibration must characterize the **app-visible measurement operator** and must not blindly apply a second inverse shading correction.

### C4 — spectral/color calibration

Source DNG matrices remain useful source-bound color metadata, but independent color authority needs an independent calibration chain.

Depending on the intended claim, a calibration may use:

- controlled illuminants with characterized SPD;
- targets with measured spectral reflectance/transmittance;
- spectroradiometric reference measurements;
- multiple illuminant families across the intended operating domain;
- repeated captures and uncertainty estimates.

A color chart alone can validate reproduction under its tested illuminants/patches; it does not identify the full sensor spectral sensitivity functions or eliminate metamerism.

Outputs must explicitly declare whether they support:

- source-bound color reproduction;
- independently calibrated colorimetry in a bounded illuminant/target domain;
- or a stronger spectral claim.

The pack must fail closed against the stronger class when the required measurements are absent.

### C5 — relative radiometric scene calibration

This module establishes whether Scientific-Master values can be interpreted as stable **relative scene radiance** under a declared camera/lens/mode domain after capture-domain factorization.

Required evidence includes:

- C1/C2 validity;
- stable calibrated/reference light levels;
- controlled geometry;
- exposure/aperture provenance;
- repeatability across the intended range;
- uncertainty budget;
- validation captures not used to fit the model.

This can support relative radiometric comparisons without claiming an absolute SI radiance scale.

### C6 — absolute radiometric/photometric calibration (optional, stronger)

Absolute scene radiance or surface irradiance claims require a traceable external reference chain appropriate to the quantity being claimed.

Possible requirements include:

- calibrated radiance/irradiance source or reference instrument;
- documented spectral response/bandpass relationship;
- optical throughput/lens-mode binding;
- uncertainty traceability;
- geometry and angular conditions;
- validation over the claimed range.

**Radiance and irradiance remain different quantities.** A camera observation of a resolved surface does not automatically provide the incident irradiance at that surface.

Without this chain, FotoGraaf must keep absolute light-level values `INFERRED_SCENE`, relative, or `UNKNOWN` as appropriate.

### C7 — geometry / illumination validation rig (optional)

For testing local incident-light inference, use scenes where geometry, surface normals, material/reflectance and light placement are independently known or measured.

This module evaluates inference quality; it does not make arbitrary real-world geometry or BRDF magically known.

It may validate:

- dominant-light direction estimates;
- shadow/visibility reasoning;
- relative illumination fields;
- local normal/depth support;
- uncertainty calibration.

---

## 5. Fit set versus validation set

A calibration is not accepted from fit residuals alone.

Each calibrated model requires separate validation captures that were not used to fit that model. The report must preserve:

- fit-set identity;
- validation-set identity;
- residual distributions;
- uncertainty model;
- domain boundaries;
- failure cases;
- model/version hash.

No universal numeric PASS threshold is invented in this v0.1 document. Thresholds must be quantity-specific, physically justified and sealed in the calibration protocol before final promotion.

---

## 6. Temperature and state drift

Mobile sensors and processing paths can change with thermal state and capture mode. A Calibration Pack therefore records environmental/thermal state where available and tests whether a calibration remains valid across the intended operating range.

If drift exceeds the declared model uncertainty, TruthRaw must either:

- select a matching calibrated state/domain;
- widen uncertainty under a validated interpolation model;
- or downgrade/fail closed.

Performance-induced CPU temperature is an execution-resource issue unless it also changes the camera measurement operator used for capture. The two must not be conflated.

---

## 7. Calibration promotion ladder

A FotoGraaf quantity may be promoted only as follows:

`MEASURED_SOURCE`

→ source interpretation under exact parser/semantics

→ `CALIBRATED_PHYSICAL` only if a matching independent Calibration Pack module validates the claimed quantity

or

→ `INFERRED_SCENE` when a declared scene model is required and physical identifiability is incomplete.

Clipped/noise-limited values remain `BOUNDED_CENSORED` where appropriate even when the sensor is well calibrated.

Calibration does not convert censoring into a finite measured value.

---

## 8. Required machine-readable CalibrationPack record

A later implementation should serialize at least:

```text
CalibrationPack {
  schemaId
  packId
  calibrationScopeKey
  datasetManifestSha256
  protocolSha256
  modelSha256
  modulesPresent[]
  authorityClaims[]
  fitSetIds[]
  validationSetIds[]
  uncertaintyModelId
  validDomain
  excludedDomains[]
  environmentRange
  calibrationTimestamp
  instrumentIdentities[]
  traceabilityStatement
  status
}
```

`status` is one of:

- `RESEARCH_ONLY`
- `VALIDATED_RELATIVE`
- `VALIDATED_ABSOLUTE_FOR_DECLARED_QUANTITY`
- `REJECTED`
- `EXPIRED_OR_OUT_OF_SCOPE`

No pack may self-promote merely because its file is present.

---

## 9. Initial Honor BKQ-N49 priority

For the current Honor work, the scientifically useful order is:

1. capture-sample-domain classification;
2. dark/offset/read-noise characterization;
3. linearity/gain/saturation characterization;
4. flat-field/lens/color-shading characterization;
5. independently measured color calibration;
6. relative radiometric validation;
7. absolute radiometry only if a traceable external reference chain is available.

Per-lens/mode packs remain separate unless controlled evidence proves that a model can be shared.

---

## 10. Acceptance boundary

FotoGraaf Calibration Pack v0.1 changes **what can later be proven**, not the pixels of the current Scientific Master by itself.

Integration into reconstruction requires a separately validated measurement-model binding, exact scope match, uncertainty propagation, regression comparison and authority review.

Until then:

- the current Scientific Master route remains unchanged;
- DNG metadata remains source-bound metadata, not independent calibration proof;
- local incident-light inference remains `INFERRED_SCENE` unless a matching calibrated chain supports more;
- HDR display remains appearance/projection and is not calibration evidence.
