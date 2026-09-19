# TruthRaw full genealogy recovery audit — PURE / Open World / TruthNegative — 2026-09-19

Status: **RESEARCH RECOVERY AUDIT / DOCUMENTATION ONLY / NO SCIENTIFIC PIXEL OR AUTHORITY CHANGE**

Branch:

`research/full-genealogy-pure-openworld-recovery-v0-1`

Base:

`research/truthnegative-v0-2-existing-house-binding`

Historical output/PURE source branch:

`research/ui-output-modes-certificate-v0.1-2026-09-14`

## 1. Why this audit exists

The project genealogy is broader than the latest Camera-5 / TruthNegative branches. A historically recovered sequence is:

`RAW as measurement`
→ `MEASURED / RECONSTRUCTED / UNKNOWN`
→ `one physical exposure`
→ `reconstructed full-colour scene / RGB RAW returned`
→ `sealed source versus new scene`
→ `unbounded new house/world`
→ `middle line / zero-line`
→ `virtual observations without extra evidence`
→ `rooms + resource-invariant devices`
→ `digital backside / Technical Backplane`
→ `Gatehouse / tussenwoning`
→ `materials / water droplets / detail`
→ `FotoGraaf room intuition + counterfactual lighting`
→ `broad FotoGraaf = photography + metrology`
→ `physical calibration / C0-C1-C2 and broader C0-C7 ladder`
→ `Android FotoGraaf as acquisition instrument`
→ `physical Camera 5 + sample-domain proof`
→ `current Camera-5/200MP forensic line`.

The purpose of this document is to map that genealogy onto the repository as it actually exists now and to identify concepts or proven implementations that live on diverged historical branches.

Permanent law remains unchanged:

> **Measured where measured. Reconstructed where necessary. Never invented.**

> **Representation can exceed the source. Knowledge claims cannot exceed the evidence.**

## 2. Branch-divergence finding

The 2026-09-14 output/PURE branch and the later Open-World/TruthNegative line are not one linear Git history.

Direct compare established that:

- `research/ui-output-modes-certificate-v0.1-2026-09-14` vs `research/open-world-foundations-v01`: **diverged**;
- the same historical branch vs `integration/current-truthraw-state-2026-09-16`: **diverged**;
- the same historical branch vs `research/truthnegative-v0-2-existing-house-binding`: **diverged**;
- observed common merge-base: `514f2f4bde6aba5a6709e176c03b22c3b9aea912`.

Therefore absence of a file from the current branch does not imply that the historical implementation never existed.

## 3. Genealogy-to-module map

| Genealogy step | Historical / current repository anchor | Current interpretation |
|---|---|---|
| RAW as measurement | sealed-source architecture, TileNative DNG source, current scientific architecture | **CURRENT** — source bytes/CFA are evidence, not appearance |
| MEASURED / RECONSTRUCTED / UNKNOWN | uncertainty, Dynamic Authority, Open Scene contracts | **CURRENT / EXPANDED** — now includes calibrated estimate, censored, counterfactual, appearance-only roles |
| one physical exposure | project-wide 1/1 evidence invariants | **CURRENT** — virtual views do not increase evidence count |
| reconstructed full-colour scene / “RGB RAW returned” | 2026-09-14 genealogy + Scientific Master | **CURRENT CONCEPT; HISTORICAL OUTPUT IMPLEMENTATION DIVERGED** |
| sealed source vs new scene | sealed-house/new-house history; Free Scientific Space | **CURRENT / FORMALIZED** |
| unbounded new house/world | zero-line/TruthRange → Free Scientific Space | **CURRENT / FORMALIZED** — representationally open, evidence finite |
| middle line / zero-line | TruthRange / L0 architecture | **CURRENT** |
| virtual observations without extra proof | Virtual Observation Manifold | **CURRENT HISTORICAL MODULE; authority retained** |
| rooms / resource-invariant devices | Building Runtime / Room ABI / bounded streaming | **CURRENT PRINCIPLE** — hardware changes execution, not truth |
| backside / Backplane | Technical Backplane v0.1 + phase 2 | **CURRENT / FROZEN VERSIONED RECORD** |
| Gatehouse / tussenwoning | house genealogy + ingress architecture | **CURRENT ARCHITECTURE, PARTIAL IMPLEMENTATION** — native DNG ingress exists; generic URI ingress exists; full external RAW Gatehouse is not fully reimplemented |
| materials / water droplets / detail | knowledge ledger, scene-physics, restoration, structure authority | **CURRENT CONSTRAINTS; HISTORICAL STRESS-TEST CONTEXT** |
| FotoGraaf room intuition / counterfactual lighting | Room Capsule, CICM, FotoGraaf Scene Metrology | **CURRENT CONCEPT, but not a 13th canonical room** |
| broad FotoGraaf = photography + metrology | FotoGraaf Scene Metrology architecture | **HISTORICAL IMPLEMENTATION/CONTRACT DIVERGED; concept preserved by later architecture** |
| physical calibration C0/C1/C2 | FotoGraaf Calibration Pack C0-C7, C0 gate, C1/C2 intake | **HISTORICAL IMPLEMENTATION DIVERGED; obligations survive** |
| Android FotoGraaf acquisition instrument | `com.truthraw.fotograafcapture` Camera2 acquisition companion | **HISTORICAL IMPLEMENTATION DIVERGED; later Camera-5 acquisition line continues the physical-proof goal** |
| physical Camera 5 + sample-domain proof | Camera-5 v0.14→v0.20 and later replay/oracles | **CURRENT FORENSIC LINE** |
| current Camera-5/200MP interpretation | current Camera-5 RAW route + v0.53/v0.54 evidence | **CURRENT BOUNDED INTERPRETATION** — 16320×12288 envelope is not proof of 200MP measured CFA |

## 4. RGB RAW / Scientific Master recovery

The historical genealogy explicitly records:

`camera-native reconstructed RGB -> cameraToXyzD50 -> float32 RGB/XYZ LinearRaw DNG`.

The Scientific Master itself remains:

- camera-native;
- scene-linear;
- reconstructed full-colour RGB;
- before normal `camera_to_xyz()`;
- before appearance;
- signed/overrange capable;
- current digest identity over canonical IEEE-754 binary32 camera-native RGB sample bits;
- computed with F64 where branch-sensitive science requires it and stored as F32 only under the validated storage gate.

This is the technical descendant of **“RGB RAW weer terug”**.

A reconstructed CFA/remosaic is a downstream projection and must not replace this master.

## 5. TRUTHRAW PURE is a real historical implementation

The following historical files exist on `research/ui-output-modes-certificate-v0.1-2026-09-14` and are absent from the current TruthNegative branch:

- `docs/implementation/OUTPUT_MODES_CERTIFICATE_IMPLEMENTATION_2026-09-14.md`;
- `docs/implementation/PURE_DNG_DEVICE_VALIDATION_2026-09-14.md`;
- `docs/research/rgb-linearraw-output-restore-v0.2/README.md`;
- `docs/research/scientific-master-linear-dng-projection-v0.1/...`;
- `docs/research/truthraw-certificate-v0.1/...`;
- `docs/research/truthraw-dng-certificate-embed-v0.1/...`;
- output-mode and PURE verification workflows/tools.

The high-fidelity PURE route was:

`sealed source`
→ `source-bound color`
→ `Scientific Master v0.2`
→ `Technical Backplane phase 2`
→ `canonical 64x64 Scientific Master replay`
→ `exact master digest gate`
→ `cameraToXyzD50`
→ `float32 XYZ-D50 LinearRaw DNG`.

The writer contract explicitly prohibits appearance, display transfer, gamut mapping and tone mapping in the Scientific-Master tile source.

The writer:

- stores 3-channel IEEE-754 float32 LinearRaw;
- preserves finite negative and >1 values rather than clipping to [0,1];
- recomputes the exact camera-native Scientific Master digest during export;
- aborts the transaction if the recomputed master differs from the admitted master identity;
- commits only after the exact master-identity gate passes.

A real Honor source-bound run produced a verified 4080×3072 PURE artifact. This is historical device evidence, not merely a design note.

## 6. PURE is not the Scientific Master

Keep these objects distinct:

1. **Scientific Master**
   - camera-native scene-linear RGB;
   - scientific reconstructed state;
   - not a DNG container.

2. **TRUTHRAW PURE**
   - downstream scientific file projection;
   - Float32 XYZ-D50 LinearRaw in the historical implementation;
   - appearance-neutral;
   - source/master/backplane lineage-bound;
   - does not create evidence.

3. **RGB LinearRaw compatibility**
   - historical 16-bit finite-window compatibility projection;
   - useful for reader interoperability;
   - may not redefine PURE.

4. **Reconstructed CFA/rawsensor**
   - compatibility or sensor-negative-style projection;
   - reconstructed, never original measured CFA evidence.

## 7. Modern placement of PURE relative to Open World

The current scientific flow is:

`Source Evidence`
→ `Measurement/de-ISP`
→ `Scientific Master`
→ `Dynamic Authority + uncertainty/support`
→ `Open Scene State`
→ optional counterfactual
→ appearance/HDR/transport
→ finite exports.

The historical PURE implementation predates Dynamic Authority. The modern recovery must therefore bind PURE to the same finalized source/master/authority lineage **without changing PURE pixels**.

Target relation:

`Scientific Master + source lineage + TruthRange/scene scale + Backplane + Dynamic Authority`
→ `TRUTHRAW PURE scientific snapshot`

and independently:

`Scientific Master + source lineage + Dynamic Authority`
→ `TruthNegative Core`

and:

`Scientific Master + Dynamic Authority`
→ `Open Scene / Free Scientific Space`.

PURE, TruthNegative and Open Scene are sibling consumers/bindings of the same scientific root. None should be reconstructed from another exported artifact.

## 8. TruthNegative placement

TruthNegative v0.2 remains downstream of the already-existing reconstructed world.

Correct role:

`Scientific Master + lineage + authority -> TruthNegative Core -> optional reconstructed sensor-negative projection`.

TruthNegative must not:

- create a second Scientific Master;
- make PURE;
- force camera-native RGB back into Bayer;
- relabel dense reconstructed positions as measured;
- prove a stronger physical Camera-5 route existed.

## 9. Backplane and Dynamic Authority

The 180-byte Technical Backplane format is historically validated and version-specific. It must not be silently mutated to carry later fields.

Dynamic Authority v0.6/v1.9 is an RGB per-channel authority sidecar. It is hash/lineage-bound and does not write back into Scientific Master pixels.

Therefore modern PURE recovery should add a **versioned external/adjacent authority binding** rather than editing frozen Backplane bytes.

## 10. Certificate genealogy

Two certificate systems must not be collapsed:

### Historical export certificate v0.1

The `TRCERT01` record was used by the historical PURE DNG path and successfully validated on a real Honor-produced PURE artifact.

Preserve it for historical artifact verification.

### Current canonical Pure Truth Certificate v1.1

`canonical/ptc/v1.1` is a later fail-closed certification framework. It evaluates source admission, reconstruction identity, Scientific Master, clipping policy, physical-calibration claims, appearance separation and export payload integrity.

It currently identifies production DNG integration as an unfinished integration item. Therefore it is not a byte-compatible replacement for the historical `TRCERT01` path.

Rule:

> Preserve legacy verification. Version any new PURE/authority certificate binding. Never silently reinterpret an old artifact under a newer schema.

## 11. FotoGraaf Scene Metrology recovery

The historical FotoGraaf Scene Metrology architecture was explicitly **not a 13th room**. It was a cross-room metrology protocol spanning:

- capture-domain metrology in MeasurementLab;
- reconstruction in Architect/Restorer;
- scene/master binding in SceneRegistry;
- post-reconstruction metrology in Surveyor;
- local geometry/material/light hypotheses in Room Capsule;
- counterfactual light in LightingStudio/CICM;
- presentation/export in Finisher/Exporter.

Its authority classes included direct source measurement, calibrated physical state, inferred scene state, censored/bounded state and downstream counterfactual/appearance roles.

This concept remains consistent with current Open Scene / Dynamic Authority architecture and should be preserved as genealogy even though the original implementation branch diverged.

## 12. Physical calibration genealogy

Historical FotoGraaf Calibration Pack v0.1 defined:

- **C0** identity/source-domain characterization;
- **C1** dark/offset/read-noise/hot-pixel characterization;
- **C2** linearity/gain/saturation characterization;
- **C3** flat-field/lens/color shading;
- **C4** spectral/color calibration;
- **C5** relative radiometric calibration;
- **C6** optional absolute radiometric/photometric calibration;
- **C7** geometry/illumination validation rig.

The C0 gate sealed exact camera/lens/mode/sample-domain/build/focus/stabilization/capture identity before calibration frames could be admitted.

The Physical Promotion Gate required:

`controlled calibration bytes`
→ validated CalibrationPack
→ exact scene binding
→ held-out physical validation
→ uncertainty validation
→ shadow-conservation validation
→ `ELIGIBLE_FOR_EXPLICIT_PROMOTION_REVIEW`.

This historical implementation is absent from the current TruthNegative branch.

Current `docs/calibration/tele-full-physical-v0.1/README.md` covers overlapping later calibration obligations with a newer campaign sequence:

`S0_PREFLIGHT -> S1_GAIN_STATE_RECON -> S2_PTC_FULL -> S3_COLOR_ILLUMINANT_FIT -> S4_COLOR_ILLUMINANT_VALIDATION -> S5_OPTICS_PSF_SFR_CA_FIT -> S6_OPTICS_FLARE_SHADING_FIT -> S7_OPTICS_INDEPENDENT_VALIDATION -> S8_HIGH_ISO_UNCERTAINTY_INDEPENDENT_REVIEW -> S9_PROMOTION_REVIEW`.

This is **not declared here to be a byte-for-byte or one-to-one successor** to C0-C7. It is the current calibration campaign covering many of the same physical-proof obligations under newer architecture.

Current status remains `PURE_TRUTH_DERIVED`, not `FULL_PHYSICAL`.

## 13. Android FotoGraaf acquisition recovery

Historical Camera2 acquisition companion:

`Camera2 capture-time state -> one RAW_SENSOR frame -> finalized DNG -> post-finalization SHA-256 -> acquisition observation JSON`.

Its job was to preserve capture-time physical/logical camera identity, raw dimensions/CFA and TotalCaptureResult binding instead of inferring physical-camera identity later from focal length or filenames.

That original app/contracts are absent from the current TruthNegative branch.

The later Camera-5 line continues the same scientific requirement in a more targeted form: physical-camera route, exact RAW_SENSOR payload identity, timestamp/result binding, stride/padding/sample-domain facts and source-first sealing.

## 14. Materials / water droplets / detail

The 2026-09-14 knowledge ledger records water/droplets as a useful stress-test because transparent/specular/refractive material stresses:

- fine-detail reconstruction;
- material decomposition;
- specular glints;
- refractive/transmissive behavior;
- highlight authority.

It also explicitly records that the exact historical origin of the literal word `waterdruppels` is not documented strongly enough to claim it as a foundational quote.

Current scene-physics/restoration work preserves the scientific lesson:

- material response/BRDF is part of image formation;
- one RGB RAW does not uniquely identify arbitrary geometry/material/illumination;
- sharpening/acutance is not measured detail;
- appearance may not overpaint valid measured support in the Scientific Master;
- counterfactual relighting remains counterfactual unless independently constrained.

## 15. Open-World illumination, scientific HDR and water-droplet detail

The Open-World lighting/HDR line is part of the recovered genealogy and must stay connected to the same Scientific Master / Dynamic Authority root.

### 15.1 Captured-world scientific HDR

The current scientific HDR route is:

`sealed single RAW/CFA`
→ `Scientific Master`
→ `per-channel Dynamic Authority`
→ `scientific HDR headroom`
→ source-bound target-colour transform
→ authority-aware scene-EV to display-EV projection
→ finite HDR transport such as P3-D65 / ST-2084 PQ.

The scene representation has no architectural `[0,1]` or fixed-EV wall, but this is **representation freedom**, not infinite measured sensor dynamic range.

Per-channel HDR meaning remains:

- finite measured/calibrated value → evidence-supported finite HDR;
- finite reconstructed value with admitted support/uncertainty → reconstructed scientific HDR, never measured;
- `CENSORED` → bound only, never invented exact radiance;
- `UNKNOWN` → no scientific HDR headroom;
- `COUNTERFACTUAL` / appearance → presentation or hypothetical world only.

Adobe HDR Limit, PQ code, MaxCLL, gain maps or tone curves never create new sensor evidence or write back into the Scientific Master.

### 15.2 Open-World illumination

Open-World illumination remains a separate authority axis.

Illumination may be:

- unknown;
- source-bound estimate;
- reconstructed/inferred;
- independently measured;
- counterfactual.

A different virtual sun/lamp/night state can create a useful Open-World HDR view, but it remains `COUNTERFACTUAL` unless independently constrained by physical measurements. A simulated relight is not another exposure and cannot increase `physicalFrameCount` or `independentEvidenceCount`.

Physically strong relighting requires support for geometry/normals, visibility/occlusion, material/BRDF, spatial illumination and spectrum. A scalar brightness operation is not a physical relighting model.

### 15.3 Water droplets as a sharpness/material/HDR stress test

The historical `waterdruppels` line is explicitly preserved.

Water/droplets stress multiple coupled scientific axes:

- optics and local SFR/MTF/PSF support;
- tiny high-frequency detail;
- measured CFA structure versus reconstructed missing colour;
- focus/motion/noise uncertainty;
- reflection, transmission, refraction and transparency;
- specular glints and highlight censoring;
- material/BRDF ambiguity;
- colour-invention risk;
- HDR highlight authority.

The modern meaning of “water-droplet sharpness” is **not** an instruction to sharpen droplets harder.

TruthRaw must keep three distinct quantities:

1. optical/sample detail authority;
2. reconstructed spatial detail;
3. output acutance/sharpening.

A droplet edge/glint may be very visually sharp while still having weak optical/evidence authority. Conversely, evidence-supported microstructure should not be blurred merely to look natural.

Current Structure Evidence rules therefore apply directly to water droplets:

- measured structure comes from source CFA support before appearance;
- censored pairs do not create exact highlight structure;
- reconstructed missing-channel detail remains reconstructed and uncertainty-bound;
- without exact-domain hold-out optical MTF support, adaptive detail/acutance remains Neutral;
- even with admitted optics support, appearance-domain detail cannot write transformed pixels into the Scientific Master or become newly measured detail.

For water-specific HDR:

- an uncensored evidence-supported specular glint may contribute real scientific HDR headroom;
- a clipped droplet highlight remains `CENSORED` and provides only a lower bound;
- Open-World relighting may generate a new hypothetical glint/refraction pattern, but that new pattern is `COUNTERFACTUAL`;
- generic blue/cyan water colour or fabricated highlight peaks are forbidden when unsupported by the captured evidence.

This keeps the original water-droplet stress-test insight connected to current Structure Evidence, Dynamic Authority, Open-World illumination and scientific HDR rather than reducing it to a sharpening preset.

## 16. Camera-5 forensic correction

Current interpretation must preserve the later payload audit over earlier “200MP RAW proven” shorthand.

Observed third-party Camera-5 route:

- app-visible/envelope geometry: `16320x12288`;
- U16 envelope bytes: `401,080,320`;
- meaningful contiguous prefix: `25,067,520` bytes;
- populated equivalent U16 domain: `4080x3072`;
- remaining large-raster region observed as zero in the audited capture.

Therefore:

- maximum-resolution session/output-envelope evidence is real;
- 200MP physical-sample authority is not established by that envelope;
- missing “15/16” positions may not be called hidden measured sensels;
- a dense TruthNegative at 16320×12288 would be reconstructed representation unless a future admitted source proves otherwise.

HONOR Pro TELE DNG evidence independently recurs in the 4080×3072 CFA domain.

## 17. Current Android-17 acquisition baseline

A separate branch now exists:

`integration/truthraw-suite-v0-55-current-android17-honor-camera-baseline`.

Its purpose is read-only measurement of the user's current Android-17 / installed HONOR Camera software and Camera2 capability surface before a new active Camera-5 replay.

This baseline creates no camera frame and therefore does not yet enter the Scientific Master / PURE / Open Scene / TruthNegative chain.

## 18. Recovery rule

Do not blindly merge the 2026-09-14 branch.

Recover by contract:

1. preserve proven historical PURE implementation and verifier semantics;
2. preserve exact historical device evidence as historical evidence;
3. port only code/tests whose dependencies still match the current Scientific Master implementation;
4. bind current Dynamic Authority without changing historical PURE pixel equations;
5. preserve `TRCERT01` for legacy verification;
6. use a new versioned certificate/binding for new PURE artifacts;
7. do not mutate the frozen 180-byte Backplane;
8. keep current TruthNegative and Camera-5 work parallel;
9. preserve current F64-compute/F32-storage precision law;
10. require regression equality of Scientific Master identity before and after any recovered PURE export.

## 19. Immediate next audit

Before any code transplant:

- enumerate every PURE/RGB/Certificate/OutputMode source/test/workflow file on the historical branch;
- classify each as **PORT**, **REFERENCE-ONLY**, **SUPERSEDED**, or **BLOCKED**;
- compare its include/API dependencies against current Scientific Master v0.2, current Dynamic Authority and current Android suite;
- identify the smallest isolated host test capable of proving that the historical PURE Float32 writer can replay the current frozen Scientific Master digest unchanged.

No production or canonical promotion is authorized by this document.
