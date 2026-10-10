# D.RAW 44489 — High-Fidelity JPEG real-device PASS — 2026-10-08

Status: **REAL-DEVICE OUTPUT PASS** for the exact 0° PURE tele High-Fidelity JPEG path described below.

This record promotes only the downstream presentation/export acceptance state. It does **not** promote scientific authority, reconstruction authority, calibration, sealed evidence or Scientific Master state.

## Exact tested runtime / APK

Runtime/code head used for the successful export:

`f42cd79ef5060e78c1c6e6bcc1e7d9d94a07d588`

Workflow proving the corresponding APK build:

- workflow: `D.RAW Free Raster v0.3 Finish APK`;
- run: `37691044445`;
- result: SUCCESS;
- artifact ID: `11513327435`;
- artifact ZIP digest reported by GitHub: `sha256:189f6962b5c269259585dab563dd2af684c0668ca773a671f5b20054fce32612`.

APK independently re-hashed from that artifact:

- bytes: `8,816,915`;
- SHA-256: `4c6a794b252a43a6bf38405b053e667738e779c0fe014bdbe02713765cfdeb22`.

PR #131 remains open, draft and unmerged. PR #130 / `4e4f358ac5d6fb4a8562286257b8edb76ae5d33f` remains frozen and scientifically untouched.

## Physical capture/output under test

The user identified the capture as **tele camera**.

Uploaded acceptance bundle:

`DRAWRAWJPG444succes.zip`

Contained files:

### Companion Float32 DNG

`DRAW_CAPTURE_1791409962721_tele_4080x3072_draw_pure_float32_v0_63.dng`

- bytes: `172,761,232`;
- SHA-256: `2a4f5b50c0e2b630306464610d773721016a2a32d0e0726b4ad237c5fb496797`.

This file is recorded only as companion provenance for the device round. This acceptance record does not infer a new scientific-authority claim from its presence.

### High-Fidelity JPEG under acceptance

`DRAW_CAPTURE_1791409962721_tele_4080x3072_draw_pure_fullres.jpg`

- bytes: `21,738,392`;
- SHA-256: `7d37016b7dbf6277ce5e196ec778bb33aa927f78009dd07c49b2891f14cd47ff`;
- geometry: `4080 × 3072`;
- route: PURE;
- rotation tested in this record: 0°.

## Independent JPEG structure inspection

The saved physical JPEG itself was inspected, not merely the encoder request or UI label.

### SOF / sampling

- JPEG type: baseline SOF0;
- precision: 8-bit;
- components: 3;
- stored geometry: `4080 × 3072`;
- component sampling: `0x11 / 0x11 / 0x11`;
- interpretation: `1×1 / 1×1 / 1×1`, i.e. real 4:4:4.

FFprobe independently reported a 4:4:4 JPEG pixel format (`yuvj444p`).

### Q100 quantization contract

DQT tables 0 and 1 were both present as 8-bit tables and every quantization entry was exactly `1`.

This is the admitted D.RAW Q100 structural contract. D.RAW does not treat an integer encoder request alone as proof of JPEG quality.

### SOS / entropy stream

- SOS marker offset: `593`;
- SOS segment length: `12`;
- canonical sequential SOS payload: `03 01 00 02 11 03 11 00 3F 00`;
- entropy stream begins at offset: `607`;
- terminal EOI: present exactly at file end;
- valid `FF 00` byte-stuffings observed in entropy stream: `18,208`;
- unexpected unstuffed markers in entropy stream: `0`.

This directly closes the earlier SOS off-by-one corruption that strict decoders correctly rejected.

## Strict decoder acceptance

The physical JPEG was independently accepted by multiple decoders:

- FFmpeg full decode: no error;
- Pillow verify/load: success;
- ImageMagick: recognized as JPEG Quality 100;
- FFprobe: 4080×3072, 4:4:4 pixel format;
- Adobe Lightroom: **user-confirmed successful open/check on 2026-10-08**.

The Lightroom confirmation is the final real-device/application compatibility evidence requested after the earlier corrupt-container round.

## Historical failures preserved as provenance

Two earlier failures remain valuable regression evidence and must not be erased:

1. first Q100/4:4:4 candidate: formally 4:4:4/Q100 headers but corrupted chroma image content due to a Huffman lookup mismatch;
2. next candidate: visual bands fixed, but strict decoders/Lightroom rejected the JPEG because the SOS header emitted one extra `0x00`, shifting the entropy start by one byte.

The accepted `f42cd79e...` path fixes the saved output structure while preserving Q100 and true 4:4:4.

## Acceptance classification

**PASS — HIGH_FIDELITY_JPEG_Q100_444_REAL_DEVICE_LIGHTROOM_COMPATIBLE_0_DEGREE_TELE**

What this PASS proves:

- full-resolution tele PURE JPEG output can be saved on real hardware;
- the actual saved file is 4080×3072;
- the actual saved file is baseline JPEG;
- the actual saved file is real 4:4:4;
- the actual saved file satisfies the admitted Q100 DQT contract;
- the actual saved file has canonical sequential SOS structure;
- the entropy stream is marker-clean under the admitted no-restart structure;
- strict software decoders and Lightroom accept the file.

What this PASS does **not** prove:

- +90° Q100/4:4:4 output under the new codec;
- route-mismatch lifecycle fail-closed behavior;
- arbitrary target-resolution output;
- embedded ICC/profile completion;
- lossless/high-bit-depth PNG completion;
- any new scientific reconstruction or authority promotion.

## Regression lock required after this PASS

The accepted output becomes the downstream regression contract. Future changes must fail CI if they break any of:

- requested quality exactly 100;
- DQT0/DQT1 all-one Q100 contract;
- SOF0 only;
- exact width/height binding;
- three components;
- `1×1 / 1×1 / 1×1` sampling;
- canonical baseline-sequential SOS;
- terminal EOI;
- entropy marker/stuffing validity;
- strict full JPEG decode;
- output derived from the full-resolution RGB24 sibling rather than a preview/presentation bitmap.

A dedicated host codec regression is added to the Free Raster APK workflow to compile the actual D.RAW Q100/4:4:4 encoder, encode a chroma-stressing synthetic RGB24 raster, inspect the resulting JPEG contract and require strict FFmpeg decode before the Android build proceeds.

## Scientific boundary unchanged

`MEASURED != CALIBRATED_ESTIMATE != RECONSTRUCTED != CENSORED != UNKNOWN != APPEARANCE`

The accepted JPEG is `DERIVED_PRESENTATION_OUTPUT`. It does not create MEASURED evidence, promote UNKNOWN, alter sealed CFA/RAW evidence, mutate Scientific Master, apply a scientific candidate or authorize scientific writeback.
