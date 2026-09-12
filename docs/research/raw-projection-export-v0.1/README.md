# Raw Projection Export v0.1

Status: **IMPLEMENTED RESEARCH ROUTE — VALIDATION PENDING GITHUB RUNNER AVAILABILITY**

## Purpose

Provide three explicit downstream representations from the already-finalized single-frame TruthRaw lineage without changing source evidence, Scientific Master identity, TruthRange zero-line, Technical Backplane, color authority, or physical-frame/evidence counts.

The three export roles are deliberately distinct:

1. `RECONSTRUCTED_CFA_RAWSENSOR_PROJECTION_V0_1`
   - headerless little-endian unsigned 16-bit CFA samples;
   - reconstructed/remosaiced from camera-native reconstructed RGB;
   - **not measured sensor evidence**.
2. `RECONSTRUCTED_CFA_DNG_PROJECTION_V0_1`
   - classic TIFF/DNG, uncompressed unsigned 16-bit CFA;
   - source CFA topology, `BlackLevel=0`, `WhiteLevel=65535`;
   - reconstructed/remosaiced compatibility projection;
   - **not measured sensor evidence**.
3. `LINEAR_DNG_COMPATIBILITY_PROJECTION_V0_1`
   - classic TIFF/DNG `LinearRaw`, uncompressed interleaved unsigned 16-bit camera RGB;
   - avoids a deliberate RGB→CFA→RGB round trip for RAW applications that accept LinearRaw;
   - compatibility projection, **not the Scientific Master**.

## Authority boundary

The exporter is downstream only.

It must never:

- mutate the sealed source;
- replace or redefine the Scientific Master;
- choose a new TruthRange gauge or zero-line;
- create a second evidence root;
- promote source-metadata-bound color to independent physical calibration;
- label reconstructed CFA samples as physically measured photosite samples.

Every export keeps:

- `physicalFrameCount = 1`;
- `independentEvidenceCount = 1`;
- `sourcePixelsClaimedMeasured = false`;
- `projectionOnly = true`;
- `fullScientificMasterMaterialized = false`.

Canonical rule:

**Measured where measured. Reconstructed where necessary. Never invented.**

## Finalized Android gate

Android export does not trust the existence of a visible preview alone. Before opening the export writer it repeats the exact-source authority chain:

`source SHA-256 seal → DNG color producer v0.2 → prepared source → source reverification → TileNative source → Scientific Master streaming v0.2 → TruthRange self-gauge → Technical Backplane phase 2 → finalized admission → source reverification → projection writer → source reverification`

If any gate fails, export is fail-closed.

## Streaming / memory contract

The writer uses 32 output rows per strip and reconstructs only the current strip plus reconstruction halo. It does not allocate a full-frame Scientific Master.

For DNG output the TIFF/DNG directory is small and resident, while pixel strips are written sequentially. A caller-supplied logical resident ceiling is enforced before strip write.

Representation quantization is explicit:

- camera-native reconstructed samples are rounded to unsigned 16-bit;
- values below `0` are clipped only in the export representation and counted as `clippedLowSamples`;
- values above `1` are clipped only in the export representation and counted as `clippedHighSamples`;
- non-finite samples fail closed;
- clipping never feeds back into Scientific Master/evidence.

Therefore these 16-bit exports are bounded compatibility representations. They are not substitutes for the unrestricted internal Scientific Master / TruthRange domain.

## DNG v0.1 representation

The writer emits little-endian classic TIFF/DNG with uncompressed strips and explicit DNG role metadata. Relevant tags include:

- `ImageWidth`, `ImageLength`;
- `BitsPerSample=16`;
- `Compression=1`;
- `PhotometricInterpretation=CFA` or `LinearRaw`;
- `StripOffsets`, `RowsPerStrip`, `StripByteCounts`;
- `SamplesPerPixel`, `PlanarConfiguration`, `SampleFormat`;
- `DNGVersion=1.4.0.0`, `DNGBackwardVersion=1.1.0.0`;
- projection-specific `UniqueCameraModel` and `ImageDescription`;
- CFA tags for the CFA DNG;
- `ColorMatrix1`, `ForwardMatrix1`, `AsShotNeutral`, `CalibrationIlluminant1=D50`;
- bounded `BlackLevel` / `WhiteLevel` matching the encoded projection domain.

The exported color metadata is derived from the already source-bound `cameraToXyzD50` used by the finalized lineage. This does **not** create independent camera/lens calibration or a `FULL_PHYSICAL` color claim.

## Falsification tests

`tests/test_raw_projection_export_v0_1.cpp` requires:

- `.rawsensor` byte length equals `width × height × 2`;
- CFA DNG reports CFA photometric interpretation, 1 sample/pixel and required CFA/DNG/color tags;
- Linear DNG reports LinearRaw photometric interpretation, 3 samples/pixel and required DNG/color tags;
- CFA DNG pixel payload is byte-identical to the `.rawsensor` projection generated from the same reconstruction;
- Scientific Master hash is exactly unchanged before/after all export operations;
- TruthRange `L0` is bit-identical before/after export;
- gauge ID and scene-scale ID are unchanged;
- an intentionally impossible memory budget fails closed.

CI is configured for GCC Release, Clang Release, Clang ASan/UBSan and an Android arm64 APK build.

## Android UI

App version: `0.5-raw-dng-export` / version code `5`.

After a finalized Scientific Preview reaches `Ready`, Android exposes:

- **Reconstructed CFA .rawsensor opslaan**;
- **Reconstructed CFA DNG opslaan**;
- **Linear DNG opslaan**.

Each action uses Android `ACTION_CREATE_DOCUMENT`; the user selects the destination. The heavy native finalization/export runs on a worker thread after the destination is granted.

## Validation state

GitHub Actions runs `34679341516` and `34679429499` failed before checkout with `runner_id=0` and `steps=[]`. The same condition was independently observed on the existing unchanged Documentation Governance workflow on this branch. Switching the new workflow from `ubuntu-24.04` to `ubuntu-latest` did not change that behavior.

Consequently those runs are recorded as **runner-execution failures, not code/test failures**. No green implementation claim or APK artifact is made from them.

The implementation is complete in the branch, but the following remain pending until GitHub assigns a runner and executes the configured tests:

- host compile/test confirmation;
- ASan/UBSan confirmation;
- Android NDK/JNI/Kotlin link/build confirmation;
- APK hash/artifact identity;
- physical Honor export;
- Lightroom/other RAW-application acceptance of the generated DNG files.

Physical application compatibility must be established using exported files; it is not inferred merely from tag construction.
