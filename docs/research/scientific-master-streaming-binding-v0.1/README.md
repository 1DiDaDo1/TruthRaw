# Scientific Master Streaming Binding v0.1

Status: **RESEARCH_CANDIDATE_CI_PASS — STREAMING_MASTER_AND_TRUTHRANGE_EQUIVALENCE_PROVEN**

Validated implementation anchor:
`2504f78b84cf016d9236a8a36be91214da2a46a9`

Validated CI evidence:
- Scientific Master Streaming Binding v0.1 run `34640922384` — **SUCCESS**
- Documentation Governance run `34640922377` — **SUCCESS**
- GCC Release, Clang Release and Clang ASan/UBSan — all build/test **SUCCESS**

Parent validated phase-2 head:
`ee9991d10ef0649293f0a9c951d90000cd3858d9`

Historical negative evidence retained:
- run `34640698841` failed during build because the first integration attempt qualified `TileRect` through the wrong nested namespace and retroactively applied conversion warnings to validated v0.1 headers;
- run `34640820471` failed during build because the first test reused a historical test-support header whose general `StatusCode` and unused helpers conflicted with this stricter integration test;
- both failures remain recorded and were corrected without editing the validated historical Full-Frame Streaming v0.1 implementation.

## Purpose

This module connects the validated Scientific Master Digest and Technical Backplane phase-2 work to a real bounded `IRawTileSource` + v4.7i reconstruction route without modifying the validated Full-Frame Streaming v0.1 bytes.

The scientific identity path is:

`IRawTileSource`
`-> exact Stage-2 tile construction (BlackLevel/residual black/GainMap exactly once)`
`-> canonical 64x64 v4.7i camera-native RGB reconstruction`
`-> Scientific Master Digest v0.1`
`-> exact TruthRange v0.2 self-gauge median`
`-> Technical Backplane Phase 2 v0.1`
`-> existing Scientific Preview finalization gate`

No XYZ conversion, appearance backend, tone curve, SDR LUT, JPEG encoding or counterfactual state participates in the Scientific Master hash.

## Why this is a separate scientific pass

Full-Frame Streaming v0.1 reconstructs twice: once for global exposure statistics and once for final appearance/output. Its second pass may reconstruct an appearance halo, so observing those calls indirectly would make Scientific Master identity dependent on appearance halo and runtime execution details.

Instead, this module defines one explicit scientific pass on the same source/backend using the already-canonical 64x64 digest grid. This keeps identity independent of preview tile size and avoids changing validated Full-Frame Streaming v0.1.

## Bounded-memory self-gauge

TruthRange v0.2 derives a self gauge from the median of positive, finite, uncensored Stage-2 samples after the canonical 10% border exclusion.

The existing full latent implementation sorts those samples in memory. For phone-scale streaming, this module computes the same median exactly with float32 radix selection:

1. first scientific pass: reconstruct/digest the master while counting the most-significant byte of every eligible positive float32 Stage-2 value;
2. select the lower/upper median byte bucket;
3. perform three additional Stage-2-only bounded scans to refine the next three bytes;
4. recover the exact lower/upper median float32 values;
5. calculate `L0 = lower + (upper-lower)*0.5`, matching TruthRange v0.2 median interpolation.

Only two 256-entry histograms are needed for radix refinement. A full Stage-2 frame is never retained.

The resulting gauge remains:
- `mode = SelfGauge`
- `gaugeId = SELF_GAUGE_STAGE2_Q0.500000`
- `crossSceneComparable = false`
- `absolutePhysicalUnits = false`.

ISO is not part of this derivation.

## What the validated test proves

For the same synthetic source and the same v4.7i research reconstruction backend:
- the bounded canonical streaming Scientific Master hash is exactly equal to the hash of the existing full latent-scene camera RGB;
- the bounded radix-selected `L0` is exactly equal to `derive_self_gauge_v0_2(..., 0.5, 0.10)`;
- the streaming-generated master/gauge/scene-scale identity successfully enters Technical Backplane Phase 2 and the existing Scientific Preview finalization gate;
- changing `cameraToXyzD50` does not change Scientific Master identity or zero-line because they are both pre-color;
- a too-small resident-memory budget fails closed;
- `physicalFrameCount = 1` and `independentEvidenceCount = 1` remain invariant.

## Non-claims

This module does not yet prove:
- physical Honor/MotionCam execution;
- a real user DNG completing the full route;
- dual-illuminant DNG color interpolation;
- FULL_PHYSICAL color calibration;
- cross-CPU bit-identical reconstruction output;
- visible Android Scientific Preview output;
- Lightroom-readable TruthRaw DNG output.

Those remain later stages. In particular, the visible preview designed earlier is reused after this scientific identity chain; this module does not redesign it.

## Next step

Connect the already-designed reconstructed/color preview representation to the finalized phase-2 admission so a visible preview is released **only after** the exact source, Scientific Master, zero-line and scene-scale bindings have passed the Backplane gate. Then carry that same gated representation into the Android ARGB_8888/JPEG path.

**Measured where measured. Reconstructed where necessary. Never invented.**
