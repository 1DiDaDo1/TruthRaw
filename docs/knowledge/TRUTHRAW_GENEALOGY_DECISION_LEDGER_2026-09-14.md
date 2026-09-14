# TruthRaw genealogy and decision ledger — 2026-09-14

Status: **HISTORICAL + CURRENT DECISION PRESERVATION**

This file records how major TruthRaw concepts evolved. It is intentionally chronological/causal rather than only describing the latest architecture. The purpose is to prevent future sessions from losing why a rule exists, repeating rejected experiments, or confusing a historical prototype with current scientific authority.

## 1. Original vision: RAW as data, not a finished photograph

The foundational idea was to stop treating RAW as an image that merely needs conventional denoise/demosaic/sharpening and instead treat it as a finite physical measurement from which a best-supported latent scene can be estimated.

Conceptual inversion:

`RAW -> physical/statistical model -> best-supported latent scene estimate -> natural image`

rather than simply:

`RAW -> denoise -> demosaic -> sharpen -> JPEG`

The project never obtained permission to claim perfect unknowable truth. The target became the most defensible reconstruction with explicit uncertainty and provenance.

Early desired interaction:

`Open DNG -> Analyze RAW -> Reconstruct -> Show natural image -> Show uncertainty/evidence -> Export`

This directly led to later evidence classes such as measured, noise-limited, model-assisted, ambiguous, clipped/censored.

## 2. Early telephoto experiments and the move away from oversharpening

One of the early telephoto RAWs established concrete source-scale thinking: 4080x3072, BGGR, WhiteLevel 1023, low saturation fraction, ISO100 and f/2.6-class capture conditions.

Initial reconstruction experiments produced too much apparent sharpness and zipper/artifact behavior. User feedback explicitly pushed the project toward a **Soft Truthful** interpretation rather than maximizing local edge contrast.

Decision:

- sharp appearance is not automatically more truthful;
- reconstructed spatial detail and output acutance must remain distinct;
- later v4.7j/v4.7k appearance/detail work may not rewrite v4.7i scientific reconstruction.

## 3. Multi-Light / virtual EV

Multi-Light was introduced as multiple virtual exposure inspections of the same RAW.

Correction that became permanent:

- virtual EV can reveal different parts of the same finite measurement;
- it is a diagnostic/reparameterization;
- it does **not** create another exposure or another evidence root.

The later `Best Observation EV` idea therefore became diagnostic only, not a source-merging mechanism.

## 4. Chroma confidence and noise-aware color

Weak-channel amplification, especially in dark/artificial-light captures, showed that color confidence must not simply follow luminance confidence.

This led to concepts such as:

- confidence-gated chroma;
- coupled RGB noise-aware curves;
- luminance-first robustness;
- explicit channel support;
- avoiding amplification of tiny noisy channel differences into false color.

Later artificial-light evidence strengthened this rationale, especially where blue-channel evidence was weak and WB gains were large.

## 5. Float32 as representation freedom

The project moved from integer-normalized sensor values into float32 scene representations so that scientific state could preserve:

- signed post-black values;
- values above normalized sensor/display unity;
- reconstructed full-color values;
- uncertainty and model state without clipping to output conventions.

An early float32 linear-scene TIFF was a major precursor to the Scientific Master.

Permanent correction:

**float32 is richer representation, not more measured bits.**

The phrase `alle vrijheid` / new house later captured this distinction: numerical freedom without evidence inflation.

## 6. Sealed house / verzegelde woning

The original RAW became the **sealed/old house**: immutable evidence that is never repainted.

The reconstructed float scene became the **new house**: richer, editable as a derived scientific state, but never allowed to rewrite the original evidence.

This separated:

- source preservation;
- reconstruction;
- appearance;
- export.

It also provided the conceptual foundation for the later Gatehouse/Main-House boundary.

## 7. Scientific Master formalization

The early scene estimate evolved into a formal Scientific/Latent Scene Master.

Important current meaning:

- full-resolution;
- full-color;
- camera-native RGB in the present project-level definition;
- before `camera_to_xyz()`;
- before appearance;
- signed float representation;
- accompanied by uncertainty/support/censor/model/provenance state.

A reconstructed CFA DNG is therefore not the Scientific Master. It is a compatibility projection.

Historical colorimetric/device-independent scene-master experiments remain important provenance but do not silently redefine the current master.

## 8. Saturation and censoring correction

Early temptation: treat saturated RAW values as exact values or reconstruct a single confident highlight value.

Correction:

- saturation means the latent quantity is censored at/above a threshold;
- the source supports a bound, not an exact above-threshold measurement;
- above-threshold point estimates are model-assisted and must be labeled as such;
- calibration may tighten a censor model but may never `uncensor` a source-clipped measurement.

This is now built into the later FotoGraaf shadow-model rules.

## 9. v3 reconstruction restriction and v4.7i measured-preserving baseline

Earlier v3i/v3j research was later restricted/withdrawn after exact-kernel/backend mismatch concerns.

The project stabilized around v4.7i as the scientific reconstruction baseline:

- physically measured CFA component preserved/reinjected;
- only missing RGB components reconstructed;
- directional/edge-aware support-limited interpolation in the research backend;
- no creative appearance authority.

v4.7j/v4.7k became detail/acutance/appearance layers only.

Decision:

confidence claims must bind the exact backend/schema/protocol they were validated against.

## 10. Nul-lijn / midden lijn / TruthRange

The project explored a `middle line` / zero-line idea to remove arbitrary finite display/container limits from the new house.

Final corrected interpretation:

`T = log2(L/L0)`

where `L0` is a gauge/reference.

The representational address space extends both directions:

`darkness <- ... <- -EV <- nul-lijn -> +EV -> ... -> brighter light`

Corrections that became canonical context:

- zero-line is not sensor black;
- zero-line is not physical darkness;
- zero-line is not display middle grey;
- infinite mathematical address space does not imply infinite sensor dynamic range;
- signed scene-linear values must be retained separately because negative post-black numerical estimates are scientifically useful even though physical light is non-negative.

Virtual EV shifts this gauge only; it does not create evidence.

## 11. Achterkant van de foto -> Technical Backplane

The historical idea of an inspectable `backside of the photo` evolved into a formal Technical Backplane.

The backside is not another image. It is compact provenance/identity state binding:

- source;
- master;
- zero-line;
- scene scale;
- evidence counts;
- room/claim/projection status.

A compact fixed binary representation was later promoted. That created an important governance rule:

**do not silently mutate a frozen Backplane layout when a new research field appears; version the schema.**

This rule now matters for future capture-domain/calibration identifiers too.

## 12. Foto graaf -> local scientific investigator

The phrase `Foto graaf` began as the idea of deeply investigating the photo/world behind visible pixels.

It was corrected away from unrestricted world invention toward a local scientific investigator:

- select a local scene domain;
- create a temporary Room Capsule;
- attach geometry/material/light hypotheses only where needed;
- simulate counterfactual illumination/capture separately;
- never back-propagate simulated evidence into the original capture.

Permanent law:

**A simulated world may generate new hypothetical measurements; it never retroactively creates new evidence for the captured world.**

The idea that only the affected local illuminated domain should be simulated became:

**Never simulate the whole world when only the illuminated local domain is required.**

## 13. Waterdruppels / water-material stress tests

Water/droplets later became a useful stress-test domain because tiny transparent/specular/refractive objects expose weaknesses in:

- optics;
- fine detail reconstruction;
- highlight censoring;
- material decomposition;
- reflection/transmission ambiguity;
- color invention.

The project learned not to invent generic blue/cyan water or synthetic highlight peaks where the RAW does not support them.

Exact historical origin of the literal word `waterdruppels` is not sufficiently documented to claim it as a foundational phrase. Preserve that uncertainty.

## 14. Constructed RAW / Virtual Ideal Camera period

The project explored building new RAW/DNG representations from reconstructed scene state. This was useful for understanding representation freedom but created a danger: a generated/re-Bayered file could be mistaken for original sensor evidence.

Correction:

- constructed CFA/DNG is not source truth;
- re-Bayer is compatibility output only;
- a richer reconstructed RGB master should be exported directly when possible rather than remosaiced and demosaiced again.

This period remains research provenance, not current source authority.

## 15. RGB RAW weer terug

A major architectural recovery occurred when full-color RGB Scientific Master output returned as the primary scientific route.

Correct direction:

`camera-native reconstructed RGB -> cameraToXyzD50 -> float32 RGB/XYZ LinearRaw DNG`

rather than:

`RGB -> artificial Bayer -> DNG -> another demosaic`

This restored a cleaner separation:

- Direct CFA = measured evidence;
- Scientific Master = reconstructed full-color state;
- LinearRaw = direct full-color projection;
- reconstructed CFA DNG = compatibility projection.

This correction directly enabled the later TRUTHRAW PURE float32 DNG path.

## 16. Goedkope en dure telefoon

A performance discussion established that scientific truth must not depend on hardware tier.

Permanent rule:

**TruthRaw uses the same scientific decisions on cheap and high-end phones. Lower-resource devices may use smaller tiles, fewer workers and less cache. They do not receive lower truth.**

This later became the distinction between truth floor and resource profile, and motivated worker-invariance tests.

## 17. Tussenwoning -> Gatehouse

External RAW decoding complexity led to the `tussenwoning` metaphor.

The Gatehouse sits between sealed source and Main House only when needed:

`sealed source -> isolated decode/audit -> sealed handoff -> detach -> Main House`

Correction:

- Gatehouse cannot become another source of truth;
- decoder support/version is provenance only;
- unsupported/ambiguous topology fails closed;
- external decoder buffers/context do not persist into the scientific Main House.

This protects source identity and keeps external tooling from silently controlling TruthRaw authority.

## 18. Light-transport fact-check

The project repeatedly revisited how light should `fall` on objects.

Corrections preserved:

- inverse-square law is context-dependent and applies to irradiance from point-like sources under specific assumptions;
- resolved-surface brightness in a camera is not generally scaled by object-to-camera `1/r^2`;
- radiance is conserved along free-space rays absent medium loss;
- shadows usually retain indirect illumination;
- specular highlights are poor base-color evidence;
- participating media require volumetric treatment;
- local relight from one photograph is underdetermined without geometry/material/illumination constraints.

This blocked naive `distance makes object darker` priors in scientific reconstruction.

## 19. Color-echtheid evolution

The project ambition around `kleur echtheid` evolved from trying to find one perfect object RGB toward calibrated/reproducible color authority.

Corrections:

- sensor RGB depends on illuminant, reflectance/transmittance, optics and sensor spectra;
- metamerism prevents unique spectral truth from ordinary RGB alone;
- white balance is neutral adaptation, not discovery of unique object truth;
- source DNG matrices can provide reproducible source-bound color without becoming independent physical calibration;
- full physical color requires controlled independent calibration and remains blocked otherwise.

The desired natural look can be downstream appearance while scientific color authority remains explicit.

## 20. Artificial light and dark scenes

Dark/artificial-light RAWs exposed a critical failure mode: very large WB gains, especially blue, can amplify weak noisy evidence.

Correction:

- reconstruct/denoise in camera-native coordinates before large display/neutral gains when practical;
- preserve per-channel confidence;
- separate sensor evidence, neutral adaptation, appearance and spectral/color calibration;
- do not declare a scene physically color-accurate simply because a neutral surface is made grey.

## 21. Lens/optics authority

The lens became formally part of the measurement operator.

Research considered PSF/MTF/CA/flare/shading/color-shading and star-field measurements.

Correction:

- source GainMap/opcodes are deterministic source correction, not a complete inverse optical model;
- no uncalibrated deconvolution may be called physical recovery;
- optical inverse remains BLOCKED pending controlled calibration.

## 22. Full-sensor/high-resolution correction

A 200 MP-class tele sensor mode prompted a distinction between:

- reconstructing a high-resolution image; and
- proving actual camera-visible RAW sensor lattice/evidence.

The project shifted toward proving exact Camera2 mode, dimensions, payload, stride, timestamp and source identity.

A processed 200 MP JPEG does not prove a 200 MP RAW path.

## 23. Streaming / tile architecture

Large scientific state led to bounded/tiled streaming rather than full-frame duplication.

The strict native-DNG reader intentionally supports a narrow verified subset and fails closed on unsupported topology.

Read optimization later cut exact tile reads from 13,824 to 7,680, but peak PSS did not improve. This created a useful performance-governance lesson:

**a speed/read-count win must not be rewritten as a memory win when the memory evidence does not support it.**

## 24. Physical empirical noise evidence and ISO-domain discontinuity

Honor tele ISO/dark campaigns moved the project from metadata-only noise assumptions toward source-bound empirical evidence.

Important sequence:

- ordinary ISO ladder showed monotonic source `NoiseProfile` growth and roughly matched output code levels at ISO400+;
- same-condition covered dark pairs provided direct temporal sigma estimates around 0.79 DN at ISO100 and ~0.87 DN at ISO400;
- exact ISO8192 repeatedly selected a radically different high-scale/censored dark sample domain around 54-55 DN sigma and ~7.1% raw-zero censoring;
- nearby ISO8184 and ISO10244 at matched exposure remained ordinary.

Critical correction:

**simple monotonic `ISO >= 8192` domain selection is rejected.**

New rule:

**empirical noise calibration must be bound to exact capture/sample-domain identity, not ISO magnitude alone.**

Physical cause remains OPEN. Do not label it DCG/analog gain/sensor bug/decoder bug without causal evidence.

## 25. CalibrationPack architecture

The project then formalized independent calibration as a separate evidence system:

`controlled calibration captures -> dataset manifest -> validated CalibrationPack -> exact scene admission -> binding`

Calibration captures can be numerous because they estimate a camera model, but they never become extra scene evidence for a later single photograph.

The scope must bind camera/system/lens/API/mode/dimensions/CFA/sample domain/firmware/focus/stabilization and runtime measurement state.

ISO alone is insufficient.

## 26. FotoGraaf Calibration Scene Admission

Runtime admission was built fail-closed.

A scene only receives `CALIBRATED_PHYSICAL` for a requested quantity when:

- exact hard scope matches;
- model claim is validated;
- runtime gain/readout state is in domain;
- shutter/ISO metadata/f-number are in validated ranges;
- thermal domain is satisfied when temperature is part of calibration;
- evidence counts remain 1/1.

Worker count is excluded from authority.

Product `HDR` cannot masquerade as a calibration claim.

## 27. Calibration measurement-model shadow route

The next step deliberately avoided changing production science immediately.

A research-only shadow adapter compares:

`current source-bound measurement interpretation || exact calibrated interpretation`

before any calibrated path is allowed to influence the Scientific Master.

It preserves:

- source Stage-2 arithmetic shape;
- signed below-black values;
- source censoring;
- exactly-once GainMap;
- model/protocol/source binding;
- worker invariance.

A noise-only calibration is explicitly tested not to move the signal coordinate.

This is a conservation-style validation strategy: diagnose and compare before changing the authoritative route.

## 28. CI failure preserved as knowledge

The first Release CI attempt for the shadow model failed because the test program used ordinary C/C++ `assert()` while the Release build defined `NDEBUG`.

Consequences:

- assertions compiled out;
- variables/functions used only by assertions became `unused` under `-Werror`;
- GCC/Clang/sanitizer matrix failed.

The correct fix was **not** weakening `-Werror` or hiding the failure. Tests were changed to use an always-active `require()` style check so Release actually validates scientific invariants.

This failure is preserved because it proves why test semantics must remain active in Release validation.

## 29. Four-mode product architecture

The product surface converged on:

- JPG
- JPG XL
- TRUTHRAW PURE
- TRUTHRAW ADVANCED

Appearance controls:

- Colourful
- Detailed
- Soft
- HDR

PURE is appearance-neutral; unknown mode fails safe to PURE; JPEG XL remains fail-closed until a validated encoder exists.

This architecture reflects the long-standing science/appearance separation rather than a new scientific model.

## 30. Certificate and technical backside

The in-file TruthRaw certificate extended the Technical Backplane idea into export provenance.

Correction preserved:

- certificate is not a visible watermark;
- unsigned development is not verified authenticity;
- no private signing key belongs in repo/APK;
- integrity binding does not itself prove physical truth;
- frozen certificate fields must be versioned rather than silently expanded.

## 31. Existing-app integration

TruthRaw can be embedded in another Android app, but only if the scientific core remains isolated.

The project rejected the idea of treating TruthRaw as an ordinary filter inserted after host-app denoise/WB/tone/re-Bayer processing. Host UI/storage can remain, while TruthRaw receives sealed source evidence and produces controlled outputs.

## 32. Current decision matrix

### PASS / accepted direction

- immutable Direct-CFA source evidence;
- separate Scientific Master;
- float32 signed/full-color scientific representation;
- v4.7i measured-preserving baseline;
- TruthRange/nul-lijn as gauge, not black;
- Technical Backplane as compact provenance backside;
- FotoGraaf as local investigator/counterfactual subsystem;
- Gatehouse isolation for external RAW;
- RGB/LinearRaw direct scientific projection;
- hardware/resource invariance of science;
- exact capture-domain-bound calibration;
- calibration shadow comparison before production promotion;
- four-mode output separation.

### REJECTED

- constructed RAW treated as source truth;
- generative scene content treated as measured truth;
- virtual EV treated as extra evidence;
- display black as zero-line;
- generic object-to-camera inverse-square brightness prior;
- R/B swap plus double orientation workaround;
- simple monotonic ISO>=8192 calibration-domain rule;
- GCam/APK/computational-RAW as TruthRaw scientific authority.

### BLOCKED

- full optical inverse;
- independent full physical color/spectral truth;
- production calibrated measurement route until calibration packs pass;
- trusted signing;
- JPEG XL production output;
- any claim that current test captures alone are calibration evidence.

## 33. Genealogy trigger keywords

When any of these words reappear, recover this genealogy rather than reading the word in isolation:

`kleur echtheid`
`licht`
`pure RAW`
`geen verzinsels`
`single-frame`
`constructed RAW`
`Multi-Light`
`achterkant foto`
`Foto graaf`
`verzegelde woning`
`waterdruppels`
`midden lijn`
`float32`
`goedkope en dure telefoon`
`RGB RAW weer terug`
`tussenwoning`

Required reconstruction pattern:

`motivation -> hypothesis -> experiment/source -> finding -> limitation/failure -> fact-check -> correction -> later integration -> current meaning`

That reconstruction pattern is itself part of the project preservation protocol.
