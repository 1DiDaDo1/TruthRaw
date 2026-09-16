# TruthRaw current scientific architecture — 2026-09-16

Status: **CURRENT RESEARCH ARCHITECTURE / CONSOLIDATION AUTHORITY**

This document describes the active scientific architecture of the 2026-09-16 integration line. It does not retroactively rewrite older dated documents and it does not by itself promote research code to canonical/main.

## 1. Permanent law

> **The original admitted source is immutable evidence. Every downstream state remains tied to that evidence, to its uncertainty/support and to its authority class. No reconstructed, censored, unknown, counterfactual or appearance state may be relabelled as measured.**

Short form:

> **Representation can exceed the source. Knowledge claims cannot exceed the evidence.**

## 2. From houses to Free Scientific Space

TruthRaw historically used a house metaphor:

- the **sealed old house** represented immutable RAW/CFA evidence;
- the **new house** represented the separately reconstructed scene world.

That metaphor remains important project history, but the active formal representation is broader:

- **Source Evidence** is sealed/immutable;
- the reconstructed domain is **Free Scientific Space**;
- the scene representation is open-world and not bounded by a literal house or Room Capsule.

A Room Capsule bounds local computation. It does not prohibit exterior, street, landscape, sky, distant structure or other scene geometry from existing in the reconstructed representation.

Free Scientific Space may exceed source representation limits such as RAW10 code range, source `WhiteLevel` as an output ceiling, `[0,1]`, original CFA lattice, source ISO working scale, SDR, integer storage or DNG. It may not claim that those representational extensions were physically measured.

## 3. Authority-separated world model

Current logical flow:

`Source Evidence`
`-> Measurement / de-ISP domain`
`-> Scientific Master`
`-> Dynamic Authority + uncertainty/support`
`-> Open Scene State`
`-> optional Counterfactual State`
`-> Appearance / HDR / Transport`
`-> finite projections/exports`

The Scientific Master is the authoritative reconstructed scene state. DNG, LinearRaw, reconstructed CFA, SDR/HDR renderings and Adobe-compatible files are downstream projections unless an exact module contract says otherwise.

## 4. Source Evidence

Source Evidence includes admitted app-visible RAW/CFA sample bytes and capture provenance. For a source-bound validation path, byte identity and decoded CFA identity may both be frozen and checked independently.

Current real target-device v0.3 verifier result:

- DNG: `IMG_BNC_TRUTHRAW20260907_094449_565.dng`
- source bytes: `25106120`
- source SHA-256: `7930ba5db0fe1b7b9dcc09e9337efbca76b8877fb78a6dc4667761bf25159b67`
- decoded CFA: `4080 x 3072`
- strips: `3072`
- decoded CFA bytes: `25067520`
- decoded CFA SHA-256: `883cbe13fd2a5afe7e9f3f4147df3a8ed3be681340a3c0422933f42d0f4d719c`
- result: `PASS_EXACT_SOURCE_AND_DECODED_CFA`

The verifier classification is `DEVICE_VALIDATION_ONLY_NO_SCIENTIFIC_WRITEBACK`.

This validates the scoped app/device path. It does not prove untouched photodiode/ADC output and it does not create additional sensor evidence.

## 5. Scientific Master

Current frozen research reference for the admitted source:

`a86034da7b6f9663640ee3b4478ccf4294d23b720f2b087d083dca38f5fb4640`

The Scientific Master is not constrained to be a DNG or a Bayer mosaic. Full-colour camera-native scene representation is allowed as reconstruction when its authority/support are explicit.

A source CFA sample can be `MEASURED`; a newly reconstructed missing colour is not. A remosaiced/reconstructed CFA is a projection of the reconstructed world, not original sensor evidence.

## 6. Dynamic Authority

Current frozen Dynamic Authority v1.9 reference:

`7678a0b145f8721347cdcc5177fedb720ba19b91bf34984a3f18bd8216408098`

Authority labels describe the epistemic class of local quantities. The current architecture distinguishes at least:

- `MEASURED`
- `RECONSTRUCTED`
- `CENSORED`
- `UNKNOWN`
- `COUNTERFACTUAL`

Appearance/transport is downstream and may not upgrade these classes.

Dynamic Authority is the generalization of two earlier research gates:

### 6.1 Numerical/promotion authority

A value is not scientifically admissible merely because an implementation can compute it. The compute/storage path must satisfy its promotion evidence.

### 6.2 Uncertainty/provenance binding

Uncertainty/support must be bound to the same Scientific-Master quantity, coordinates and identity. Missing fields, mismatched identities or unproven feature semantics fail closed.

## 7. Precision architecture

Real-source investigation demonstrated branch-sensitive F32/F64 divergence. Therefore precision is stage-specific:

`exact packed/integer RAW evidence`
`-> F32 in demonstrated-safe operations`
`-> F64 branch-sensitive reconstruction`
`-> F64 calibration / optimization / covariance`
`-> optional controlled F32 Scientific-Master storage after F64 compute`
`-> arbitrary/high precision as reference validation`

A later F32-to-F64 cast cannot repair a branch decision already made differently in F32.

Retained eight-file v0.4 promotion evidence:

- 8 real RAW/DNG sources;
- 50,135,040 directional reconstruction sites;
- 3,545 F32/F64 branch differences;
- 0 green-clamp differences;
- 0 colour-clamp differences;
- 0 measured-channel violations;
- maximum local F32/F64 reconstruction difference: `0.03754056890225277`;
- maximum branch amplification: `204871.6928905374x`;
- maximum F64-compute -> F32-storage error: `5.960464477539063e-08`;
- half-ULP storage violations: 0.

Historical `3549 / 44 / 2580` output is superseded provisional locator provenance, not authority.

## 8. Uncertainty boundary

The historical frozen v5.0g/p1 model expects 18 exact hidden-CFA/features. The model specification, feature-schema provenance and historical compatibility behavior are known, but the exact 10,023-byte `uncertainty_core_v5_0g.py` source has not been recovered.

Therefore:

- do not infer feature semantics from their names;
- do not construct an approximate adapter;
- preserve the historical role/order quirk;
- keep the local feature bridge unresolved until exact source recovery or a hash-verified equivalent feature definition exists.

This blocker does not justify borrowing uncertainty from another readout domain and does not block unrelated physical acquisition work.

## 9. TruthRange / zero-line

For positive scene light:

`T = log2(L/L0)`

Current source-bound reference carried by the verifier:

`L0 = 0.12564234435558320`

`L0` is a gauge/reference, not sensor black, zero photons, clipping, display black or middle gray by definition.

TruthRange address space may be unbounded while evidence remains finite. Reconstruction beyond the evidence window must remain labelled reconstruction/uncertainty, not new measurement.

## 10. Open-world correction

The represented scene is not confined to the old house metaphor.

A local Room Capsule may cover one computational region while the broader Open Scene State contains other reconstructed scene structure. The architecture therefore separates:

- **world extent** — potentially open-ended representation;
- **evidence support** — finite and source-bound;
- **compute extent** — local/tiled/resource-dependent;
- **presentation extent** — finite output chosen for a device or file.

These must not be collapsed into one notion of “dynamic range” or “image size”.

## 11. Single-frame evidence invariants

Current master identity remains:

- `physicalFrameCount = 1`
- `independentEvidenceCount = 1`

Virtual EV/ISO observations, manifold views, display variants and counterfactual renders do not increase either count.

## 12. Counterfactual illumination

CICM/Lighting Studio may create a hypothetical state under changed illumination/capture assumptions. That state is useful, but it is not evidence of the captured world.

A counterfactual path must retain:

- source/master identity;
- evidence count = one;
- `createsNewEvidence = false`;
- `scientificWritebackAllowed = false`;
- no appearance/transport authority upgrade.

Physically strong relighting requires additional geometry/material/visibility/illumination information. A plausible learned or inferred relight is not retroactively measured truth.

## 13. HDR and Adobe

Scientific HDR headroom is authority-aware.

- `MEASURED` may support exact scientific headroom where the measurement is valid;
- `CENSORED` supports a bound, not invented exact radiance;
- `UNKNOWN`, `RECONSTRUCTED` and `COUNTERFACTUAL` do not become measured HDR headroom merely by being representable;
- display Gain Maps are presentation/transport adaptation.

Current source-bound P3 transform reference:

`2105712a1be9950089976ba358afd4b062347680d5e6d8c500462c2e8d541533`

Lightroom/Adobe may finish or display HDR. It does not determine the Scientific Master.

## 14. FotoGraaf / Camera 5 / maximum-resolution gate

FotoGraaf is acquisition/metrology upstream of TruthRaw reconstruction.

The intended maximum-resolution proof chain is:

`logical camera 0`
`-> physical camera 5`
`-> SENSOR_PIXEL_MODE_MAXIMUM_RESOLUTION`
`-> RAW_SENSOR 16320 x 12288`
`-> physical TotalCaptureResult 5`

A successful proof may be classified as app-visible maximum-resolution RAW_SENSOR CFA. It must not silently become `UNTOUCHED_NATIVE_200MP_ADC`.

Qualifying evidence should include the actually applied mode, real Image dimensions, `Image.timestamp == SENSOR_TIMESTAMP`, row/pixel stride, buffer length/padding, original rawsensor bytes/hash, CFA, black/white levels, NoiseProfile if available, exposure/ISO/focus/stabilisation and a bound capture result. DNG may be an auxiliary container, not the primary proof of RAW_SENSOR buffer identity.

4080x3072, 8160x6144 and 16320x12288 remain separate sample/readout domains until measurements demonstrate transferability.

## 15. Execution architecture and device scaling

The older 12-room Building Runtime remains a valid execution abstraction. Scientific permission and resource allocation are separate axes.

Weak and strong phones may differ in:

- tile size;
- cache size;
- concurrency;
- CPU/GPU/Vulkan acceleration;
- rebuild/eviction policy.

They may not differ in evidence count, uncertainty semantics or authority rules merely because one device is faster.

## 16. Technical Backplane

The Technical Backplane is a compact digital backside binding identities such as source, Scientific Master, zero-line/scene scale, evidence counts and room/provenance status.

It is not a second image and not new evidence. It exists to make cross-room identity/permission machine-readable without duplicating the full image state.

## 17. Current blockers and next permitted work

Hard blockers:

1. recover exact historical v5.0g extractor bytes or a hash-verified equivalent feature definition;
2. obtain a true qualifying 16320x12288 Camera-5 RAW_SENSOR evidence set before physical 200 MP promotion;
3. do not upgrade color/illuminant/optics to FULL_PHYSICAL without independent calibration;
4. do not treat counterfactual light or Adobe transport as captured evidence.

Safe current continuation:

- strengthen machine-readable Android v0.3 evidence validation;
- preserve and regression-test frozen downstream identities;
- continue open-world/Dynamic-Authority integrity tests without widening claims;
- prepare the 200 MP promotion path to ingest real evidence when supplied.
