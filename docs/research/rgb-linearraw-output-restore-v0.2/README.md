# TruthRaw RGB / LinearRaw output restore v0.2

Status: **IMPLEMENTATION IN PROGRESS — DO NOT PROMOTE UNTIL HOST + ANDROID + ADOBE/READER VALIDATION PASS**

## Why this module exists

The 12 September raw-projection branch correctly restored the downstream export *roles* and finalized Scientific-Master/Backplane admission, but it did not restore the older proven RGB/LinearRaw compatibility engineering. In particular, the v0.1 writer used a single raw IFD, clipped reconstructed RGB directly to `[0,1]`, had no finite headroom/BaselineExposure compensation, and embedded no display preview.

Earlier TruthRaw work had already established a better output architecture:

- the primary high-fidelity RAW projection is camera-native reconstructed RGB / DNG LinearRaw;
- reconstructed CFA is an optional compatibility projection and must not replace the RGB master projection;
- direct measured CFA evidence and reconstructed CFA are different products and must never share an authority label;
- the unrestricted signed/overrange Scientific Master remains authoritative over every finite DNG representation;
- a DNG compatibility file may include a display preview without changing its scientific RGB payload.

Canonical laws remain unchanged:

> **Measured where measured. Reconstructed where necessary. Never invented.**

> **Representation can exceed the source. Knowledge claims cannot exceed the evidence.**

## Correct output hierarchy

1. **Sealed Direct CFA evidence**
   - physically measured source samples / source lineage;
   - never rewritten by export;
   - if a sample-exact Direct-CFA rewrap or rawsensor mirror is ever exposed, it must be labelled measured-source rewrap, not reconstructed CFA.

2. **Scientific Master**
   - camera-native reconstructed scene-linear RGB float32;
   - before `camera_to_xyz()` and before appearance;
   - signed and capable of scene values above 1;
   - not a display image and not itself a DNG container.

3. **TruthRaw RGB RAW / LinearRaw DNG** — primary high-fidelity RAW projection
   - 3-channel camera-native RGB;
   - no RGB -> CFA -> RGB round trip;
   - finite 16-bit DNG compatibility representation;
   - white balance / source-bound color remains metadata-driven and editable downstream;
   - no Soft Natural / presentation tone curve baked into the scientific RGB payload;
   - role: `LINEAR_DNG_COMPATIBILITY_PROJECTION`, downstream only.

4. **Reconstructed CFA DNG / reconstructed CFA rawsensor** — optional compatibility projection
   - remosaiced from reconstructed RGB;
   - never called measured sensor RAW;
   - exists only for applications/workflows that require a mosaic representation.

5. **JPEG / thumbnail / UI preview**
   - presentation only;
   - may be embedded for Android/gallery/RAW-application discovery;
   - never used to validate or redefine the Scientific Master.

## Finite DNG headroom contract

The v0.1 direct `[0,1]` clipping rule is not sufficient for the preferred RGB RAW output because real TruthRaw scenes can contain valid reconstructed scene-linear values above 1.

The restored LinearRaw writer must therefore use a finite compatibility window, selected deterministically from the finalized Scientific Master before the DNG payload is emitted. Historical validated examples used windows such as `1.25x` and `2.0x` and compensated the representation with DNG `BaselineExposure = log2(window)` so downstream software can recover the intended nominal brightness while values above scene 1 remain representable.

Requirements:

- negative Scientific-Master values may clamp at the finite unsigned representation boundary and must be counted;
- positive values must not clip merely because they exceed scene value 1 when they fit the selected compatibility window;
- any remaining high clipping after window selection must be counted explicitly;
- window selection and quantization may not feed back into Scientific Master, TruthRange `L0`, scene binding, Backplane, evidence counts or color authority;
- the chosen window and BaselineExposure must be deterministic and covered by round-trip tests.

## Preview/container contract

The preferred compatibility structure to restore is the previously validated multi-IFD pattern:

- **IFD0:** small display thumbnail / preview entry point;
- **raw SubIFD:** full-resolution 3-channel 16-bit LinearRaw RGB payload;
- **preview SubIFD:** display-oriented preview suitable for Android/reader discovery.

Historically the Android-preview-fixed files used a small IFD0 thumbnail, an unchanged full-resolution LinearRaw SubIFD, and a JPEG display preview SubIFD. The scientific LinearRaw bytes were not changed by the preview fix.

For v0.2, preview implementation may be native or Android-assisted, but the following are mandatory:

- preview generation is downstream of the finalized scientific state;
- preview cannot alter raw RGB payload bytes;
- preview cannot alter source/master hash, zero-line, scene-scale identity, Backplane or authority;
- absence/failure of preview generation must not silently convert a scientifically valid export into a different scientific product;
- Android/gallery preview usability and Lightroom/ACR editability must be tested on the exact produced DNG bytes.

## Finalized export gate

Keep the existing 12 September fail-closed chain:

`source SHA-256 seal -> DNG color producer v0.2 -> prepared source -> source reverification -> TileNative source -> Scientific Master streaming v0.2 -> TruthRange self-gauge -> Technical Backplane phase 2 -> finalized admission -> projection writer -> source reverification`

The restoration changes representation, not scientific authority.

Every exported projection must keep:

- `physicalFrameCount = 1`;
- `independentEvidenceCount = 1`;
- `fullScientificMasterMaterialized = false` for the bounded streaming path;
- no promotion to `FULL_PHYSICAL` color without independent calibration;
- Scientific Master hash unchanged before/after export;
- TruthRange `L0`, gauge ID and scene-scale ID unchanged before/after export.

## Performance / house-runtime rule

Restore the RGB output without undoing the house/runtime work:

- stream in bounded strips/tiles;
- do not materialize the full 4080x3072 float RGB master solely for DNG layout;
- reuse bounded workspace where possible;
- export remains a downstream sink, not a new room with authority over upstream science;
- later Building Runtime scheduling may change tile size/concurrency, never the output equations or evidence claims.

## Validation gates before APK promotion

1. GCC Release compile/test PASS.
2. Clang Release compile/test PASS.
3. Clang ASan/UBSan PASS.
4. Android arm64 NDK/Kotlin link/build PASS.
5. Structural TIFF/DNG parser round-trip PASS.
6. Exact LinearRaw payload round-trip within the documented finite-window quantization error.
7. BaselineExposure/window test including a scene with values `>1`.
8. Scientific Master hash and TruthRange/Backplane invariants unchanged before/after export.
9. Embedded preview read-back and decode PASS.
10. Adobe `dng_validate` on exact produced hashes: 0 errors / 0 warnings before calling standards-validation PASS.
11. Lightroom/ACR: file opens as editable RAW/LinearRaw and exposure/WB controls behave normally.
12. Android/Honor: file receives a usable preview/thumbnail in the intended viewer path.
13. Physical source re-verification before and after export remains PASS.

## Android naming for v0.2

Primary action:

**TruthRaw RGB RAW (Linear DNG) opslaan**

Secondary compatibility actions:

- **Reconstructed CFA DNG (compatibiliteit) opslaan**
- **Reconstructed CFA .rawsensor (compatibiliteit) opslaan**

A future sample-exact Direct-CFA/rawsensor rewrap must use a different explicit name and authority contract; it must not be conflated with the reconstructed CFA projection.
