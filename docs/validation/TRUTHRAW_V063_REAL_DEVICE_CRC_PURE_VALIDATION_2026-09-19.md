# TruthRaw v0.63 real-device CRC + PURE artifact validation

Date: 2026-09-19

Status: **PASS**

Validated artifact:

`IMG_BNC_TRUTHRAW20260907_094414_423_truthraw_pure_float32_v0_63.dng`

Artifact SHA-256:

`ebcf0751b6489e2acc82cafbb927e76e842064e0470717af657a208e1e653235`

Artifact bytes:

`151022004`

## DNG structure

- width: 4080
- height: 3072
- samples per pixel: 3
- BitsPerSample: 32 / 32 / 32
- SampleFormat: IEEE Float / IEEE Float / IEEE Float
- PhotometricInterpretation: LinearRaw
- DNGVersion: 1.4.0.0
- Compression: 1 (uncompressed)
- Orientation: 3
- writer identity: `TruthRaw scientific-master-linear-dng-projection-v0.1`

## v0.63 private binding

The saved DNG contains:

- `role=TRUTHRAW_PURE_FLOAT32_XYZ_D50_LINEAR_DNG_PROJECTION`
- `private_contract=TRUTHRAW_PURE_SELF_BINDING_V0_63`
- source SHA-256 `578fad42dad6819b1d3f9a1f9cbfcc5c547b63ae01f3951f26dca988d655d10e`
- Scientific Master SHA-256 `faca6604479380c2de83917e977a4cfbcd17ea2a3acf6f842b753315f9987423`
- Zero-Line SHA-256 `ed3748e5f4a8feb2eaab7bb9cf39ce68bffe8c6723b76e52e80b21b856b2b030`
- exact L0 binary64 bits `0x3f979f54a0000000`
- decoded L0 approximately `0.023068735376000404`
- scene-scale SHA-256 `90cca2a7932116f9059174e50028817a0ecc66df182d107eda92b1955cb6fdaf`
- `technical_backplane_crc_scope=PREFIX_176_BYTES`
- declared Technical Backplane CRC32 `0x8cde5b54`
- exact 180-byte serialized Technical Backplane

## Technical Backplane verification

Decoded serialized record:

- magic: `TRBACK01`
- version: 1
- serialized bytes: 180
- forbiddenFlags: 0
- physicalFrameCount: 1
- independentEvidenceCount: 1
- reserved bytes 165..175: all zero
- internal stored CRC32 bytes 176..179: little-endian `54 5b de 8c` = `0x8cde5b54`

Recomputed CRC32 over bytes 0..175:

`0x8cde5b54`

Therefore:

`recomputed CRC == internal stored CRC == DNG declared CRC`

The v0.62 full-180-byte residue bug is closed.

The four Backplane identity hashes also match the textual DNG private binding exactly:

- source
- Scientific Master
- Zero-Line
- scene-scale

## Float32 payload validation

Actual raster components: `37,601,280`

- all components finite
- negative components: `496,902`
- components > 1: `0`
- minimum: approximately `-0.012010658159852028`
- maximum: approximately `0.9967811703681946`

Canonical raster-order Float32 payload SHA-256:

`9ea98575cfb5c19842cf3b2eb4a4637338880908f7a20b86908a57afc4c14b59`

This is bit-identical to the previously validated v0.60/v0.62 PURE raster for the same sealed source. The v0.63 CRC correction, self-binding versioning, UI and branding therefore did not alter the PURE pixel payload.

## Empirical wrapper

The paired empirical report identifies app `0.28-v0.63-crc-ui-branding`, versionCode 28 and installed APK SHA-256 `82dc3103b4e945e129fa5ea38fe717a342d459d1f6b71e4895bab9e00282d173`.

Its pre- and post-probe source SHA-256 are both the same source hash used by the DNG private binding. The source remained stable across the empirical wrapper.

## Verdict

**v0.63 real-device PURE artifact gate passes.**

The CRC defect identified from v0.62 is corrected in the actual saved DNG, not merely in source code or CI. The post-write self-binding route, frozen Backplane, 1/1 evidence identity and Float32 pixel invariance are all preserved.
