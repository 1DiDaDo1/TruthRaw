# DNG Projection Export v0.1

Status: **RESEARCH IMPLEMENTATION — HOST/ANDROID VALIDATION IN PROGRESS**

## Purpose

Provide downstream file representations of an already-finalized TruthRaw single-frame lineage without changing evidence, Scientific Master identity, TruthRange zero-line, scene-scale, Technical Backplane, color authority, or frame/evidence counts.

The export layer is a representation/projection layer only.

## Output roles

### Linear DNG 16

- uncompressed classic TIFF/DNG container;
- `PhotometricInterpretation = LinearRaw`;
- 16-bit unsigned, 3 samples per pixel;
- payload is camera-native reconstructed RGB from the same reconstruction backend used to define the Scientific Master;
- samples outside `[0,1]` are explicitly counted and clamped for this bounded compatibility projection;
- source-bound `cameraToXyzD50` is represented as a synthetic compatibility profile (`ForwardMatrix1`) with its inverse as `ColorMatrix1`;
- this profile does **not** create independent camera calibration or `FULL_PHYSICAL` color authority.

This is the preferred first Lightroom interoperability candidate because it avoids deliberately re-mosaicing reconstructed RGB only for a downstream application to demosaic it again.

### CFA DNG 16

- uncompressed CFA DNG;
- original Bayer topology is preserved as topology metadata;
- payload is normalized Stage-2 CFA after source black/gain corrections used by TruthRaw;
- the resulting values are **not** the immutable original sensor-count evidence;
- role: `CFA_DNG_16_RECONSTRUCTED_PROJECTION`.

The CFA file must never be described as a newly measured physical RAW. It is a reconstructed/corrected projection.

### Scientific `.rawsensor` float32

- TruthRaw-private binary format, magic `TRRAWS01`;
- 128-byte fixed header;
- header binds source SHA-256, Scientific Master SHA-256, TruthRange `L0`, dimensions, orientation, CFA topology, claim scope, and frame/evidence counts;
- payload is interleaved little-endian float32 camera-native reconstructed RGB;
- no `[0,1]` compatibility clamp is performed;
- not a DNG and not intended as a Lightroom container.

## Scientific release gate

Export is permitted only after:

1. exact source SHA-256 sealing;
2. source-bound DNG color binding;
3. Scientific Master v0.2 + real TruthRange self-gauge;
4. Technical Backplane phase 2;
5. finalized source/master admission.

During the export pass the camera-native Scientific Master is reconstructed a second time tile-by-tile and hashed again. Export succeeds only when this digest exactly equals the already-finalized Scientific Master digest and the Backplane digest.

Therefore a changed reconstruction/backend/source cannot silently produce a file under an older Scientific Master identity.

## Memory and execution policy

- canonical export tile core: 64×64;
- DNG files are written in 64-row strips;
- no full-frame Scientific Master buffer is allocated;
- no full RAW is materialized by the export adapter;
- caller logical memory budget remains enforced;
- Android output uses a seekable/truncatable Storage Access Framework descriptor;
- a native export failure truncates the destination back to zero bytes.

This build remains CPU/NDK reference execution. Vulkan is not part of this module.

## Authority boundary

Hard invariant:

- measured Direct-CFA source evidence remains immutable;
- `physicalFrameCount = 1`;
- `independentEvidenceCount = 1`;
- source-metadata color remains source-bound;
- export creates no photons, no evidence, no independent calibration, and no stronger physical claim;
- Scientific Master remains separate from all compatibility projections.

Canonical rule:

**Measured where measured. Reconstructed where necessary. Never invented.**

## Validation status

The implementation includes host falsification tests for:

- LinearRaw DNG topology and 3-channel payload contract;
- CFA DNG topology/Bayer metadata;
- `.rawsensor` exact payload size/header role;
- exact Scientific Master digest re-verification;
- fail-closed rejection of a tampered Scientific Master identity.

Android CI additionally verifies the JNI symbol and arm64 APK build.

**Not yet proven until the corresponding CI/device evidence exists:**

- successful host/ASan/Android build for this exact head;
- successful physical export on HONOR BKQ-N49;
- import/display/edit behavior in Lightroom or other third-party DNG consumers;
- universal compatibility across DNG readers;
- `FULL_PHYSICAL` color.
