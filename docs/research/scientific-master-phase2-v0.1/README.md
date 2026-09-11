# Scientific Master Phase2 v0.1

Status: **RESEARCH IMPLEMENTATION CANDIDATE — CI PROOF REQUIRED**

Parent branch head:
`a7dd9d4ba00922a71e2099fdcb8001cdf6461b8e`

This module is the first integration step after validated **Scientific Master Digest v0.1**.

## Purpose

Full-Frame Streaming v0.1 already performs the correct two-pass bounded reconstruction path, but its public sink only receives presentation/diagnostic products. The camera-native reconstructed RGB exists transiently inside pass 2 as `w.cam`, immediately after `reconstruction.reconstructTile(...)` and before `camera_to_xyz(...)`.

Scientific Master Phase2 v0.1 binds that transient scientific state to the deterministic Scientific Master digest without modifying frozen v4.7i reconstruction bytes and without rewriting Full-Frame Streaming v0.1 history.

The exact capture point is:

`Stage-2 tile -> v4.7i reconstruction -> CAMERA-NATIVE SCIENTIFIC MASTER DIGEST -> camera->XYZ -> appearance -> SDR/HDR output`

## Scientific boundary

The digest includes only the reconstructed camera-native scene-linear RGB core samples.

It excludes:
- source-evidence hash;
- zero-line identity;
- scene-scale identity;
- XYZ conversion;
- white/display appearance;
- tone curve;
- output acutance;
- SDR/HDR projection;
- preview encoding.

Those identities and roles remain separate. In particular, zero-line and scene-scale still require their own real deterministic bindings before a complete Technical Backplane phase 2 may be claimed.

## Runtime/resource behavior

The integration uses the validated 64x64 canonical digest-cell contract. Phase2 v0.1 therefore requires runtime core tile sizes to be multiples of 64. Current intended mobile tiers 64/128/256/512 satisfy that constraint.

The digest bookkeeping is explicitly added to the streaming resident-memory accounting. A memory-budget failure is an execution/admission failure only; it may not weaken or alter scientific reconstruction.

No full-frame Scientific Master RGB buffer is introduced by the streaming integration.

## Validation design

The test suite constructs one synthetic Direct-CFA-style frame and computes an independent reference Scientific Master by:
1. producing the complete Stage-2 frame;
2. running the frozen v4.7i measured-preserving reconstruction across the whole frame;
3. hashing that camera-native full-frame result through the validated digest contract.

It then runs the actual phase2 streaming integration and requires bit-identical master hashes for:
- 64-pixel streaming cores;
- 128-pixel streaming cores;
- SkinSafeDetailedCrisp appearance;
- NeutralReference appearance;
- HDR enabled vs disabled;
- scientific diagnostics enabled vs disabled.

This directly tests that resource partitioning and presentation choices cannot alter Scientific Master identity.

Additional fail-closed gate:
- a 96-pixel runtime core is rejected because it splits the fixed 64x64 canonical digest grid.

The test also verifies that digest resident state is included in total streaming memory accounting.

## Relationship to Backplane phase 2

A successful run of this module produces a **real Scientific Master SHA-256 from actual reconstructed camera-native samples** rather than the fixture master hashes used by Scientific Preview Source Binding v0.2 tests.

That closes only one of the remaining phase-2 identity inputs.

Still required before finalized Scientific Preview authority:
1. real deterministic zero-line identity;
2. real deterministic scene-scale identity;
3. construction of Technical Backplane v0.1 using exact source/master/zero-line/scene-scale bindings;
4. Source Binding v0.2 finalization using that Backplane;
5. no placeholder/sentinel hashes;
6. physical Honor/MotionCam execution remains a separate hardware proof boundary.

## Non-claims

This module does not claim:
- a completed Technical Backplane phase 2;
- finalized Scientific Preview authority;
- physical Honor execution;
- FULL_PHYSICAL color;
- cross-CPU floating-point bit identity;
- any additional physical frame or independent evidence.

Ordinary lineage remains:
- `physicalFrameCount = 1`
- `independentEvidenceCount = 1`

**Measured where measured. Reconstructed where necessary. Never invented.**
