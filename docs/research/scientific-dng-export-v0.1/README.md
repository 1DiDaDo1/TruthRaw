# Scientific DNG Export v0.1

Status: **RESEARCH IMPLEMENTATION — VALIDATION IN PROGRESS**

## Purpose

Provide a bounded, standards-oriented DNG compatibility projection downstream of an already established TruthRaw Scientific Master lineage. This module does not create or replace evidence and does not change the Scientific Master.

Two projection roles are deliberately separate:

- `LINEAR_RAW_COMPATIBILITY_PROJECTION`: three-channel camera-native reconstructed RGB written as 16-bit DNG `LinearRaw`.
- `RECONSTRUCTED_CFA_PROJECTION`: one-channel 16-bit CFA obtained by selecting the source CFA component from the reconstructed camera-native RGB at each sensor site. This is remosaiced reconstruction, **not measured sensor evidence**.

## Authority boundary

The exporter requires a prepared, validated single-frame source/color lineage and rejects unverified or preview-sentinel color authority.

The exporter preserves:

- `physicalFrameCount = 1`;
- `independentEvidenceCount = 1`;
- source metadata color as source-bound unless an independent calibration was already supplied;
- the Scientific Master as a separate float32 camera-native scene representation.

The DNG output itself receives no new scientific authority. A successful DNG export does **not** imply `FULL_PHYSICAL` color.

## Scientific identity gate

Export is tied to an expected finalized Scientific Master digest.

Before writing any byte, v0.1 replays the canonical 64×64 camera-native reconstruction and recomputes the Scientific Master digest. If it differs from the expected digest, export fails before output begins.

During the actual write pass, the reconstruction is replayed and hashed again. The resulting digest must again equal the expected Scientific Master identity.

This deliberately costs additional computation in v0.1. Correct lineage is prioritized over export speed.

## Representation boundary

DNG v0.1 uses an unsigned 16-bit compatibility projection:

`projected = round(clamp(scientific_master_component, 0, 1) * 65535)`

Therefore negative and greater-than-one values that can exist in the Scientific Master are not preserved by this compatibility projection. The DNG must never be treated as the Scientific Master.

The module streams canonical tiles and does not materialize the full Scientific Master.

## DNG structure

Current writer scope:

- classic little-endian TIFF/DNG;
- uncompressed 64×64 tiles;
- DNGVersion 1.4.0.0;
- DNGBackwardVersion 1.2.0.0;
- 16-bit unsigned samples;
- `LinearRaw` or CFA PhotometricInterpretation according to projection role;
- orientation inherited from source metadata;
- source CFA pattern retained for the reconstructed-CFA projection;
- a derived single-D50 source-bound color profile embedded from the already resolved `cameraToXyzD50` transform.

The embedded profile is algebraically derived from the existing resolved source-bound transform. It is not a new camera calibration.

## Fail-closed tests

The standalone test requires:

- replayed Scientific Master digest equality;
- valid TIFF/DNG structural tags for both projection roles;
- `1/1` physical-frame/evidence invariants;
- no full Scientific Master materialization;
- source-bound color not promoted to independent physical color;
- a wrong expected master digest produces `SCIENTIFIC_IDENTITY_MISMATCH` before output bytes are written;
- preview-sentinel color authority is rejected before output.

## Not yet proven

Until CI and physical application testing are complete, this module does not claim:

- Lightroom/Adobe physical compatibility for a produced Honor file;
- Android export-path correctness;
- real-device export time/RSS/thermal behavior;
- byte-preservation of the unrestricted Scientific Master inside DNG;
- independent physical camera/lens color calibration.

Canonical rule:

**Measured where measured. Reconstructed where necessary. Never invented.**
