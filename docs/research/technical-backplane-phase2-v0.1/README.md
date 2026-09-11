# Technical Backplane Phase 2 v0.1

Status: **RESEARCH IMPLEMENTATION CANDIDATE — CI PROOF REQUIRED BEFORE PROMOTION**

Parent validated Scientific Master Digest head:
`a7dd9d4ba00922a71e2099fdcb8001cdf6461b8e`

## Purpose

This module completes the missing post-master bridge that earlier Scientific Preview Source Binding v0.2 could only exercise with fixture hashes.

The phase-2 chain is now:

`prepared exact source + authorized color binding`
`-> real Scientific Master SHA-256`
`-> canonical TruthRange zero-line identity`
`-> canonical scene-scale identity`
`-> Technical Backplane v0.1`
`-> existing Scientific Preview Source Binding v0.2 finalization gate`

No placeholder/dummy hash path exists in `finalize_phase2()`.

## Scientific authority boundaries

The four Backplane identities remain separate:

1. **sourceEvidenceHash** — exact sealed source bytes.
2. **scientificMasterHash** — deterministic digest of the camera-native reconstructed Scientific Master, produced by Scientific Master Digest v0.1.
3. **zeroLineHash** — identity of the already-defined TruthRange gauge/zero-line.
4. **sceneScaleHash** — identity of the already-defined scene-scale contract.

They are not collapsed into one hash because they have different scientific meanings and mutation rules.

### Zero-line binding

The zero-line identity hashes:
- TruthRange gauge mode;
- exact IEEE-754 binary64 bits of `L0`;
- gauge ID;
- cross-scene-comparability flag;
- absolute-physical-units flag.

It does **not** choose or move `L0`. It only binds the gauge that already exists.

For a self gauge, cross-scene and absolute-physical claims are rejected. Physical-absolute mode remains fail-closed unless the scene binding declares both exposure and gain normalization to a common scene.

### Scene-scale binding

The scene-scale identity hashes:
- `sceneScaleId`;
- GainMap-exactly-once flag;
- exposure-normalized-to-common-scene flag;
- gain-normalized-to-common-scene flag.

Capture ISO is intentionally absent. ISO remains capture provenance and is not reintroduced as a Scene/TruthRange coordinate.

## Preview finalization

`finalize_phase2()` first revalidates that the incoming prepared source exactly matches the canonical pre-master state from Scientific Preview Source Binding v0.2.

It then requires:
- a non-zero real Scientific Master SHA-256;
- a valid zero-line gauge;
- a non-empty scene-scale ID;
- GainMap exactly once;
- one physical frame;
- one independent evidence root;
- valid room/claim enums;
- zero forbidden flags.

Only after Technical Backplane v0.1 validates and serializes the 180-byte record does the function invoke the existing `finalize_scientific_color_lineage()` gate.

Therefore the preview cannot become finalized merely because color rendering exists. Finalization depends on the real source/master/zero-line/scene-scale lineage.

## Validation design

The integration test deliberately does not construct fake 32-byte master hashes. Instead it:

1. seals an actual in-memory source byte stream with the existing SHA-256 source sealer;
2. prepares a valid source-metadata-bound color record;
3. constructs finite camera-native reconstructed RGB values;
4. runs those values through **Scientific Master Digest v0.1** to obtain the master SHA-256;
5. supplies a real TruthRange self-gauge and scene-scale binding;
6. builds and serializes Technical Backplane phase 2;
7. passes that Backplane into the existing Scientific Preview v0.2 finalization gate;
8. round-trips the 180-byte Backplane and verifies all four bindings.

Additional falsification checks require:
- zero master digest -> reject;
- invalid self-gauge authority -> reject;
- missing scene-scale ID -> reject;
- GainMap not exactly once -> reject;
- mutated prepared source identity -> reject;
- unsupported physical-absolute gauge without common exposure/gain normalization -> reject;
- changed `L0` changes only the zero-line identity;
- changed `sceneScaleId` changes only the scene-scale identity.

## What this does not yet prove

This module does not yet prove:
- physical Honor/MotionCam execution;
- that a real phone DNG has completed the full phase-2 route;
- FULL_PHYSICAL camera/lens color calibration;
- dual-illuminant DNG color interpolation;
- cross-CPU bit-identical reconstruction output;
- a Lightroom-readable TruthRaw compatibility DNG;
- Android gallery/JPEG output from a finalized Scientific Preview.

Those are subsequent integration/output tasks. This module only removes the fixture/dummy-hash gap in the post-master authority chain.

## Next step after CI PASS

Wire the already-existing full-frame streaming reconstruction so each camera-native reconstructed tile contributes to Scientific Master Digest v0.1, derive/bind the actual TruthRange gauge and scene scale for that same lineage, then use this phase-2 bridge before releasing the previously designed Scientific Preview representation.

**Measured where measured. Reconstructed where necessary. Never invented.**
