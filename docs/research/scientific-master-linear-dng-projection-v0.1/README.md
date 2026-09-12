# Scientific Master Linear DNG Projection v0.1

Status: **VALIDATED RESEARCH IMPLEMENTATION — SYNTHETIC STRUCTURE / IDENTITY GATE**

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

## Authority invariants tested

The test suite requires:

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

## Validated CI evidence

Validated implementation SHA before this documentation commit:

`5a86bfcb530900aa5eaadb3aabef74b39a4045e8`

Workflow:

`Scientific Master Linear DNG Projection v0.1`

Successful run:

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
- run `34659924210`: GCC Release and ASan/UBSan passed, but Clang Release correctly rejected the synthetic Scientific Master identity; this exposed compiler-sensitive synthetic fixture arithmetic.

The TIFF defect was traced to reading `vector.size()` in the same call in which that vector was moved. Argument evaluation order allowed Release builds to observe a moved-from vector and emit a zero TIFF count. The serializer now captures the count before the move. The later Clang-only identity mismatch was fixed by making the synthetic fixture itself bit-stable, without weakening the production digest comparison.

## What is *not* proven yet

This module does **not** yet prove:

- ingestion by Adobe Lightroom / Lightroom Classic / Camera Raw;
- ingestion by Adobe DNG SDK validation tools;
- compatibility with every DNG reader;
- real Honor Magic 8 Pro / MotionCam end-to-end DNG output;
- Android filesystem transaction/atomic-rename behavior;
- on-device memory, latency, thermal, or storage behavior;
- that a cross-build or cross-architecture reconstruction produces an identical Scientific Master digest when the reconstruction backend itself does not make that stronger promise.

Until Adobe ingestion is actually tested, the correct description is **standards-oriented Linear DNG compatibility projection**, not “Lightroom-proven DNG”.

## Next integration step

The next module must supply the `IScientificMasterTileSource` from the real sealed-source reconstruction route: the same TileNative source lineage and reconstruction backend used to establish the admitted Scientific Master. It must not materialize a second full-frame master merely for export. A filesystem transaction sink can then stage to a temporary file and atomically publish only after the master-identity gate succeeds.
