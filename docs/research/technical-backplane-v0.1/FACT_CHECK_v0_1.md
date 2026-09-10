# Fact check — Technical Backplane v0.1

## Supported by implementation/tests

- The wire format is fixed at exactly 180 bytes.
- Serialization uses explicit little-endian integer encoding; it is not a raw C++ struct dump and does not depend on compiler padding.
- Four distinct 32-byte fields bind source evidence, scientific master, zero-line and scene-scale identity.
- The layout contains exactly one zero-line field. The deterministic fixture also confirms its synthetic zero-line byte sequence occurs once in the serialized record.
- One-byte mutation in a bound hash is detected by CRC32.
- Non-zero reserved bytes are rejected even when CRC32 is recomputed, so reserved-space semantics fail closed independently from the checksum.
- `physicalFrameCount != 1` or `independentEvidenceCount != 1` is rejected.
- Scientific-master mutation, zero-line mutation, appearance-as-evidence and counterfactual-as-evidence flags are rejected.
- Unknown room/claim status values are rejected.
- Empty zero-line/source/master/scene-scale bindings are rejected.
- GCC Release, Clang Release and Clang ASan/UBSan produce identical metric output locally.

## Deliberate non-claims

- CRC32 is not cryptographic authentication.
- A 32-byte binding is only meaningful if an upstream Archivist actually computed/verified it against the intended object.
- This module does not define physical radiometric calibration.
- It does not change the TruthRange zero-line value; it binds the chosen zero-line identity.
- It is not extra sensor evidence.
- It does not embed itself into DNG, TIFF, JPEG, HEIF or another container yet.
- It does not contain per-pixel uncertainty, geometry, appearance or image payload.

## Architectural consequence

The renewed house can carry one tiny immutable technical backside through corridor handles while all megapixel-scale data remain in persistent artifacts or tile streams. The backplane size is independent of image megapixel count.
