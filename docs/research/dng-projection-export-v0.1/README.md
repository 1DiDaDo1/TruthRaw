# DNG Projection Export v0.1

Status: **RESEARCH IMPLEMENTATION COMPLETE — EXECUTION VALIDATION BLOCKED BEFORE HOSTED-RUNNER ASSIGNMENT**

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

### CFA DNG 16 — rawsensor projection

- uncompressed CFA DNG;
- original Bayer topology is preserved as topology metadata;
- payload is normalized Stage-2 CFA after the source black/gain corrections used by TruthRaw;
- the resulting values are **not** the immutable original sensor-count evidence;
- role: `CFA_DNG_16_RECONSTRUCTED_PROJECTION`;
- this is the DNG/rawsensor-style output: a standards container around the TruthRaw sensor-grid projection.

The CFA file must never be described as a newly measured physical RAW. It is a reconstructed/corrected projection.

### Scientific Master `.trmaster` float32

- TruthRaw-private binary format; the v0.1 internal serialization magic is `TRRAWS01` for historical implementation compatibility, but the canonical user-facing suffix is `.trmaster`;
- 128-byte fixed header;
- header binds source SHA-256, Scientific Master SHA-256, TruthRange `L0`, dimensions, orientation, source CFA topology, claim scope, and frame/evidence counts;
- payload is interleaved little-endian float32 camera-native reconstructed RGB;
- no `[0,1]` compatibility clamp is performed;
- this is **not** called `.rawsensor`, because it is 3-channel Scientific Master RGB rather than a CFA/sensor-grid payload;
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

## Implementation state

The repository implementation now contains:

- bounded Linear DNG writer;
- bounded reconstructed-CFA/rawsensor DNG writer;
- exact float32 `.trmaster` Scientific Master transport;
- host falsification tests for all three roles and digest tamper rejection;
- Android JNI bridge that re-seals/re-verifies the source, re-runs finalized phase-2 admission, exports through the Storage Access Framework descriptor, and truncates failed output to zero bytes;
- dedicated Android export UI with separate `Linear DNG`, `CFA DNG (rawsensor-projectie)`, and `Scientific Master .trmaster` actions;
- arm64-only Android build wiring and explicit no-Vulkan execution contract.

## Validation status

The intended executable validation covers:

- LinearRaw DNG topology and 3-channel payload contract;
- CFA DNG topology/Bayer metadata;
- `.trmaster` exact payload size/header role;
- exact Scientific Master digest re-verification;
- fail-closed rejection of a tampered Scientific Master identity;
- GCC and Clang release builds;
- Clang ASan/UBSan;
- Android NDK/JNI symbol linkage and arm64 APK assembly.

As of 2026-09-12, the GitHub-hosted validation is blocked **before runner assignment**, not by an observed code/test failure. Linux run `34677365672` and macOS rescue run `34677499457` both produced jobs with no executed steps and `runner_id = 0`. Earlier DNG-export run `34663000063` shows the same condition and produced no artifact.

See `VALIDATION_BLOCKER_2026-09-12.md` for the exact evidence and recovery condition.

**Therefore still unproven:**

- successful host compile/test/ASan execution for this exact export head;
- successful Android NDK/JNI/APK execution for this exact export head;
- successful physical export on HONOR BKQ-N49;
- import/display/edit behavior in Lightroom or other third-party DNG consumers;
- universal compatibility across DNG readers;
- `FULL_PHYSICAL` color.

No APK is labeled validated until a runner actually executes and passes the build. The hosted-runner infrastructure failures are retained as negative infrastructure evidence and are not rewritten as TruthRaw research failures.
