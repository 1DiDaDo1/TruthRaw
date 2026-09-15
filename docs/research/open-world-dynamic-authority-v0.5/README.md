# TruthRaw Open-World Dynamic Authority Field v0.5

Status: **RESEARCH — NOT MAIN-PROMOTED**

## Purpose

This step turns the project's "open-ended dynamic photo" idea into a machine-checkable scientific field without claiming infinite sensor dynamic range.

The represented scene has no fixed 0..1 container and no fixed EV wall. Positive radiometric values may be mapped to TruthRange

`T = log2(L / L0)`

without an arbitrary clamp. Signed scene-linear estimators are preserved separately when they are zero or negative and therefore do not have a positive-light TruthRange coordinate.

The central rule remains:

> Representation may exceed the source; knowledge claims may not exceed the evidence.

## Per-sample authority

v0.5 keeps these states distinct:

- `MEASURED`
- `CALIBRATED_ESTIMATE`
- `RECONSTRUCTED`
- `CENSORED`
- `UNKNOWN`
- `COUNTERFACTUAL`
- `APPEARANCE_ONLY`

A Scientific Master Dynamic Authority Field may contain the first five states only. Counterfactual and appearance output must live in separate view fields and cannot write back as Scientific Master authority.

`CENSORED` is not treated as reconstructed data. A clipped highlight may carry a known censor bound, but the hidden radiance above that bound remains unknown until independent evidence justifies a reconstruction.

## Four dynamic-range meanings

v0.5 deliberately separates:

1. **Representation range** — the span of positive values currently represented in the field.
2. **Evidence-supported range** — positive `MEASURED` + `CALIBRATED_ESTIMATE` values only.
3. **Reconstruction-supported range** — positive `RECONSTRUCTED` values only.
4. **Scientific-supported range** — the union of evidence-supported and reconstructed values, without counterfactual/appearance output.

No one of these numbers is allowed to masquerade as the sensor's physical measured dynamic range.

The summary therefore stores `fixed_dynamic_range_limit_ev = null`. This means TruthRaw does not impose an architectural EV cap; it does **not** mean finite floating-point storage or a finite sensor is mathematically infinite.

## Cheap phone -> heavy hardware

The scientific contract is hardware-independent. `plan_dynamic_execution()` receives an explicit memory/core budget and chooses only execution geometry:

- tile-core size,
- halo size,
- concurrency,
- estimated peak working memory.

A lower-memory phone can therefore use small bounded workspaces and stream results to persistent storage. A stronger phone/workstation may use larger tiles and more concurrency. Both plans retain the same `scientific_policy_sha256` for the same source/master binding.

Truth may become slower on weak hardware; it may not become different truth.

## Chunk-independent streaming identity

`StreamingDynamicAuthorityAccumulator` requires global raster order but does not include chunk boundaries in the scientific content digest. The same sample sequence emitted in small or large chunks produces the same `content_sha256` and the same dynamic summary.

This is the core mechanism needed to make a 200.5 MP field possible without requiring the entire field in RAM.

## Claim boundaries

v0.5 does **not** claim:

- infinite measured dynamic range;
- recovery of physically clipped highlights without evidence;
- recovery of noise-floor detail as measured structure;
- that a positive TruthRange coordinate is absolute physical radiance;
- that counterfactual relighting is another exposure;
- that presentation HDR increases scientific evidence;
- that 4080x3072 uncertainty authority transfers to the 16320x12288 Camera-5 route.

The current v0.4 domain correction remains binding: Camera-5 maximum-resolution uncertainty needs its own validation or a defensible separately proven cross-mode equivalence.

## Files

- `tools/open_world_dynamic_authority_v05.py`
- `tests/test_open_world_dynamic_authority_v05.py`
- `state/OPEN_WORLD_DYNAMIC_AUTHORITY_V05_STATE.json`

## Next gate

The next integration should construct this field from real Scene Master / v0.4 runtime artifacts rather than synthetic sample records, then persist it through the Technical Backplane and streamed artifact store. The 16320x12288 Camera-5 route remains blocked from uncertainty-backed reconstruction authority until maximum-resolution uncertainty is independently validated.
