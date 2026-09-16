# TruthRaw project history and changes — reconstructed through 2026-09-16

Status: **CURRENT HISTORICAL CONSOLIDATION**

Purpose: preserve how TruthRaw actually evolved, including wrong turns, provisional results, historical metaphors and later corrections. This document is not a claim that every idea below was canonical at the moment it first appeared.

Evidence language used here:

- **repository-verified** — present in repository files/commits/tests;
- **device-report verified** — present in the real exported Android v0.3 report;
- **historical-chat recovered** — recovered from preserved conversation screenshots/handoffs and retained because it explains why later code exists;
- **superseded** — historically real but no longer current authority.

## 1. Precursor: natural RAW/GCam and lens-specific thinking

The earliest line treated RAW as something that should be understood quantitatively rather than cosmetically. MotionCam RAW capture, noise modelling, lens identity and a natural rendering target were already central. The tele path was important early.

This stage matters historically because later FotoGraaf/metrology work did not appear from nowhere: the project already wanted the software to know which lens/sensor/capture conditions had shaped a measurement.

## 2. RAW becomes an inverse problem

The project moved from “develop the photo” toward:

`RAW measurement -> physical/statistical forward model -> estimate latent scene -> image/projection`

The core question became: **what latent scene most plausibly caused the measured values?**

This immediately created the need to distinguish reconstruction from measurement and to acknowledge that missing information cannot be recovered merely by demanding a cleaner image.

## 3. Single-frame identity becomes deliberate

Multi-frame reasoning was explored historically, but the project ultimately retained one physical frame and one independent evidence set as the master identity.

Permanent invariants later became:

- `physicalFrameCount = 1`
- `independentEvidenceCount = 1`

Virtual observations and counterfactual worlds do not increase those counts.

## 4. 2026-09-03: Scientific Master replaces “better DNG” as the real target

A major re-audit established that a reconstructed CFA/DNG should not be the final scientific state merely because cameras traditionally use DNG/Bayer containers.

The architecture separated:

1. immutable source evidence;
2. measurement domain;
3. full-colour camera-native latent/reconstructed scene;
4. colorimetric/scientific scene master;
5. compatibility/display exports.

This is the technical descendant of the historical “RGB RAW weer terug” idea: a full-colour reconstructed master is more natural for scene science than forcing every reconstructed quantity back into an artificial Bayer mosaic.

## 5. “Unlimited new house” -> representation freedom

The historical “sealed old house / new house” metaphor became a major design tool.

- old/sealed house: original Direct-CFA/RAW evidence and capture provenance;
- new house: a separately reconstructed scene world.

The crucial historical leap was that the new house did **not** have to inherit the representational limits of the source container. This opened the door to:

- signed/float scene values;
- values above normalized unity;
- escape from source `WhiteLevel` as an output ceiling;
- ISO-neutral reconstructed scene representation;
- large/unbounded TruthRange coordinates;
- multiple virtual observations/projections from one scientific scene;
- counterfactual illumination and virtual camera views.

The later scientific correction is equally important: representation freedom does not create additional evidence.

## 6. TruthRange / zero-line

The historical “middenlijn” became a formal scene coordinate:

`T = log2(L/L0)`

`L0` is a reference/gauge, not sensor black, absolute darkness, DNG BlackLevel, clipping or zero photons.

This separated four ideas that had previously been easy to conflate:

- representational/address range;
- evidence-supported range;
- reconstructed-support range;
- presentation/output range.

Permanent statement:

> **The axis can be infinite. The evidence is finite. The reconstruction may go beyond the evidence only as reconstruction.**

## 7. Virtual observations and counterfactual light

Once the Scientific Master was no longer treated as one fixed rendered exposure, the project explored virtual EV/ISO views and a Virtual Observation Manifold.

The correction that survived is:

- many virtual views may be useful;
- they remain views of the same single evidence set;
- they do not become independent exposures.

Counterfactual illumination later gained its own authority floor. Simulated sun/night/light can describe a hypothetical world but must not write itself back into captured-world evidence.

## 8. Rooms, Runtime and Technical Backplane

The “house” metaphor became software architecture with logical rooms such as Archivist, Measurement Lab, Architect, Restorer, Scene Registry, Surveyor, Manifold Conditioning, CICM/Lighting Studio, Room Capsule, Colorist, Finisher and Exporter.

Two durable conclusions followed:

- a room's scientific permission is controlled by its truth/authority floor;
- RAM/CPU/GPU/thermal resources control execution only.

The Technical Backplane became the compact “digital backside” binding source/master/zero-line/scene-scale/evidence/provenance identity without becoming a second image or new evidence.

## 9. DNG and reconstructed RAW become projections

The architecture clarified:

- original CFA = measured source evidence;
- reconstructed CFA = reconstructed projection;
- LinearRaw/DNG = compatibility/projection state;
- Scientific Master = authoritative reconstructed scene state.

No export is allowed to rewrite the source history or silently replace the Scientific Master.

## 10. Acquisition/metrology returns to the front: FotoGraaf

The project then asked a more upstream question: before reconstructing a RAW, can TruthRaw prove exactly which physical camera/stream/buffer produced the admitted measurement?

FotoGraaf matured into an acquisition/metrology layer rather than an image enhancer.

The verified 4080x3072 tele path and the still-open maximum-resolution path must remain separate claims.

## 11. 2026-09-15: Free Scientific Space formalizes the unlimited-world idea

The active scientific terminology moved beyond treating the “new house” as the formal world boundary.

**Free Scientific Space** means the reconstructed scientific representation may exceed:

- RAW10 source code range;
- source `WhiteLevel` as output ceiling;
- `[0,1]`;
- source ISO working scale;
- DNG storage;
- original CFA lattice;
- float32;
- source display/dynamic-range conventions.

The house metaphor remains provenance/history. The permanent law is stricter than the metaphor:

> **Representation can exceed the source. Knowledge claims cannot exceed the evidence.**

## 12. 17:28–19:26 — precision becomes an authority problem

Historical-chat recovery plus repository state show the next important shift.

The full-file sweep was intentionally treated as locator/evidence summary, while real C++ fixtures were intended to lock the scientifically relevant branch hazards. This is an early machine-readable promotion/authority concept: a value is not accepted merely because software produced it.

Real MotionCam and Honor sources demonstrated that tiny F32/F64 differences before a branch threshold could select different reconstruction branches. This invalidated the simplistic rule “F32 error is smaller than sensor noise, therefore F32 is always safe.”

The mature precision direction became:

`exact RAW integer/packed evidence`
`-> F32 only where demonstrated safe`
`-> F64 branch-sensitive reconstruction`
`-> F64 calibration / optimization / covariance`
`-> controlled F32 storage after F64 compute`
`-> arbitrary/high precision as reference validator`

Key point: **compute precision is not storage precision, and neither is evidence authority.**

## 13. Eight-file mixed-precision promotion evidence

Two MotionCam/Direct-CFA sources and six Honor vendor DNGs were used to consolidate the precision gate.

Retained v0.4 authority result:

- 50,135,040 direction sites;
- 3,545 F32/F64 branch differences;
- 0 green-clamp differences;
- 0 colour-clamp differences;
- 0 measured-channel violations;
- maximum local reconstruction difference `0.03754056890225277`;
- maximum branch amplification `204871.6928905374x`;
- maximum F64-compute -> F32-storage error `5.960464477539063e-08`;
- 0 half-ULP storage violations.

Earlier `3549 / 44 / 2580` counts remain preserved as **superseded provisional locator output**, not current authority.

## 14. 19:33–20:08 — uncertainty binding becomes epistemic authority

The v0.8 path correctly used `uncertaintyApplied=false` as an audit hook rather than inventing uncertainty.

The next question was no longer only “is the computation numerically stable?” but:

**is the local uncertainty/support genuinely bound to this exact Scientific-Master quantity, coordinate and identity?**

The v0.9 direction therefore became fail-closed:

- only existing p50/p95 or other real runtime quantities may be bound;
- canonical identity/binding must match;
- missing runtime data -> unresolved;
- no sigma fabricated from quantiles;
- missing covariance is not zero.

This is the second parent of later Dynamic Authority.

## 15. v5.0g/p1 historical feature extractor blocker

The frozen model expects 18 exact historical features. The model/feature-schema provenance and historical role-order behavior could be recovered, including evidence that a prospective holdout used the expected extractor identity.

But the exact 10,023-byte `uncertainty_core_v5_0g.py` source itself has not been recovered.

Therefore the formal blocker remains:

`OPEN_NEEDS_EXACT_V5G_FEATURE_EXTRACTOR_RECOVERY_OR_HASH_VERIFIED_EQUIVALENT_FEATURE_DEFINITION`

No implementation may infer those 18 semantics from names or “clean up” the historical mapping.

## 16. 20:35–20:38 — handoff to true 200 MP FotoGraaf Step 3B

The uncertainty blocker was deliberately separated from unrelated physical acquisition work.

The next required physical chain became:

`logical camera 0`
`-> physical camera 5`
`-> SENSOR_PIXEL_MODE_MAXIMUM_RESOLUTION`
`-> RAW_SENSOR 16320x12288`
`-> physical TotalCaptureResult 5`

A successful proof would support an app-visible maximum-resolution RAW_SENSOR CFA claim, **not** automatically untouched native photodiode/ADC output.

4080x3072, 8160x6144 and 16320x12288 remain separate domains until measurements show transferability.

## 17. Next chat: proof contracts -> Structure Evidence -> Dynamic Authority

The next development line continued the Camera-5/200MP preparation and fixed proof-tooling issues without weakening the proof contract.

The architecture then generalized: rather than carrying only radiance, each tile/pixel should also carry the machine-readable authority of that quantity.

This became the Dynamic Authority line:

- `MEASURED`
- `RECONSTRUCTED`
- `CENSORED`
- `UNKNOWN`
- `COUNTERFACTUAL`

Dynamic Authority therefore has two historical parents:

1. numerical/promotion authority from mixed-precision gating;
2. local epistemic authority from uncertainty/provenance binding.

## 18. Open-world correction

The next conceptual correction was that the reconstructed world is not a finite literal house.

The active representation may contain interior, exterior, street, landscape, sky, distant geometry or arbitrary scene structure. A Room Capsule limits local compute only.

This does **not** widen the evidence. It widens the representational world in which finite evidence and labelled reconstruction coexist.

## 19. HDR / Adobe correction

HDR work preserved the same authority discipline.

A saturated/censored sample can justify a bound such as “true radiance is at least the saturation boundary”, but not an invented exact value above that boundary.

Adobe Gain Map was classified as downstream display/presentation adaptation rather than another exposure. Lightroom may finish/display HDR, but TruthRaw must decide the scientific scene and authority first.

`UNKNOWN` does not create scientific HDR headroom simply because the representation can hold large numbers.

## 20. Dynamic Authority v1.9 and source binding

By the late open-world line, source fingerprints, Scientific Master identity and Dynamic Authority were bound strongly enough to keep measured authority source-specific.

Frozen references later carried into the Android v0.3 report:

- Scientific Master: `a86034da7b6f9663640ee3b4478ccf4294d23b720f2b087d083dca38f5fb4640`
- Dynamic Authority v1.9: `7678a0b145f8721347cdcc5177fedb720ba19b91bf34984a3f18bd8216408098`
- source-bound P3 transform: `2105712a1be9950089976ba358afd4b062347680d5e6d8c500462c2e8d541533`
- reference `L0`: `0.12564234435558320`

## 21. 2026-09-16 real Android v0.3 device PASS

Device-report verified:

- schema `TruthRawAndroidVerificationReport/0.3`;
- classification `DEVICE_VALIDATION_ONLY_NO_SCIENTIFIC_WRITEBACK`;
- app `0.3-debug`;
- device `HONOR BKQ-N49`;
- Android `16 (API 36)`;
- selected source `IMG_BNC_TRUTHRAW20260907_094449_565.dng`;
- result `PASS_EXACT_SOURCE_AND_DECODED_CFA`;
- exact source identity match;
- exact independently decoded CFA identity match.

The report explicitly records:

- Scientific Master recomputed on device: false;
- Dynamic Authority recomputed on device: false;
- HDR projection run on device: false;
- creates new sensor evidence: false;
- scientific writeback allowed: false;
- appearance/transport may upgrade authority: false.

This closes the scoped device source/CFA identity gate only.

## 22. Current change in this consolidation line

The 2026-09-16 integration branch now makes the recovered knowledge operational by:

- replacing the stale root/bootstrap reading order;
- adding a current Free Scientific Space/open-world architecture document;
- adding a new document-status index instead of rewriting historical snapshots;
- adding a current project-state JSON;
- preserving the exact v0.3 device report as evidence;
- adding a machine-readable regression test for that report;
- explicitly wiring that regression into open-world CI;
- updating Android documentation from “v0.2 screenshot passed / v0.3 awaiting real run” to the real v0.3 exported PASS.

## 23. What remains open

The project must continue fail-closed on at least these points:

1. exact historical v5.0g extractor recovery/equivalence;
2. true qualifying Camera-5 16320x12288 RAW_SENSOR evidence;
3. readout-domain-specific uncertainty/precision/calibration for that future 200 MP route;
4. independent physical color/illuminant/optics calibration for stronger FULL_PHYSICAL claims;
5. counterfactual light remains hypothetical;
6. research success does not automatically become canonical/main promotion.

## 24. Historical interpretation

The project should no longer be narrated as:

`ambitious old vision -> project went off course -> strict new vision replaced it`

A more faithful line is:

`natural/lens-aware RAW idea -> RAW as measurements -> inverse scene reconstruction -> uncertainty and information limits -> Scientific Master beyond DNG -> unlimited/new-house representation idea -> TruthRange -> rooms/backplane/counterfactual projections -> Free Scientific Space -> precision authority -> local uncertainty authority -> FotoGraaf physical promotion -> Dynamic Authority -> open-world correction -> HDR/Adobe authority -> real device identity verification`

The original ambition was not discarded. It was made progressively more explicit about what is measured, what is reconstructed and what must remain unknown.
