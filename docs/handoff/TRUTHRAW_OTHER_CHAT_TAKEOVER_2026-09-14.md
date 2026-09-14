# TruthRaw complete other-chat takeover — 2026-09-14

Status: **MANDATORY TAKEOVER HANDOFF / READ BEFORE CONTINUING**

Purpose: this file is the operational handoff for another chat. It does not replace the deeper knowledge pack; it tells the next chat exactly what exists, why it exists, what is proven, what is only hypothesized, what was rejected, and what the next work should be.

## 0. Mandatory read order

Before changing code or architecture, read in this order:

1. `START_HERE_NEW_CHAT.md`
2. `TRUTHRAW_KNOWLEDGE_PRESERVATION_2026-09-14.md`
3. `docs/handoff/TRUTHRAW_OTHER_CHAT_TAKEOVER_2026-09-14.md` (this file)
4. `docs/handoff/TRUTHRAW_NEXT_CHAT_HANDOFF_2026-09-14.md`
5. `docs/handoff/TRUTHRAW_KNOWLEDGE_PRESERVATION_HANDOFF_2026-09-14.md`
6. `docs/handoff/TRUTHRAW_FOTOGRAAF_PHYSICAL_ACQUISITION_HANDOFF_2026-09-14.md`
7. `docs/knowledge/TRUTHRAW_KNOWLEDGE_BOOTSTRAP_2026-09-14.md`
8. `docs/knowledge/TRUTHRAW_COMPLETE_PROJECT_KNOWLEDGE_2026-09-14.md`
9. `docs/knowledge/TRUTHRAW_GENEALOGY_DECISION_LEDGER_2026-09-14.md`
10. `docs/knowledge/TRUTHRAW_ACTIVE_RESEARCH_AND_VALIDATION_STATE_2026-09-14.md`
11. `docs/knowledge/TRUTHRAW_TOPIC_COVERAGE_CHECKLIST_2026-09-14.md`
12. `docs/knowledge/TRUTHRAW_KNOWLEDGE_INDEX_2026-09-14.json`
13. module-local research documents and tests relevant to the next task.

Do not rebuild the project story from chat memory if these files are available.

## 1. Repository / PR state at takeover creation

Repository:

`1DiDaDo1/TruthRaw`

Active branch:

`research/ui-output-modes-certificate-v0.1-2026-09-14`

Draft PR:

`#23 — Integrate four-mode output policy + TRUTHRAW PURE float32 DNG`

Base:

`research/project-factcheck-optimization-v0.1-2026-09-13`

Immediately before this handoff file was committed, branch head was:

`3b5e3bec9c01c5e0e3a548bea8eccb018373d546`

PR state at that moment:

- OPEN
- DRAFT
- MERGEABLE
- NOT MERGED

This handoff commit itself moves the branch head. The next chat must re-fetch PR head and exact-head workflow state before making any current CI claim.

**Do not merge PR #23 without explicit user direction.**

## 2. Non-negotiable project laws

Keep these exact laws:

**Measured where measured. Reconstructed where necessary. Never invented.**

Dutch working form:

**Echt gemeten. Echt gereconstrueerd. Geen verzinsels.**

Second law:

**Representation can exceed the source. Knowledge claims cannot exceed the evidence.**

Consequences:

- original Direct-CFA/source RAW remains immutable sealed evidence;
- normal scene evidence remains `physicalFrameCount=1` and `independentEvidenceCount=1`;
- calibration captures may be numerous but do not become additional scene evidence for a later photograph;
- a float32/master representation may be richer than RAW10/RAW12/RAW14 numeric coding without claiming extra photons;
- saturated source values remain censored/bounded rather than exact unknown-above-threshold values;
- counterfactual lighting/world simulation may never become captured-world evidence;
- appearance may not rewrite the Scientific Master;
- resource/worker count may not change scientific truth;
- GCam/APK/computational-RAW content may not determine TruthRaw evidence, calibration, topology, color, noise or architecture.

## 3. Current scientific master / source separation

Current project-level Scientific Master:

- full-resolution reconstructed scene state;
- camera-native RGB;
- before normal `camera_to_xyz()`;
- before appearance;
- signed float32 or richer representation;
- accompanied conceptually by uncertainty/support/censor/model/provenance state;
- not original CFA evidence;
- not a display image.

Direct-CFA source evidence stays separate and immutable.

Reconstructed CFA DNG is only a compatibility projection. The preferred full-color scientific projection is the float32 LinearRaw route.

## 4. Nul-lijn / TruthRange rule

For positive scene light:

`T = log2(L/L0)`

`L0` is a gauge/reference, not sensor black, DNG BlackLevel, clipping, display black or middle grey.

The intended new-house address space is two-sided:

`darkness <- ... <- -EV <- nul-lijn -> +EV -> ... -> brighter light`

The house may be representationally unbounded upward and downward. The sensor evidence remains finite, noisy, quantized and censored.

Negative post-black numerical values are not negative physical light. They must remain available in signed scientific state and must not be clipped merely to make the log coordinate convenient.

## 5. House / Backplane / Gatehouse genealogy

Keep the historical architecture meanings:

- **sealed house / gezegelde woning** = immutable source RAW/CFA evidence;
- **new house / alle vrijheid** = richer reconstructed representation without evidence inflation;
- **Technical Backplane / achterkant van de foto** = compact technical provenance/identity backside, not hidden extra image evidence;
- **Gatehouse / tussenwoning** = isolated external decode/audit boundary, detached after sealed handoff, never a second source of truth.

Frozen Backplane/certificate layouts must not be silently expanded. New persistent fields require versioned schemas.

## 6. Reconstruction authority

### v4.7i

Current scientific reconstruction baseline:

- measured-preserving;
- measured CFA component preserved/reinjected exactly where required;
- only missing color components reconstructed;
- research backend directional/edge-aware/support-limited;
- no creative appearance authority.

### v4.7j / v4.7k

Appearance/detail/output-acutance only.

### older v3 variants

Restricted/withdrawn after exact-kernel/backend mismatch concerns. Preserve history; do not promote as current science.

## 7. Multi-worker / “multiple photographers” execution model

Important recovered fact: the older full v4.7i processor already had true multithreaded tile workers and parallel downstream HDR gain work.

Architecture rule:

**room = scientific responsibility; photographer = execution worker.**

Multiple photographers may work in one room on independent tiles. Multiple downstream rooms may run concurrently after the required barriers/seals. More cores may improve wall time, but must not change any authoritative result.

Current validated direction:

- one serialized RAW producer when source thread safety requires it;
- N workers with private scratch;
- deterministic reduction barrier;
- one global exposure/scene decision;
- parallel reconstruction/appearance/HDR where safe;
- bounded reorder buffer;
- ordered writer/commit;
- worker count excluded from scientific/calibration identity.

`Full Frame Streaming v0.2 Multi-Worker` and `Worker Resource Invariance v0.1` have been green on the active implementation line before later takeover documentation commits.

## 8. Product / APK architecture

Public modes:

1. `JPG`
2. `JPG XL`
3. `TRUTHRAW PURE`
4. `TRUTHRAW ADVANCED`

Appearance controls:

- `Colourful`
- `Detailed`
- `Soft`
- `HDR`

PURE remains appearance-neutral. Unknown/missing mode fails safe to PURE. JPEG XL remains fail-closed until a real encoder is validated.

Languages represented:

- English fallback
- Dutch
- German
- French

Current prototype identity:

`0.7-four-mode-certificate`

## 9. TRUTHRAW PURE float32 DNG

Preferred export chain:

`sealed source -> source-bound color -> Scientific Master v0.2 -> Technical Backplane phase 2 -> canonical replay/digest gate -> cameraToXyzD50 -> float32 XYZ-D50 LinearRaw DNG`

Key properties:

- IEEE float32;
- 3 samples/pixel;
- LinearRaw photometric role;
- negative values preserved;
- values above 1 preserved;
- no creative tone/appearance/gamut mapping;
- exact master digest gate;
- one physical frame / one evidence root.

A supplemental exact-source Honor run is already PASS for artifact/projection integrity, but the formal five-source device gate remains separate/open.

## 10. Certificate v0.1

Certificate is embedded in the technical/file backside, never as a visible watermark.

Current state:

`UNSIGNED DEVELOPMENT`

No private TruthRaw signing key may be embedded in repo/APK. A verified badge is forbidden until trusted issuer/signature verification exists.

Certificate v0.1 does not serialize every internal exporter boolean; artifact verification must not overclaim fields not present in file bytes.

## 11. FotoGraaf scientific role

FotoGraaf evolved into a local scientific investigator/metrology system, not a free-form world generator.

Important authority classes used conceptually:

- `MEASURED_SOURCE`
- `CALIBRATED_PHYSICAL`
- `INFERRED_SCENE`
- `BOUNDED_CENSORED`
- `UNKNOWN`
- `COUNTERFACTUAL`
- `APPEARANCE`

FotoGraaf may analyze capture state, dynamic-range support, geometry/light hypotheses and local scene structure. It may not turn an inference into a measurement claim without independent calibration.

Radiance toward camera and incident irradiance/illuminance are different quantities. One RGB RAW does not uniquely determine illumination × BRDF/material × geometry.

RoomCapsule/CICM relighting remains counterfactual unless independently constrained.

## 12. CalibrationPack architecture

Current intended chain:

`controlled calibration captures`
`-> Dataset Manifest`
`-> validated CalibrationPack`
`-> exact Scene Admission`
`-> CalibrationBindingPacket`
`-> exact model/protocol bytes`
`-> MeasurementLab shadow`
`-> held-out physical validation`
`-> uncertainty validation`
`-> Physical Promotion Gate`
`-> explicit production-promotion review`

Important rule: Physical Promotion Gate does not itself change production pixels or master authority. It only makes a model eligible for explicit review.

Current calibration acquisition classes include:

- C0 exact capture identity;
- C1 dark/noise;
- C2 linearity/gain/saturation;
- C3 flats/shading;
- C4 color;
- C5 relative radiometry;
- C6 absolute radiometry;
- C7 incident-light/geometry validation.

C6/C7 are required for strong absolute-radiance/irradiance claims. Scene dynamic range can become calibrated via appropriate C1/C2 evidence; product HDR is not a calibration quantity.

## 13. Honor tele ISO / dark empirical result that must be preserved

The exact-ISO8192-associated sample-domain observation is important:

- exact ISO8192 repeatedly showed roughly 54–55 DN dark sigma and ~7.1% raw-zero censoring;
- nearby ISO8184 and ISO10244 at matched exposure remained ordinary;
- therefore simple monotonic `ISO >= 8192` threshold is **REJECTED**;
- reproducible discrete ISO8192-associated sample domain is **PASS AS OBSERVATION**;
- physical cause remains **OPEN**;
- do not label it DCG, analog-gain switch, sensor defect or decoder bug without causal evidence.

Calibration must bind exact capture/sample-domain identity, not ISO magnitude alone.

## 14. Capture Evidence Seal / C0 rule

The existing import UI uses `ACTION_OPEN_DOCUMENT`. It can hash/inspect a finalized DNG but cannot retroactively invent acquisition-time physical camera identity.

Therefore FotoGraaf has a source-side Camera2 acquisition companion.

Two-stage authority:

1. record acquisition-time Camera2 evidence while source-stack state still exists;
2. after DNG finalization, hash the actual file bytes and bind them to the acquisition observation/C0 record.

Rejected authority shortcuts:

- `DNG_METADATA_ONLY` for physical camera identity;
- `EXIF_ONLY`;
- `ISO_ONLY`;
- `ISO_DERIVED`;
- filename guessing;
- user guess.

## 15. Real Honor Magic8 Pro / BKQ-N49 camera observations

Three real Camera2 observations already showed that generic camera selection is not sufficient.

Observed logical camera `0`:

- logical multi-camera = true;
- advertised physical IDs = `2`, `4`, `5`;
- original generic route requested no physical ID;
- Camera2 selected active physical ID `2`;
- RAW was `4096x3072`, BGGR;
- dynamic WhiteLevel was `1023`.

Two other observations used direct Camera2 ID `1`, produced `4096x3072` GRBG RAW and were not the desired tele route.

Conclusion:

**generic automatic Honor lens choice is unacceptable for physical calibration.**

Historical project role hints remain only hints until capture-time result evidence confirms them:

- physical ID `2` = main candidate;
- physical ID `4` = ultrawide candidate;
- physical ID `5` = tele candidate.

## 16. Forced HONOR tele route v0.2

`HonorTeleActivity.kt` exists.

Its intended fail-closed sequence:

`logical multi-camera`
`-> physical OutputConfiguration ID 5`
`-> physical-aware capture request for ID 5`
`-> physical TotalCaptureResult for ID 5`
`-> exact RAW/result timestamp match`
`-> DNG built with physical camera characteristics/result`
`-> finalized source hash`

If Honor silently falls back to another sensor or no physical result for ID5 exists, the route must FAIL rather than save a file called tele.

The forced-ID5 route is implemented and builds. Real-device proof that it produces the desired tele RAW is still **OPEN** until a new BKQ-N49 capture confirms it.

## 17. Physical-camera capability probe v0.1

`HonorCapabilityProbeActivity.kt` exists and is registered in the combined suite manifest and launcher.

Its button in the suite is:

`FotoGraaf · scan main / wide / tele / max-res / macro / RAW14`

It is read-only and makes no capture.

It inventories logical/physical Camera2 capabilities including:

- standard `RAW_SENSOR` output sizes;
- maximum-resolution `RAW_SENSOR` output sizes;
- regular/max-resolution pixel and active arrays;
- CFA;
- RAW capability;
- MANUAL_SENSOR capability;
- ultra-high-resolution capability flags;
- sensitivity/exposure ranges;
- focal lengths;
- minimum focus distance;
- focus-distance calibration;
- AF modes including macro when advertised;
- OIS modes;
- runtime RAW14 advertisement when the platform exposes that format.

Output authority class:

`CAMERA2_CAPABILITY_OBSERVATION_ONLY`

A capability is not capture proof and grants no calibration authority.

## 18. Main / wide / tele future engine rule

Do not build three unrelated camera stacks. The next architecture should use one physical-camera engine/profile with separate measured capability records per physical route.

Conceptual profile:

`physical route`
`-> standard RAW capabilities`
`-> maximum-resolution RAW capabilities`
`-> focus/macro capabilities`
`-> stabilization`
`-> bit-depth/format capabilities`
`-> validated capture modes`

The same scientific admission rules apply to main, ultrawide and tele.

Never make lens role authoritative solely from the historical ID map. The role becomes accepted only after the capability/capture evidence is consistent and physical result identity is observed.

## 19. Native 200 MP tele rule

The tele sensor is marketed as 200 MP, but a marketed sensor or stock-camera 200 MP JPEG is not proof of public Direct-CFA 200 MP RAW.

TruthRaw requires Camera2 to advertise a maximum-resolution RAW route for the physical tele path.

A maximum-resolution RAW at or above ~180 million pixels may be tagged as a **200 MP-class candidate**, not proof.

Capture proof requires:

`MAXIMUM_RESOLUTION raw stream`
`-> matching sensor pixel mode`
`-> physical output session`
`-> requested physical result`
`-> timestamp identity`
`-> sealed source SHA-256`
`-> C0`

If Camera2 exposes only a lower-resolution RAW while Honor produces a 200 MP processed image, TruthRaw must not invent a 200 MP Direct-CFA source.

The same maximum-resolution rule applies to main and ultrawide.

Historical research had reported a possible tele maximum-resolution lattice around `16320x12288` (~200.5 MP) and normal high-resolution around `8160x6144` (~50.1 MP), but this remains a candidate/historical device observation until the new capability probe reports the actual current Camera2 characteristics on the user's firmware.

## 20. Macro / close-focus rule

HONOR marketing/specification context indicated an ultrawide macro feature. TruthRaw must still prove what Camera2 exposes and which physical sensor actually produces the RAW.

Capability probe records:

- `LENS_INFO_MINIMUM_FOCUS_DISTANCE`;
- focus calibration;
- AF modes;
- whether `CONTROL_AF_MODE_MACRO` is advertised.

Future **Wide Macro RAW** route must physically bind to the wide sensor and captured RAW.

Tele should be called **tele close focus** until measured focus/reproduction evidence supports the stronger term `macro`.

Do not infer macro from a crop or marketing label.

## 21. Android 17 / RAW14 rule

Platform research established that Android 17 introduces a RAW14 image-format capability. Treat this as an independent capability axis.

Important:

**Android 17 platform support does not mean Honor or every physical camera exposes RAW14.**

Current branch compileSdk is 35 / Android-16-era. The capability probe therefore uses runtime reflection for `ImageFormat.RAW14` so the current app remains buildable while a future Android-17+ runtime can reveal the format.

Required RAW14 proof sequence:

1. runtime exposes RAW14;
2. physical camera advertises RAW14 size(s);
3. requested physical session/capture succeeds;
4. packed 14-bit byte/sample semantics are independently validated;
5. original packed source bytes remain sealed;
6. RAW14 decoder/unpacker is validated before MeasurementLab consumes it.

RAW14 and maximum-resolution/200 MP are independent. Never combine them unless the exact route proves both.

Do not force RAW14 through an old RAW_SENSOR/DngCreator assumption merely because the API constant exists.

## 22. Current combined TruthRaw + FotoGraaf APK

The combined project is:

`app/android/truthraw-suite-v01/`

It packages:

- normal TruthRaw UI/reconstruction/export path;
- FotoGraaf generic Camera2 acquisition;
- FotoGraaf forced HONOR tele physical-ID5 route;
- FotoGraaf physical-camera capability probe.

Because Android permissions are application-wide, the combined suite APK declares CAMERA permission. Scientific authority remains separated by contract/activity rules, not by package permission.

A dated suite document now exists:

`app/android/truthraw-suite-v01/TRUTHRAW_SUITE_V0_1_2026-09-14.md`

The previous unclassified `README.md` was removed because Documentation Governance correctly rejected an unclassified README-like path.

## 23. Last exact built combined APK anchor

On exact head:

`fc80cbcea951623429aa686effc9abbeecc2407e`

`TruthRaw Suite APK Build v0.1` completed successfully.

Artifact:

- name: `truthraw-main-plus-fotograaf-v0.1-debug-apk`
- artifact id: `10358082184`
- artifact size: `1,513,822` bytes (ZIP artifact payload metadata)
- artifact digest: `sha256:9990ba97f71053485514f3208e9204a7e1f8fa72a5abb15382d59c403ae8566a`
- workflow run: `34868870206`

The standalone FotoGraaf companion also built successfully on the same head:

- name: `truthraw-fotograaf-camera2-capture-v0.1-debug-apk`
- artifact id: `10358472755`
- artifact size: `799,607` bytes
- artifact digest: `sha256:57c8866b67c8fe0b5bf9342f7ca5b7a9039bd08c382b038060293dd3e68b4c28`
- workflow run: `34868870183`

Later documentation/governance-fix commits moved the branch head. Rebuild/re-fetch before claiming the newest head has the same artifact digest.

## 24. Exact-head CI status before governance fix / takeover commits

At exact head `fc80cb...` most scientific/product workflows were green, including:

- Output Mode Policy
- PURE DNG Artifact Verifier
- TruthRange v0.4/v0.5
- FotoGraaf Calibration Scene Admission
- Physical Promotion Gate
- Calibration Pack
- C0 Capture Identity Gate
- Capture Evidence Seal
- Calibration Intake
- Calibration Model Shadow
- Full Frame Streaming v0.2 Multi-Worker
- Worker Resource Invariance
- Canonical Integrity
- Camera RGB Covariance
- XYZ D50 Uncertainty
- Scientific Master Linear DNG Projection
- Certificate / DNG Certificate Embed
- Android source-bound/finalized builds
- TruthRaw Suite APK Build
- FotoGraaf Camera2 Acquisition Companion

At that exact point `Documentation Governance Current` failed only because:

`unclassified_readme_like_path:app/android/truthraw-suite-v01/README.md`

That specific governance issue has now been structurally fixed by creating the dated suite document and deleting the unclassified README.

At the earlier exact check, `Reconstructed Color Preview` and `Android Honor Empirical Harness` were still in progress. Later commits moved the head, so the next chat must fetch exact latest CI from scratch.

## 25. What is PASS / OPEN / BLOCKED / REJECTED now

### PASS / implemented direction

- sealed source vs Scientific Master separation;
- v4.7i measured-preserving reconstruction baseline;
- float32 PURE LinearRaw projection;
- certificate development embedding;
- four output modes;
- knowledge preservation pack;
- multi-worker host architecture and resource invariance gate;
- FotoGraaf CalibrationPack contracts;
- C0 scene/capture identity gate;
- C1/C2 intake partitioning by exact scope/sample domain;
- Capture Evidence Seal;
- scene admission;
- shadow MeasurementLab path;
- Physical Promotion Gate;
- generic Camera2 acquisition companion;
- forced physical-ID5 tele route implementation;
- physical-camera capability probe implementation;
- combined TruthRaw + FotoGraaf APK build on `fc80cb...`.

### OPEN

- real BKQ-N49 forced physical-ID5 capture proof;
- actual capability-probe JSON for current user firmware;
- current exact maximum-resolution RAW sizes for physical IDs 2/4/5;
- public native 200 MP tele RAW proof;
- main/wide maximum-resolution RAW proof;
- wide physical macro RAW proof;
- tele close-focus characterization;
- actual RAW14 advertisement on Android 17 Honor firmware;
- physical cause of exact-ISO8192-associated domain;
- independent NoiseProfile semantics;
- actual controlled CalibrationPack datasets;
- formal five-source PURE device artifact gate;
- Adobe/Lightroom interoperability;
- final appearance renderer semantics.

### BLOCKED

- production calibration-assisted Scientific Master until controlled physical datasets pass all gates;
- full optical inverse/deconvolution until independent optics calibration;
- full independent physical color/spectral authority until calibration;
- absolute incident irradiance/radiance without C6/C7 traceability;
- trusted certificate verification without issuer infrastructure;
- JPEG XL production output without validated encoder;
- RAW14 scientific processing until actual route + packed-byte semantics + decoder are validated.

### REJECTED

- constructed/re-Bayer RAW as original source truth;
- virtual EV as extra evidence;
- display black as zero-line;
- generic object-camera inverse-square brightness prior;
- ISO magnitude alone as calibration/sample-domain selector;
- simple `ISO >= 8192` domain rule;
- automatic Honor logical-camera lens selection for physical calibration;
- DNG/EXIF/filename/user guess as substitute for capture-time physical camera identity;
- GCam/APK/computational RAW as TruthRaw scientific authority;
- silent fallback to main/digital crop while labeling result tele;
- calling a marketed 200 MP image native Direct-CFA RAW without Camera2/capture proof.

## 26. Exact next steps for another chat

A new chat should continue in this order:

1. Re-fetch PR #23 current head, PR state and exact-head workflows.
2. Confirm Documentation Governance is green after replacement of the suite README with the dated document; if not, inspect the exact current failure rather than weakening governance.
3. Confirm newest `TruthRaw Suite APK Build v0.1` and `FotoGraaf Camera2 Acquisition Companion v0.1` succeed on the newest head.
4. If needed, download the newest combined APK artifact for the user.
5. On the Honor Magic8 Pro/BKQ-N49, run **FotoGraaf capability probe** first and obtain its JSON.
6. From that JSON, inspect logical camera 0 and physical IDs 2/4/5: standard RAW sizes, maximum-resolution RAW sizes, CFA, UHR capability, focal lengths, min focus, AF macro modes, OIS and runtime RAW14 support.
7. Do not build/label 200 MP, macro or RAW14 routes until the JSON says the exact route is advertised.
8. For each advertised candidate, build a fail-closed physical capture session probe using explicit physical output and matching physical result.
9. Require timestamp identity and finalized source SHA before C0 admission.
10. Only C0-passing physical routes enter C1/C2/C3/C4/C5/... calibration acquisition.
11. Keep Scientific Master/reconstruction unchanged while acquisition/calibration evidence is still being established.

## 27. Expected future camera architecture

The likely correct vNext capture architecture is one shared engine:

`HonorPhysicalCameraProfile`
`+ CapabilityReport`
`+ PhysicalCaptureRoute`
`+ CaptureEvidenceSeal`
`+ C0 identity`

with route instances for:

- main standard RAW;
- main maximum-resolution RAW if advertised;
- ultrawide standard RAW;
- ultrawide maximum-resolution RAW if advertised;
- ultrawide macro/close-focus RAW if physically proven;
- tele standard RAW;
- tele maximum-resolution / 200 MP-class RAW if advertised and proven;
- tele close-focus if physically supported;
- RAW14 variants on Android 17+ only where the exact physical route advertises and validates RAW14.

This is preferable to multiple unrelated hardcoded camera implementations.

## 28. User continuation behavior

The user commonly continues with short commands such as:

`Begin`
`Ga verder`
`Ga door`

Interpret these as permission to continue autonomously to the next scientifically justified project step, without asking for unnecessary confirmation.

For historical trigger terms such as `kleur echtheid`, `licht`, `pure RAW`, `constructed RAW`, `Multi-Light`, `achterkant foto`, `Foto graaf`, `verzegelde woning`, `waterdruppels`, `midden lijn`, `float32`, `goedkope en dure telefoon`, `RGB RAW weer terug`, `tussenwoning`, recover the documented genealogy rather than treating the term as isolated new vocabulary.

Required genealogy pattern:

`motivation -> hypothesis -> experiment/source -> finding -> limitation/failure -> fact-check -> correction -> later integration -> current meaning`

## 29. Final takeover rule

Do not restart the project from a generic RAW pipeline. TruthRaw is already far beyond that stage. The next chat should treat the repository documents, validation gates, real Honor observations, FotoGraaf acquisition architecture and current APK paths as the existing baseline.

The immediate practical target is not to alter reconstruction. It is to **measure exactly what the Honor Magic8 Pro exposes for physical main/wide/tele, maximum-resolution RAW, macro/close-focus and later RAW14, then only admit physical routes that Camera2 and sealed capture evidence actually prove.**
