# Scientific Master Linear DNG Projection v0.1

Status: **VALIDATED RESEARCH IMPLEMENTATION — SYNTHETIC WRITER; STREAMING MASTER ADAPTER UNDER CI VALIDATION**

This module creates a **representation-only DNG compatibility projection** from the already reconstructed, camera-native Scientific Master domain. It does not create a second truth source and it does not alter Direct-CFA evidence, the Scientific Master, the Technical Backplane, frame count, evidence count, or color authority.

TruthRaw law remains unchanged:

> Measured where measured. Reconstructed where necessary. Never invented.

and:

> Representation can exceed the source. Knowledge claims cannot exceed the evidence.

## Role in the architecture

Input authority:

`sealed source lineage -> admitted Scientific Master identity -> authorized cameraToXyzD50`

Output role:

`LINEAR_DNG_XYZ_D50_COMPATIBILITY_PROJECTION`

The output is neither Direct-CFA evidence nor the Scientific Master. It is a standards-oriented interoperability projection intended for external raw software.

## Projection model

The Scientific Master contract is camera-native reconstructed RGB before `camera_to_xyz()` and before appearance. The exporter reads it in the same canonical 64x64 tile order used by the Scientific Master digest, recomputes the exact master digest, transforms each RGB triplet through the already-authorized `cameraToXyzD50`, and stores the resulting XYZ-D50 triplet as three-channel float LinearRaw data.

The DNG/TIFF representation currently uses:

- classic little-endian TIFF;
- DNG 1.4 version tags;
- `PhotometricInterpretation = 34892` (`LinearRaw`);
- `BitsPerSample = 32,32,32`;
- `SampleFormat = 3,3,3` (IEEE floating point);
- `SamplesPerPixel = 3`;
- uncompressed 64x64 tiles;
- synthetic XYZ-D50 raw color coordinates;
- identity `ColorMatrix1` because the stored raw coordinate itself is XYZ-D50;
- D50 `CalibrationIlluminant1`;
- D50 `AsShotNeutral`;
- `DNGPrivateData` carrying TruthRaw role and lineage identifiers.

No tone curve, display transfer function, gamut mapping, appearance profile, highlight invention, or clipping to `[0,1]` is performed before storage. Negative and greater-than-one components are counted in the result instead of silently changing them.

## Transactional master-identity gate

The byte sink is transactional. DNG bytes may be staged while the master digest is recomputed, but the artifact is released only when the recomputed camera-native Scientific Master SHA-256 is exactly equal to the admitted `scientificMasterSha256`.

On mismatch the sink is aborted and no committed artifact may remain.

This is important because the DNG is downstream of the Scientific Master: export is allowed to *represent* a master, never to silently substitute a different one.

## Streaming Scientific Master adapter

`StreamingScientificMasterTileSource` is the bounded bridge between the existing scientific reconstruction path and the DNG writer. It deliberately reuses the same `fill_stage2()` implementation, the same `IReconstructionBackend`, the same reconstruction halo, the same CFA metadata, and the same canonical 64x64 tile schedule used by Scientific Master Streaming Binding v0.2.

The adapter:

- accepts an existing `IRawTileSource` plus reconstruction backend;
- reconstructs one canonical camera-native RGB tile at a time;
- reuses its Stage-2/reconstruction workspace across calls;
- performs no camera-to-XYZ transform itself;
- performs no appearance processing;
- rejects non-canonical tile origins and incorrect trailing tile extents;
- does not materialize a full-frame Scientific Master.

Its dedicated test computes the canonical Scientific Master identity through the established v0.2 binding, independently replays every canonical tile through the adapter, and requires the resulting digest to be exactly equal. This adapter test is not promoted to validated status until the full compiler/sanitizer matrix is green.

## Authority invariants tested

The writer test suite requires:

- exact Scientific Master digest verification before commit;
- transactional abort on a deliberately wrong master hash;
- deterministic projection bytes for repeated identical inputs in one build;
- rejection of a singular color transform;
- `representationOnly == true`;
- `scientificMasterModified == false`;
- `appearanceApplied == false`;
- `counterfactualObservationCreated == false`;
- `physicalFrameCount == 1`;
- `independentEvidenceCount == 1`;
- bounded master tile request of at most `64*64*3` float samples;
- TIFF/DNG structural tags and first projected float samples.

The synthetic master fixture deliberately uses binary32-exact values with power-of-two denominators. This prevents the fixture itself from changing bits under compiler FMA/devirtualization choices; the production Scientific Master digest gate remains bit-exact and is not relaxed.

## Validated writer CI evidence

Validated writer implementation SHA:

`5a86bfcb530900aa5eaadb3aabef74b39a4045e8`

Workflow:

`Scientific Master Linear DNG Projection v0.1`

Successful writer run:

`34660013745`

All three required jobs succeeded:

- GCC Release;
- Clang Release;
- Clang ASan/UBSan.

Documentation Governance on the same implementation SHA also succeeded (run `34660013744`).

## Negative evidence retained

Failures are not erased or relabeled:

- run `34659618950`: GCC Release and Clang Release failed while ASan/UBSan passed; investigation exposed a release-only TIFF metadata cardinality problem path;
- run `34659777690`: same release symptom remained after an initial test-side numerical correction, proving the first hypothesis was insufficient;
- run `34659924210`: GCC Release and ASan/UBSan passed, but Clang Release correctly rejected the synthetic Scientific Master identity; this exposed compiler-sensitive synthetic fixture arithmetic;
- run `34660262524` at SHA `749116828534906cbbc46a69db0ebdbeaa46a895`: all three builds failed because the first adapter revision referred to `streaming_v0_1::TileRect` instead of canonical `truthraw::TileRect`, and the reused test fixture exposed two unused static helpers under `-Werror`;
- run `34660391357` at SHA `2598c6dc15fb979ecf496243756f6f482d8e992f`: the `TileRect` namespace error was fixed; builds then failed only on those two unused shared test helpers;
- run `34660405843` at SHA `32488ffecabd93fa4ae93d6262a9b4e5c3e8d2b0`, attempts 1 and 2: GitHub created all three job records but returned no executable steps or logs. These attempts are retained as CI infrastructure-start failures and are not interpreted as scientific or code validation results.

The TIFF defect was traced to reading `vector.size()` in the same call in which that vector was moved. Argument evaluation order allowed Release builds to observe a moved-from vector and emit a zero TIFF count. The serializer now captures the count before the move. The later Clang-only identity mismatch was fixed by making the synthetic fixture itself bit-stable, without weakening the production digest comparison.

## What is *not* proven yet

This module does **not** yet prove:

- ingestion by Adobe Lightroom / Lightroom Classic / Camera Raw;
- ingestion by Adobe DNG SDK validation tools;
- compatibility with every DNG reader;
- real Honor Magic 8 Pro / MotionCam end-to-end DNG output;
- Android filesystem transaction/atomic-rename behavior;
- on-device memory, latency, thermal, or storage behavior;
- that a cross-build or cross-architecture reconstruction produces an identical Scientific Master digest when the reconstruction backend itself does not make that stronger promise;
- sealed-source export finalization from the original random-access byte object.

Until Adobe ingestion is actually tested, the correct description is **standards-oriented Linear DNG compatibility projection**, not “Lightroom-proven DNG”.

## Next integration step after adapter validation

Once the adapter itself passes the full matrix, the next authority layer must own the direct-native export finalization route. It must receive the original sealed random-access byte source, reverify the source SHA-256, construct `TileNativeDngSource` from exactly that byte object and the already-prepared options, establish/verify Scientific Master Binding v0.2, bind the authorized color record without upgrading its authority, and only then invoke the transactional DNG writer.

That route must not accept a loose `bool` such as `isFinalized=true`, and it must not trust an arbitrary caller-supplied `IRawTileSource` to stand in for the sealed source lineage.
