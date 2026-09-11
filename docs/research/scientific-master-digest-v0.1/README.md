# Scientific Master Digest v0.1

Status: **RESEARCH IMPLEMENTATION CANDIDATE — CI PROOF REQUIRED BEFORE PROMOTION**

Parent integration head:
`2fdf05ca1bbbc59cd8867df0cae117d1eec92d51`

## Why this module exists

Scientific Preview Source Binding v0.2 already proves the two-phase admission model, but its phase-2 tests can only use fixture hashes until a real deterministic Scientific Master identity exists. This module supplies that missing identity contract without modifying frozen/canonical v4.7i reconstruction bytes.

The bound scientific state is the **camera-native reconstructed scene-linear RGB** produced by v4.7i reconstruction **before** `camera_to_xyz()` and before any appearance, tone, output-acutance, preview or export transform. In the current Full-Frame Streaming v0.1 pass-2 implementation this state exists as the reconstructed `w.cam` tile immediately after `reconstruction.run(...)`.

This module does **not** define a new reconstruction algorithm. It only defines how the exact resulting Scientific Master content is identified.

## Canonical identity contract

A Scientific Master is bound as exact IEEE-754 binary32 camera-native RGB sample bits.

Constants:
- version: `1`
- channels: `3`
- channel order: camera-native `R,G,B`
- sample encoding: IEEE-754 binary32, serialized little-endian
- canonical digest-cell edge: `64` pixels
- master scientific role: `V47I_RECONSTRUCTED_CAMERA_SCENE`
- raster semantics inside every cell: row-major, interleaved RGB

NaN and infinity fail closed. Signed zero is intentionally bit-significant. v0.1 is a **content identity**, not a tolerance-based equivalence test.

### Cell serialization

Every canonical cell is serialized as:

`TRSMCL01 | version | headerBytes=32 | x | y | w | h | channels | sampleEncoding | reserved=0 | exact RGB float bits`

The cell digest is SHA-256 of those bytes.

### Master serialization

The root identity stream is:

`TRSMDG01 | version | headerBytes=48 | width | height | channels | sampleEncoding | channelOrder | cellEdge | scientificRole | flags=0 | cellColumns | cellRows | cellCount | reserved=0 | cell SHA-256 values in canonical row-major cell order`

The Scientific Master digest is SHA-256 of that root stream.

This is a deterministic hierarchical serialization. It avoids materializing the megapixel master or a multi-gigabyte canonical byte stream merely to calculate identity.

## Why fixed 64x64 cells

Runtime resource policy may legitimately use different tile sizes on low/mid/high devices. Scientific identity must not therefore depend on whether Full-Frame Streaming runs 64, 128, 256 or 512 pixel core tiles.

`ScientificMasterDigestAccumulator::add_tile()` accepts tiles made from the fixed 64x64 canonical grid, splits larger runtime tiles into canonical cells, stores only one 32-byte leaf digest plus one seen byte per cell, and finalizes in canonical spatial order.

Consequences:
- runtime tile order does not alter identity;
- runtime tile size does not alter identity when it respects the canonical-cell boundaries;
- duplicate cells fail closed;
- missing cells fail closed;
- a one-bit sample change changes identity;
- zero-line, scene-scale, source evidence and appearance are not folded into this digest.

For a `16320 x 12288` (~200.54 MP) master:
- cell columns: `255`
- cell rows: `192`
- cells: `48,960`
- digest bookkeeping remains below `2 MiB` in the v0.1 implementation.

This is consistent with the TruthRaw rule that resource capacity changes execution, never truth authority.

## Relationship to the Technical Backplane

Technical Backplane v0.1 already has four distinct identity bindings:
- source evidence SHA-256;
- Scientific Master SHA-256;
- zero-line SHA-256;
- scene-scale SHA-256.

This module produces **only** the Scientific Master binding. It deliberately does not merge the zero-line or scene-scale into the Scientific Master digest. Those concepts remain separately authoritative and independently mutation-protected by the Backplane.

The next integration step after this module passes CI is:
1. feed actual reconstructed camera-native tiles into this accumulator from the streaming scientific route;
2. finalize the real Scientific Master SHA-256;
3. populate Technical Backplane phase 2 with that digest plus real zero-line and scene-scale bindings;
4. run the existing Scientific Preview Source Binding v0.2 finalization gate with no fixture hashes.

## Validation gates

The native test suite requires:
- an independently calculated known SHA-256 vector to match;
- identical digest for 64/128/256 runtime tile partitions;
- identical digest for reversed runtime tile traversal order;
- one-float-bit mutation changes digest;
- `+0.0` and `-0.0` remain distinct identities;
- incomplete coverage blocks finalization;
- duplicate cells fail closed;
- NaN fails closed;
- non-canonical tile origin fails closed;
- 200MP-class digest bookkeeping remains below 2 MiB.

CI must pass GCC Release, Clang Release and Clang ASan/UBSan before this module may be called validated.

## Non-claims

v0.1 does **not** prove:
- physical Honor/MotionCam execution;
- cross-CPU or cross-compiler bit-identical v4.7i floating-point reconstruction;
- deterministic Scientific Master serialization across different reconstruction algorithms or algorithm versions;
- independent camera/lens physical color calibration;
- FULL_PHYSICAL color;
- a completed Technical Backplane phase 2;
- finalized Scientific Preview authority;
- any new photons, frames or independent evidence.

If two executions produce different Scientific Master float bits, their digest should differ. Whether such a difference is scientifically acceptable is a separate determinism/equivalence question and must not be hidden by a tolerant hash.

**Measured where measured. Reconstructed where necessary. Never invented.**
