# TruthRaw — Project History and FotoGraaf Context

**Date:** 2026-09-15  
**Status:** project-history / continuity authority; does not replace module-local scientific or promotion authority  
**Purpose:** prevent the most recent Android/Camera2 execution work from being mistaken for the origin, whole scope, or final direction of TruthRaw.

---

## 1. Why this document exists

TruthRaw has developed across multiple conversations, research branches, canonical modules, failed experiments, device captures and handoffs. The most recent two development conversations became increasingly dominated by FotoGraaf/Camera2 acquisition work and both were interrupted around uploaded-file handling. Those interruption points are **conversation/execution boundaries, not architectural milestones**.

Therefore a future session must not infer that TruthRaw "became a camera app" merely because the latest recoverable work concerns physical camera ID 5, RAW_SENSOR routing, `.rawsensor` evidence or maximum-resolution capture.

The Android FotoGraaf acquisition layer is the newest lower-level consequence of an older project rule:

> If the final image is to be reconstructed from facts, the origin and authority of those facts must eventually be provable too.

This document records that larger continuity.

---

## 2. Stable project doctrine

TruthRaw remains a single-frame scientific reconstruction system built around:

> **Measured where measured. Reconstructed where necessary. Never invented.**
>
> **Echt gemeten. Echt gereconstrueerd. Geen verzinsels.**

The global separation is:

**Evidence -> Reconstruction -> Appearance / Counterfactual / Export**

Permanent implications:

- original RAW/CFA sample evidence and capture provenance remain immutable;
- one physical exposure remains one physical evidence source;
- reconstructed values never become retroactively measured values;
- clipping is censored/bounded evidence, not exact latent radiance;
- uncertainty is part of scientific state, even when the rendered estimate looks clean;
- appearance never writes back into the Scientific Master;
- virtual EV/ISO views do not create independent measurements;
- counterfactual lighting/capture does not create evidence for the historical capture;
- representation may exceed source/container range, but knowledge claims may not exceed evidence;
- resource capability may change execution cost, tile size and concurrency, but never scientific permission;
- GCam/APK/computational-RAW material may be historical or comparative input, but must not determine TruthRaw scientific authority, calibration or Direct-CFA evidence.

---

## 3. Historical development line

The project should be understood in this order.

### 3.1 GCam / MotionCam precursor

The earliest practical problem was natural mobile photography: MotionCam pure RAW, GCam noise-model construction, natural configuration tuning, soft-to-detailed profiles, Sabre/detail behavior, colour/lightness and strong attention to the tele camera.

This phase is historically important because it introduced the question that later became central to TruthRaw: processing should respond to what the data actually supports instead of applying fixed sharpening/noise/color assumptions.

It is a **precursor**, not TruthRaw scientific authority.

### 3.2 RAW as measurement; image reconstruction as an inverse problem

The project then moved beyond tuning an existing ISP. RAW was treated as measurement data with uncertainty. The goal became:

**RAW observation -> physical/statistical interpretation -> most defensible latent scene estimate -> image**

The target was not "denoise until no noise remains". A visually clean estimate can coexist with substantial uncertainty.

### 3.3 Single-frame became a hard boundary

Multi-frame methods could be useful elsewhere, but TruthRaw's scientific master was deliberately built around one physical exposure. Missing information may be reconstructed only under explicit evidence/uncertainty rules; it must never be presented as a second measurement.

This later formalized the distinction between **MEASURED**, **RECONSTRUCTED**, **CENSORED/UNKNOWN/UNDETERMINED**, **COUNTERFACTUAL**, and **APPEARANCE** quantities.

### 3.4 Cross-device testing: Honor and Pixel have different roles

A real high-resolution Google Pixel computational DNG became an important cross-device test. It demonstrated that TruthRaw must not silently assume an Honor/tele/Direct-CFA source model. Capability, provenance, sample representation and prior processing must be discovered and admitted explicitly.

The Pixel path is therefore valuable for **generic ingest/provenance/computational-source handling**.

Honor Direct-CFA paths are valuable for **stronger direct-measurement lineage and physical calibration work**.

These are different authorities. Neither device is allowed to define the universal architecture by itself.

### 3.5 Float scene / Latent Camera Scene / Scientific Master

Once reconstruction became separate from the source container, the reconstructed world no longer needed to inherit RAW10 integer range, source WhiteLevel as output ceiling, source ISO as working scale, SDR range or DNG storage constraints.

The Scientific Master can use wider floating-point coordinates and carry uncertainty/support/censor/model-influence state.

This is representational freedom, not new photons.

### 3.6 Uncertainty and Soft Truthful Reconstruction

Early attempts showed that a reconstruction could look sharper while becoming less truthful through zippering, unstable chroma or unsupported high-frequency detail.

This produced a lasting rule:

**More visible detail is not automatically more truth.**

Scientific reconstruction, appearance detail and final output acutance therefore became separate responsibilities.

### 3.7 Multi-Light -> Virtual Observation Manifold -> Best Conditioning

The same RAW can be numerically inspected under multiple virtual exposure/gain views. These views can reveal where existing evidence becomes easier to inspect, but they are not new captures.

The mature interpretation is conditioning/reparameterization: if an algorithm produces substantively different scientific content merely because the same state was expressed at another virtual EV, that algorithm is a new candidate requiring its own validation.

### 3.8 Sealed old house and new house

The **sealed old house** is the immutable source evidence and capture provenance.

The **new house** is the separate reconstructed scientific scene.

The new house may have a richer numeric representation. It may never rewrite the old house or claim more evidential certainty than the old house supports.

### 3.9 Middle line -> TruthRange / zero-line

The old "middle line" idea matured into an explicit scene coordinate/gauge. For positive physical scene light TruthRaw may use:

`T = log2(L/L0)`

The zero-line is not DNG BlackLevel, zero photons, clipping, display black or an arbitrary claim of physical darkness. It is a reference binding for the reconstructed scene representation.

### 3.10 Back of the photo -> Technical Backplane

The old idea that a scientific photograph needs a verifiable "back side" matured into the Technical Backplane: compact format-neutral lineage and authority binding for source, Scientific Master, zero-line, scene scale and room/module status.

It contains no extra photographic evidence and does not replace calibration.

### 3.11 In-between house -> Professional RAW Gatehouse

Unknown/professional/vendor RAW formats are admitted through an isolation/gatehouse layer. The Gatehouse may recognize, validate, decode and seal a handoff; it may never create a second photographic truth.

### 3.12 Cheap vs expensive phone -> independent resource axis

The Building Runtime and Rooms formalized the distinction between scientific authority and execution resources.

A slower phone may use smaller tiles, fewer concurrent rooms and more disposable cache. A faster phone may use larger workspaces or more compatible concurrency. Both must reach the same scientific authority for the same admitted evidence and algorithm version.

### 3.13 Water droplets -> Material Truth

Water, glints, thin structure, transparency, fine edges and dark/highlight transitions were used as difficult material stress tests. The mature system must not simply recognize an object class and apply a "water look". It responds to evidence-supported local properties such as texture support, edge risk, glint/censor risk and low-SNR behavior.

### 3.14 Fotograafkamer -> CICM / Room Capsule / appearance floors

The older Fotograafkamer concept asked how a photographer would inspect or alter light locally. This matured into explicitly separated counterfactual and appearance layers such as CICM, Room Capsule, Lighting Studio, Colorist and Finisher.

A simulated light world can answer a hypothetical question. It cannot retroactively become evidence about the historical exposure.

### 3.15 RGB RAW again -> controlled export projections

The Scientific Master is reconstructed full-colour scene state. Export may project it into formats useful to conventional software.

Important distinctions remain:

- exact/original CFA repack: original measured sample evidence;
- reconstructed CFA projection: derived/reconstructed, not original measured Bayer;
- Linear DNG / compatibility projection: full-colour derived representation;
- Scientific Master: scientific state, not merely an interchange DNG.

No export may silently relabel reconstructed values as original sensor measurements.

### 3.16 FULL_PHYSICAL calibration campaign

Before the latest Camera2-heavy conversations, the project already had a broader physical calibration trajectory: readout/gain-state reconnaissance, PTC/noise/electron characterization, colour/illuminant calibration, optics, high-ISO review, independent validation and promotion gates.

This establishes the reason the project later needed its own capture/metrology instrument.

### 3.17 Broad FotoGraaf

The broad FotoGraaf concept is not synonymous with Camera2. It is the bridge between photography and metrology: controlled acquisition, source identity, physical calibration, lighting/material understanding and the evidence needed by TruthRaw.

### 3.18 Android FotoGraaf / C0 and acquisition metrology

The Android FotoGraaf is the newest acquisition layer **before** TruthRaw reconstruction. Its job is to prove which physical camera/lens/capture mode/sample domain produced an observation and to preserve the evidence needed for later calibration and reconstruction.

C0/Capture Identity therefore binds source identity to the physical route instead of trusting convenient metadata alone.

### 3.19 Physical Camera2 ID 5 / `.rawsensor` / maximum-resolution research

Recent work proved a normal physical-tele route strongly enough to preserve direct Camera2 sample-domain bytes alongside a DNG convenience container. The `.rawsensor` payload is primary byte evidence for the app-visible RAW_SENSOR buffer; DNGCreator output is a useful container, not a stronger authority than the source buffer.

Maximum-resolution capability/session support does **not** by itself prove that a 16320x12288 Direct-CFA frame was actually delivered. Actual delivered buffer, physical result binding, pixel mode, binning factor, CFA, black/white, noise profile, stride/size and timestamp identity must be observed.

That is a local acquisition research question, not the definition of TruthRaw as a project.

---

## 4. Three names that must not be conflated

### Fotograafkamer

Historical/conceptual room for local photographic/light investigation. Its mature descendants live in counterfactual and appearance floors such as CICM/Room Capsule/Lighting Studio/Colorist/Finisher.

### Broad FotoGraaf

The larger photography + metrology + calibration concept connecting controlled capture to physical calibration and ultimately to TruthRaw.

### Android FotoGraaf

The concrete Camera2 acquisition/metrology application. It must prove source identity and collect admissible physical evidence. It is a subsystem of the larger trajectory, not the entire project.

---

## 5. How to interpret the two recent camera-heavy conversations

The last two conversations became increasingly Camera2-focused and were interrupted around uploaded-file handling.

For continuity purposes:

1. their interruption points are **not** promoted architectural decisions;
2. the fact that the final visible work concerns Camera2 does **not** mean Camera2 replaced the broader TruthRaw/FotoGraaf agenda;
3. the first of those conversations was still rooted in broad FotoGraaf/TruthRaw optimization and moved into physical source identity because calibration authority required it;
4. the following conversation inherited that open acquisition problem, so Camera2 naturally dominated its later steps;
5. local acquisition work may resume from the strongest recovered evidence, but project-wide work must reconnect acquisition to calibration packs and the Scientific Master.

This is a conversation-continuity note. Module status, admission and scientific promotion remain governed by repository evidence and module-local validation documents.

---

## 6. Current trajectory

The intended project-wide trajectory is:

**controlled acquisition**  
-> **capture identity / physical-route proof**  
-> **raw byte + metadata evidence**  
-> **gain/readout characterization**  
-> **noise/linearity/saturation/PTC evidence**  
-> **flat/shading and optics evidence**  
-> **colour/illuminant evidence**  
-> **held-out physical validation**  
-> **versioned calibration packs with explicit domains**  
-> **TruthRaw admission of only supported calibration authority**  
-> **uncertainty-aware Scientific Master**  
-> **appearance/counterfactual/export layers**.

A calibration pack must never become universal merely because it matches a convenient metadata field such as focal length. Physical camera identity, mode, sample domain and calibration domain must be compatible.

---

## 7. Local acquisition boundary at this historical point

The recovered late-stage acquisition state establishes a strong standard physical tele capture path and a primary Camera2 `.rawsensor` evidence path.

The maximum-resolution/high-resolution question remains epistemically separate:

**session/capability support != delivered Direct-CFA frame proof**.

If that subtrack is resumed, the decisive experiment is an actual physical-ID5 maximum-resolution RAW_SENSOR delivery with complete result/buffer binding. Do not pre-label it "native 200 MP", "binned 50 MP", or "remosaic" until the delivered sample domain is measured.

---

## 8. Reading rule for future sessions

When answering "where were we?", do not answer only with the last Camera2 experiment.

Use two coordinates:

- **project-wide coordinate:** acquisition/metrology is being connected back into physical calibration and the larger TruthRaw reconstruction architecture;
- **local execution coordinate:** the most recent acquisition experiment may concern ID5 RAW_SENSOR routing, `.rawsensor`, or maximum-resolution delivery.

Likewise, never infer current global authority from a dated historical README merely because it appears in a recent branch. Read module-local status/promotion evidence and the current status/claim documentation.

---

## 9. Compact lineage

**GCam/MotionCam natural photography**  
-> **noise/confidence thinking**  
-> **single-frame inverse reconstruction**  
-> **Pixel cross-device/computational-source test**  
-> **device-independent admission/provenance**  
-> **float Latent/Scientific Master**  
-> **uncertainty + Soft Truthful**  
-> **sealed old house / new house**  
-> **TruthRange / zero-line**  
-> **Multi-Light -> VOM / Best Conditioning**  
-> **Material Truth**  
-> **Fotograafkamer -> CICM / Room Capsule**  
-> **Technical Backplane**  
-> **Professional RAW Gatehouse**  
-> **resource-adaptive Rooms**  
-> **controlled DNG/RAW projections**  
-> **on-device scientific preview/export work**  
-> **FULL_PHYSICAL calibration campaign**  
-> **broad FotoGraaf**  
-> **C0 / test-app**  
-> **physical Camera2 acquisition**  
-> **ID5 / `.rawsensor` / maximum-resolution evidence**  
-> **calibration packs and further TruthRaw reconstruction validation**.

---

## 10. One-sentence historical definition

**TruthRaw began as the attempt to make natural mobile photography depend on measured RAW facts rather than opaque processing, evolved into a device-independent single-frame uncertainty-aware scientific reconstruction architecture, and only later grew FotoGraaf as the acquisition/metrology layer needed to prove and calibrate the physical origin of those facts.**
