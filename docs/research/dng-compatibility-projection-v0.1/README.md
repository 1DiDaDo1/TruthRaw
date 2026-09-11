# DNG Compatibility Projection v0.1

Status: **RESEARCH CANDIDATE — HOST VALIDATION PENDING**

## Purpose

Add standards-oriented downstream DNG projections without changing TruthRaw evidence, Scientific Master identity, TruthRange zero-line, Technical Backplane, finalized preview authority, or appearance pixels.

The exporter is a **projection/output layer**. It is not a source of scientific truth.

Canonical boundary:

- original physical Direct-CFA source = measured evidence;
- Scientific Master = camera-native reconstructed RGB scientific state;
- Linear DNG = compatibility projection of that Scientific Master;
- measured-preserving CFA DNG = derived CFA projection of the measured-preserving camera-native reconstruction;
- neither generated DNG is a second physical capture or independent evidence root.

The project rule remains:

**Measured where measured. Reconstructed where necessary. Never invented.**

## Outputs

### 1. Linear Scientific Master DNG

Role:

`LINEAR_SCIENTIFIC_MASTER_PROJECTION`

The main image is written as uncompressed 32-bit IEEE-754 `LinearRaw`, three interleaved camera-native RGB samples per pixel, tiled on the same fixed 64×64 canonical grid used by the Scientific Master digest.

The output is taken **before camera-to-XYZ conversion, appearance, tone mapping, sRGB conversion, JPEG conversion, or display clipping**.

No remosaic step is used. This avoids throwing away the reconstructed RGB information merely so another RAW application can demosaic it again.

### 2. Measured-Preserving CFA DNG

Role:

`MEASURED_PRESERVING_CFA_PROJECTION`

The main image is uncompressed 32-bit IEEE-754 CFA. At each Bayer location the exporter selects only the reconstructed camera-RGB channel corresponding to the source CFA measurement at that site.

For a measured-preserving reconstruction backend, this is expected to retain the exact Stage-2 measured component at the CFA site. It is nevertheless a **derived projection**, because Stage-2 already includes source black normalization and any admitted source gain-field processing. It is not the original integer sensor code-value plane and must never be relabeled as such.

The v0.1 test matrix covers BGGR, RGGB, GRBG and GBRG.

## Scientific-Master binding

Every export independently rebuilds `ScientificMasterDigestAccumulator` while the 64×64 camera-native reconstruction tiles are being written.

The resulting digest must exactly equal the finalized Scientific Master hash supplied in `ProjectionMetadata.expectedScientificMasterHash`.

If it does not, the exporter returns:

`SCIENTIFIC_MASTER_MISMATCH`

The output destination must then be discarded by the caller. A future Android SAF integration therefore must delete/abandon a destination on any non-OK completion rather than present a partially written file as a valid export.

This check prevents a DNG projection from silently drifting away from the exact finalized scientific state that authorized it.

## Color authority

The generated DNG uses a resolved **source-bound compatibility profile**:

- pixel samples stay camera-native;
- `ColorMatrix1` is derived as the inverse of the already validated `cameraToXyzD50` binding;
- `CalibrationIlluminant1` is D50;
- the resolved source white is written as `AsShotWhiteXY`;
- the original source/color identity, expected Scientific Master hash, binding ID and authority are also recorded in `DNGPrivateData`.

This is not a new camera characterization. If input authority is `SOURCE_METADATA_BOUND`, output authority remains `SOURCE_METADATA_BOUND`.

The exporter refuses `UNVERIFIED` and `PREVIEW_SENTINEL` color authority.

`FULL_PHYSICAL` color is not created by this module.

## DNG container subset

v0.1 intentionally uses a small, auditable subset:

- classic little-endian TIFF/DNG;
- 64×64 tiles;
- uncompressed main image;
- 32-bit IEEE-754 samples;
- DNGVersion 1.4.0.0;
- DNGBackwardVersion 1.4.0.0 because the main image uses floating-point samples;
- LinearRaw (`PhotometricInterpretation=34892`) for RGB output;
- CFA (`PhotometricInterpretation=32803`) plus CFA pattern tags for the CFA output;
- orientation preserved from the bound source;
- a single resolved compatibility color profile rather than pretending the generated file contains a new independent dual-illuminant calibration.

The file is written sequentially. Tile offsets and byte counts are calculated before output starts, so Android can later write directly to a Storage Access Framework file descriptor without seeking or buffering a full frame.

The classic TIFF v0.1 writer fails closed if the generated file would exceed 32-bit TIFF offset space. BigTIFF/DNG is outside this first compatibility subset.

## Resource boundary

No full-frame RGB or CFA projection buffer is owned by the exporter.

Resident state consists of:

- the existing bounded source;
- one Stage-2/reconstruction workspace;
- one 64×64 output tile;
- the DNG header/tile-offset arrays;
- the Scientific Master digest accumulator;
- the caller-provided sequential sink.

A sink is free to be a file descriptor with tiny resident state. Host tests use an in-memory sink only to inspect generated bytes.

## Private provenance

`DNGPrivateData` begins with the required null-terminated identifier `TruthRaw` and records, in byte-order-independent text:

- projection schema/version;
- projection role;
- exact source SHA-256;
- exact expected Scientific Master SHA-256;
- source evidence ID;
- color binding ID;
- color authority;
- physical frame count = 1;
- independent evidence count = 1;
- `projection_is_evidence=false`;
- `full_physical_color_promoted=false`;
- `scientific_master_modified=false`.

This metadata supplements the normal DNG tags; it does not grant authority.

## Validation plan

Host validation must prove at minimum:

1. valid little-endian TIFF/DNG structure;
2. float LinearRaw tag set;
3. exact float32 camera-native Scientific Master pixels in the Linear DNG;
4. exact measured-preserving CFA component at every real pixel;
5. correct CFA tags for all four supported Bayer phases;
6. exact Scientific Master hash match;
7. deliberate expected-hash mutation is rejected;
8. color authority cannot be promoted by export;
9. no full-frame projection buffer is materialized;
10. GCC, Clang and ASan/UBSan all pass.

Only after this host proof is green may the module be connected to Android `ACTION_CREATE_DOCUMENT` / file-descriptor export.

## Compatibility claim boundary

A structurally valid DNG does **not** yet prove Lightroom compatibility. Lightroom/Camera Raw behavior will be treated as empirical interoperability evidence only after a file generated by the physical Honor route is actually imported and inspected.

Likewise, this module does not yet define a separate historical `.rawsensor` byte format because no current-branch canonical `.rawsensor` contract was found. The CFA DNG provides the first standardized rawsensor-like projection while that internal-format question remains explicit.

## Adobe DNG notice

This product includes DNG technology under license by Adobe.

The implementation uses the published DNG format semantics; no Adobe SDK source code is copied into this module.
