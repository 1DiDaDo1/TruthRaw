# TruthRaw consolidated handoff — 2026-09-16

Status: **CURRENT INTEGRATION HANDOFF / NOT A MAIN PROMOTION**

This handoff is the shortest reliable bridge into the fully consolidated project state. It must be read together with `docs/CURRENT_SCIENTIFIC_ARCHITECTURE_2026-09-16.md` and `state/CURRENT_PROJECT_STATE_2026-09-16.json`.

It supplements, rather than rewrites, `docs/handoff/TRUTHRAW_DETAILED_HANDOFF_2026-09-16.md` by adding the recovered 17:28–20:38 precision/uncertainty/FotoGraaf chronology and the real exported Android v0.3 device PASS.

## 1. Permanent law

**Source evidence is sealed/immutable. Reconstruction is representationally free. Knowledge claims remain evidence-bounded.**

No reconstructed, censored, unknown, counterfactual, appearance or transport state may be relabelled as measured.

## 2. Current world model

Use:

`Source Evidence -> Measurement -> Scientific Master in Free Scientific Space -> Dynamic Authority/uncertainty/support -> Open Scene State -> optional Counterfactual State -> Appearance/HDR/Transport -> finite export`

The historical sealed-house/new-house language is retained as provenance. The formal world is Free Scientific Space and is not bounded by a literal house or Room Capsule.

## 3. Frozen source and target-device PASS

Real exported Android report:

- file/report schema: `TruthRawAndroidVerificationReport/0.3`
- classification: `DEVICE_VALIDATION_ONLY_NO_SCIENTIFIC_WRITEBACK`
- app: `0.3-debug`
- device: `HONOR BKQ-N49`
- Android: `16 (API 36)`
- timestamp: `2026-09-16T09:02:07.303793Z`
- source: `IMG_BNC_TRUTHRAW20260907_094449_565.dng`
- result: `PASS_EXACT_SOURCE_AND_DECODED_CFA`
- source identity match: true
- CFA identity match: true

Source identity:

- bytes `25106120`
- SHA-256 `7930ba5db0fe1b7b9dcc09e9337efbca76b8877fb78a6dc4667761bf25159b67`

Decoded CFA identity:

- 4080 x 3072
- 3072 strips
- 25067520 bytes
- SHA-256 `883cbe13fd2a5afe7e9f3f4147df3a8ed3be681340a3c0422933f42d0f4d719c`

The report explicitly says:

- Scientific Master recomputed on device: false
- Dynamic Authority recomputed on device: false
- HDR projection run on device: false
- creates new sensor evidence: false
- scientific writeback allowed: false
- appearance/transport may upgrade authority: false

Therefore this is a **device-validation PASS for exact source/CFA identity only**.

## 4. Android v0.3 build identity

Retained signed APK SHA-256:

`01f94593ebd9dcb8d7e5f8c5681eed1f79f469de3862b98723912fb9a36d8b61`

The Android app remains a non-canonical debug/integration client. APK behavior does not define scientific truth.

## 5. Frozen downstream source-bound references

- Scientific Master SHA-256:
  `a86034da7b6f9663640ee3b4478ccf4294d23b720f2b087d083dca38f5fb4640`
- Dynamic Authority v1.9 SHA-256:
  `7678a0b145f8721347cdcc5177fedb720ba19b91bf34984a3f18bd8216408098`
- source-bound P3 transform SHA-256:
  `2105712a1be9950089976ba358afd4b062347680d5e6d8c500462c2e8d541533`
- reference `L0`:
  `0.12564234435558320`

Android v0.3 carries these as frozen references; it did not recompute them.

## 6. Precision state recovered from previous-chat history

The precision architecture is stage-specific:

`exact RAW integer/packed evidence -> F32 only where proven safe -> F64 branch-sensitive reconstruction -> F64 calibration/optimization/covariance -> controlled F32 storage after F64 compute -> arbitrary precision reference validation`

Retained eight-file v0.4 evidence:

- 8 real sources;
- 50,135,040 directional reconstruction sites;
- 3,545 F32/F64 branch differences;
- 0 green clamp differences;
- 0 colour clamp differences;
- 0 measured-channel violations;
- maximum local reconstruction difference `0.03754056890225277`;
- maximum branch amplification `204871.6928905374x`;
- maximum F64-compute -> F32-storage error `5.960464477539063e-08`;
- 0 half-ULP storage violations.

Treat `3549/44/2580` only as superseded provisional locator provenance.

## 7. Uncertainty state

The v0.8 audit path correctly left `uncertaintyApplied=false` until genuine uncertainty could be bound.

The v0.9 direction requires local uncertainty/support to bind to the same Scientific-Master coordinates and canonical identity. Missing/mismatched data stays unresolved; missing covariance is not zero and sigma is not fabricated from quantiles.

The historical v5.0g/p1 model expects 18 features. The exact historical 10,023-byte `uncertainty_core_v5_0g.py` remains missing.

Formal blocker:

`OPEN_NEEDS_EXACT_V5G_FEATURE_EXTRACTOR_RECOVERY_OR_HASH_VERIFIED_EQUIVALENT_FEATURE_DEFINITION`

Do not infer feature semantics from names and do not clean up historical compatibility quirks.

## 8. Dynamic Authority lineage

Dynamic Authority is not an isolated late idea. It generalizes two earlier gates:

1. numerical/promotion authority from mixed-precision validation;
2. local epistemic authority from uncertainty/provenance binding.

Current relevant classes:

`MEASURED`, `RECONSTRUCTED`, `CENSORED`, `UNKNOWN`, `COUNTERFACTUAL`.

A display/appearance state cannot upgrade these classes.

## 9. Open-world correction

The reconstructed world is not a finite “new house”. The house metaphor remains useful history, but Free Scientific Space may represent broader scene structure. Room Capsule limits local compute only.

Open world does not mean open evidence. Evidence remains finite and source-bound.

## 10. HDR / Adobe

Scientific HDR remains authority-aware.

- censored values may support bounds, not invented exact radiance;
- `UNKNOWN` creates no scientific HDR headroom;
- Adobe Gain Map = `PRESENTATION_AND_DISPLAY_ADAPTATION_ONLY`;
- no fake multi-exposure evidence may be created from one capture;
- Lightroom may be a downstream finisher, never the arbiter of captured scientific truth.

## 11. FotoGraaf / Camera-5 maximum-resolution continuation

Next qualifying physical path:

`logical camera 0 -> physical camera 5 -> SENSOR_PIXEL_MODE_MAXIMUM_RESOLUTION -> RAW_SENSOR 16320x12288 -> physical TotalCaptureResult 5`

Required proof includes real applied mode, Image dimensions, timestamp binding, stride/padding/buffer facts, original rawsensor bytes/hash, CFA, black/white levels, NoiseProfile where available, exposure/ISO/focus/stabilization and physical result binding.

Permitted future bounded claim if proven:

`APP_VISIBLE_MAXIMUM_RESOLUTION_RAW_SENSOR_CFA_PROVEN`

Forbidden without separate evidence:

`UNTOUCHED_NATIVE_200MP_ADC`

Do not borrow 4080x3072 uncertainty into 16320x12288.

## 12. Current repository continuation

The current integration branch is:

`integration/current-truthraw-state-2026-09-16`

It is based on:

`research/open-world-foundations-v01`

base commit:

`976a5e4e0bed7dccdf0391db8b5128041a83652c`

This branch updates living documentation and adds machine-readable v0.3 report validation. It is not a canonical/main promotion.

## 13. Immediate work order

1. preserve the exact real v0.3 verification report as scoped evidence;
2. run a regression that rejects any report changing source/CFA identity, frozen downstream references or fail-closed scientific rules;
3. keep v5.0g feature recovery open until exact/equivalent semantics are proven;
4. keep 200 MP physical promotion open until a real qualifying sample set exists;
5. continue Dynamic Authority/open-world integration only without widening claims beyond evidence.

## 14. Do not regress these rules

- source history is immutable;
- `physicalFrameCount=1`, `independentEvidenceCount=1`;
- virtual/counterfactual/display worlds do not create sensor evidence;
- compute precision, storage precision and evidence authority are separate;
- Free Scientific Space is representational freedom, not permission to invent facts;
- scientific writeback from Android verifier/appearance/transport remains forbidden;
- historical snapshots and failed experiments stay preserved.
