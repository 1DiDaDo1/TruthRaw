# Scientific Master Streaming Binding v0.1

Status: **RESEARCH IMPLEMENTATION CANDIDATE — CI PROOF REQUIRED**

Parent validated phase-2 head:
`ee9991d10ef0649293f0a9c951d90000cd3858d9`

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

## Identity and authority

Output identity contains:
- actual Scientific Master SHA-256 from camera-native reconstructed RGB;
- actual TruthRange self-gauge/zero-line state;
- scene scale `TRUTHRANGE_SELF_GAUGE_STAGE2_V0_2`;
- one physical frame;
- one independent evidence root.

The scientific pass does not create a second image claim. It binds the scene state that the later preview must reference.

## Validation gates

The test suite requires all of the following:

- the bounded 64x64 streaming Scientific Master hash equals the hash of the existing full latent-scene camera RGB for the same source/backend;
- bounded radix `L0` equals `derive_self_gauge_v0_2(..., 0.5, 0.10)` exactly;
- the streaming identity feeds the validated Technical Backplane phase-2 finalization path successfully;
- changing `cameraToXyzD50` cannot change Scientific Master hash or zero-line because both are pre-color;
- a too-small logical resident budget fails closed;
- frame/evidence counts remain 1/1.

CI must pass GCC Release, Clang Release and Clang ASan/UBSan before promotion.

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

**Measured where measured. Reconstructed where necessary. Never invented.**
