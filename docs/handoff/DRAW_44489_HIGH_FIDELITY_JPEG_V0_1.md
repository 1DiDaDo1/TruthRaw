# D.RAW 44489 — High-Fidelity JPEG v0.1 — accepted state 2026-10-08

Status: **REAL-DEVICE OUTPUT PASS for the exact 0° tele/PURE case**, with a dedicated end-to-end codec regression locked into CI.

This is downstream presentation/output only. It does not promote scientific authority, reconstruction authority, calibration, sealed evidence or Scientific Master state.

---

## 1. Current output cable

`existing full-resolution scientific/photo renderer -> exact display-oriented RGB24 sRGB sibling -> { Free Raster backing artifact | High-Fidelity JPEG encoder | future output siblings }`

Important consequences:

- the full-resolution RGB24 sibling is emitted before NV21/JPEG chroma reduction;
- Free Raster does not need to round-trip through JPEG compression;
- High-Fidelity JPEG consumes that RGB24 sibling directly;
- a preview bitmap cannot silently become the JPEG master;
- the old NV21 / Android `YuvImage.compressToJpeg()` route is not the admitted High-Fidelity JPEG path.

Safety state remains:

- `previewRequired=false`;
- `createsNewEvidence=false`;
- `scientificWritebackAllowed=false`;
- `sourceMutationAllowed=false`;
- source authority `EXISTING_ADMITTED_DNG_OBSERVATION`;
- output authority `DERIVED_PRESENTATION_OUTPUT`.

---

## 2. Admitted High-Fidelity JPEG contract

Requested quality is exactly **100**, but the request itself is not proof. The actual emitted JPEG must prove the contract fail-closed.

Required actual-file properties:

- baseline SOF0;
- exact frozen output width/height;
- 8-bit precision;
- three components;
- sampling `1×1 / 1×1 / 1×1` (true 4:4:4);
- DQT table 0 all `1`;
- DQT table 1 all `1`;
- canonical baseline sequential SOS;
- terminal EOI;
- no admitted malformed/non-baseline alternative.

Production verifier:

`suite_android/app/src/main/java/com/truthraw/adaptiveui/HighFidelityJpegContractV01.kt`

Contract version:

`HighFidelityJpegContract/0.2`

Unit tests:

`suite_android/app/src/test/java/com/truthraw/adaptiveui/HighFidelityJpegContractV01Test.kt`

These tests cover true 4:4:4, Q100 DQT, exact geometry and rejection of 4:2:0, non-Q100 quantization, malformed SOS and missing terminal EOI.

---

## 3. Historical codec failures retained as regression provenance

### A. Huffman/entropy mismatch

The first Q100/4:4:4 physical candidate had formally correct headers but severe horizontal colour bands. Header correctness alone was therefore insufficient. Root cause was a chroma Huffman lookup/bitstream mismatch.

The encoder was repaired so Huffman codes are generated deterministically from the same DHT definitions written into the JPEG stream.

### B. SOS off-by-one

The following candidate looked visually correct in tolerant decoders but Lightroom rejected it. FFmpeg/ImageMagick also exposed container/decode problems. Root cause: an extra `0x00` in SOS shifted entropy start by one byte.

The accepted runtime canonicalizes the baseline sequential SOS and verifies final output fail-closed.

Do not delete these failed rounds from project history; they justify the regression design.

---

## 4. Exact accepted runtime and build

Physically accepted runtime/code head:

`f42cd79ef5060e78c1c6e6bcc1e7d9d94a07d588`

Successful build workflow:

- workflow `D.RAW Free Raster v0.3 Finish APK`;
- run `37691044445`;
- result SUCCESS;
- artifact ID `11513327435`;
- artifact ZIP digest `sha256:189f6962b5c269259585dab563dd2af684c0668ca773a671f5b20054fce32612`.

Accepted APK:

- bytes `8,816,915`;
- SHA-256 `4c6a794b252a43a6bf38405b053e667738e779c0fe014bdbe02713765cfdeb22`.

PR #131 remains open, draft and unmerged. PR #130 / `4e4f358ac5d6fb4a8562286257b8edb76ae5d33f` remains frozen.

---

## 5. 2026-10-08 physical REAL-DEVICE OUTPUT PASS

User acceptance bundle:

`DRAWRAWJPG444succes.zip`

The user identified the source as tele camera.

### Companion Float32 DNG

`DRAW_CAPTURE_1791409962721_tele_4080x3072_draw_pure_float32_v0_63.dng`

- bytes `172,761,232`;
- SHA-256 `2a4f5b50c0e2b630306464610d773721016a2a32d0e0726b4ad237c5fb496797`.

Recorded as companion provenance only; no new scientific-authority claim is inferred from its presence.

### Accepted JPEG

`DRAW_CAPTURE_1791409962721_tele_4080x3072_draw_pure_fullres.jpg`

- route PURE;
- orientation tested here: 0°;
- bytes `21,738,392`;
- SHA-256 `7d37016b7dbf6277ce5e196ec778bb33aa927f78009dd07c49b2891f14cd47ff`;
- geometry `4080 × 3072`;
- baseline SOF0, 8-bit, 3 components;
- sampling `0x11 / 0x11 / 0x11` = true 4:4:4;
- DQT0 all `1`;
- DQT1 all `1`;
- SOS marker offset `593`;
- SOS length `12`;
- canonical SOS payload `03 01 00 02 11 03 11 00 3F 00`;
- entropy starts at offset `607`;
- `18,208` valid `FF 00` byte-stuffings;
- `0` unexpected unstuffed entropy markers;
- EOI exactly at file end.

Independent acceptance:

- FFmpeg full decode: success;
- FFprobe: `4080×3072`, `yuvj444p`;
- Pillow verify/load: success;
- ImageMagick: Quality 100;
- Adobe Lightroom: **user-confirmed successful check**.

Classification:

**PASS — HIGH_FIDELITY_JPEG_Q100_444_REAL_DEVICE_LIGHTROOM_COMPATIBLE_0_DEGREE_TELE**

Detailed device evidence:

`docs/handoff/DRAW_44489_HIGH_FIDELITY_JPEG_REAL_DEVICE_PASS_2026-10-08.md`

---

## 6. End-to-end regression lock

Regression-lock head:

`e4742bf048a160b65398d93461f9fa3799faa638`

This head changes test/CI only relative to the physically accepted runtime; it does not change accepted runtime codec semantics.

Added regression assets:

- `tools/high_fidelity_jpeg_codec_regression_v0_1.cpp`;
- `tools/test_high_fidelity_jpeg_codec_v0_1.py`;
- updated `.github/workflows/draw-free-raster-finish-v03.yml`.

The CI regression:

1. compiles the actual D.RAW header-only Q100/4:4:4 encoder;
2. builds a chroma-stressing synthetic RGB24 raster;
3. encodes it with the accepted final-output SOS canonicalization behavior;
4. verifies SOI/terminal EOI, SOF0, exact geometry, 4:4:4 sampling, all-one DQT0/DQT1 and canonical SOS;
5. checks entropy byte stuffing / rejects unexpected markers under the admitted no-restart structure;
6. requires a full strict FFmpeg decode using `-xerror`;
7. requires FFprobe geometry and 4:4:4 pixel-format confirmation;
8. blocks the Android build if any of these fail.

Workflow run:

- `D.RAW Free Raster v0.3 Finish APK` run `37696081058`;
- dedicated `High-Fidelity JPEG codec regression` step: SUCCESS on `e4742bf...`.

Harness setup provenance:

- first regression attempt failed because FFmpeg was not preinstalled on the Ubuntu runner;
- second setup attempt revealed a host-GCC `class-memaccess` warning promoted by `-Werror`;
- final harness explicitly installs FFmpeg and leaves host warnings visible while letting actual encode/contract/decode behavior decide PASS/FAIL.

These harness setup failures did not alter or invalidate the real-device accepted runtime.

---

## 7. Historical quality-96 / 4:2:0 audit remains historical only

The 2026-10-07 image-quality audit correctly found an older JPEG file to use quality 96 and 4:2:0 sampling. That result must remain as provenance for that old file.

It is not the current High-Fidelity JPEG endpoint. New admitted High-Fidelity JPEG output uses the physically validated Q100 / true-4:4:4 path above.

---

## 8. Scope not yet proven

This PASS does not prove:

- +90° under the new Q100/4:4:4 codec;
- route/source mismatch lifecycle on real device;
- arbitrary output-resolution scaling;
- embedded ICC/profile completion;
- PNG/lossless/high-bit-depth sibling output;
- broader device/source coverage;
- any new scientific reconstruction or authority promotion.

The older +90° full-resolution JPEG acceptance remains useful geometry provenance, but it was not this new codec state.

---

## 9. Scientific boundary unchanged

`MEASURED != CALIBRATED_ESTIMATE != RECONSTRUCTED != CENSORED != UNKNOWN != APPEARANCE`

The RGB24 sibling, Free Raster raster and JPEG are downstream derived presentation outputs. They cannot create MEASURED evidence, promote UNKNOWN, mutate sealed CFA/source bytes, change Scientific Master, apply a scientific candidate or authorize scientific writeback.
