# Scientific Preview Source Binding v0.1

Status: RESEARCH CANDIDATE — CI proof pending

Base: Reconstructed Color Preview v0.1 @ `372ffe1565e10070db74586298ba82042500af81`.

## Purpose

Close the authority gap between an Android-selected RAW and the already validated bounded reconstructed-color preview.

The old Android CFA preview intentionally opens `TileNativeDngSource` with a parser-only sentinel source ID and identity color matrix. That route remains `SOURCE_PROXY` only and MUST NOT enter the Main House color pipeline.

Scientific color admission now requires all of the following before a `TileNativeDngSource::OpenOptions` color binding is produced:

1. SHA-256 is computed over the exact source bytes through `IRandomAccessByteSource` in bounded chunks.
2. Source identity is canonical `sha256:<64 lowercase hex>`.
3. The color record is explicitly validated and has an admitted authority class.
4. The color record is bound to the exact same source SHA-256.
5. The 3x3 camera-to-XYZ D50 matrix is finite and has a non-empty binding ID.
6. The immutable Technical Backplane is valid and its `sourceEvidenceHash` exactly equals the source SHA-256.
7. `physicalFrameCount == 1` and `independentEvidenceCount == 1` remain unchanged.

Only after all gates pass is the existing TileNative `OpenOptions` object populated.

## Authority classes

- `UNVERIFIED` — blocked.
- `PREVIEW_SENTINEL` — blocked from scientific color, even if the numerical matrix is identity or otherwise plausible.
- `SOURCE_METADATA_BOUND` — may produce a source-bound color preview. It is not absolute color truth.
- `GATEHOUSE_CERTIFIED_METADATA` — external professional RAW metadata that survived the Gatehouse/handoff authority path; source-bound preview only unless stronger calibration exists.
- `INDEPENDENT_CALIBRATION` — independently calibrated color-preview authority. This still does not by itself establish TruthRaw `FULL_PHYSICAL`; lens/illuminant/spectral and other calibration requirements remain separate.

The authority enum is an internal governance classification, not a cryptographic signature. Android/UI code MUST NOT be allowed to submit or self-assign it. A production producer for source-metadata color binding remains a separate module/proof.

## Source seal

`seal_source_sha256()` hashes the source incrementally. Default I/O chunk is 64 KiB and is capped at 64 KiB. Source megapixel count therefore changes I/O time, not hash workspace size.

The seal carries:
- 32-byte SHA-256 digest;
- source byte length;
- canonical `sourceEvidenceId` string;
- bounded hash workspace metric.

`reverify_source_sha256()` can re-read the same byte source immediately before Main-House admission. This detects a changed source/provider between seal and use. It costs an additional sequential read but no full-file buffer.

v0.1 does not claim that every Android document provider is immutable. A mutable provider/TOCTOU-resistant production policy remains open; fail-closed re-verification is the current safe research path.

## Backplane rule

Technical Backplane v0.1 remains byte-for-byte unchanged. This module does not add fields to the 180-byte record. It only requires the existing `sourceEvidenceHash[32]` to equal the newly computed source SHA-256.

The preview layer may not initialize or rewrite Scientific Master, zero-line, scene-scale, frame count or evidence count.

## Color semantics

This module deliberately does NOT invent a camera matrix and does not parse a full DNG color model itself. It governs an already-produced color binding.

For the native DNG path, a later governed producer may derive a source-bound transform from the actual DNG metadata chain (for example AsShotNeutral / CameraCalibration / ForwardMatrix where valid and supported). That producer must receive its own validation and uncertainty limits.

Until such a producer exists for a selected real file, Android must remain on the existing grayscale `SOURCE_PROXY` fallback rather than substitute the identity matrix.

For a Gatehouse professional RAW, the equivalent color record must come from the certified handoff metadata path and remain bound to the original source evidence identity.

## Tests

v0.1 tests require:
- standard SHA-256 `abc` vector;
- canonical lowercase evidence ID;
- source-bound record admission;
- independent-calibration admission with a distinct claim scope;
- Gatehouse-certified metadata admission;
- preview sentinel rejection;
- unvalidated binding rejection;
- source mismatch rejection;
- non-finite matrix rejection;
- Backplane source mismatch rejection;
- frame/evidence invariant rejection;
- tampered source rejection on re-verification;
- bounded hash workspace.

## Explicit non-claims

This candidate does not yet prove:
- a production DNG color-binding producer;
- Honor-specific independent color/lens calibration;
- physical-device source hashing latency/thermal cost;
- cryptographic immutability of arbitrary Android document providers;
- `FULL_PHYSICAL` color;
- that the selected real Android RAW can already show the reconstructed-color preview.

The next integration proof is:

```text
Android URI/PFD
 -> bounded SHA-256 source seal
 -> authorized internal color record
 -> Backplane source-hash match
 -> TileNativeDngSource
 -> StreamingTruthRawProcessor v4.7i
 -> BoundedSrgbPreviewSink
 -> ARGB_8888 / JPEG sRGB
```
