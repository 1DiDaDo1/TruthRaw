# TruthRaw v0.68 real-device Full-Resolution Restoration validation

Date: 2026-09-20

Branch:

`integration/truthraw-suite-v0-68-restoration-transactional-fgs`

Validated artifact:

`IMG_BNC_TRUTHRAW20260907_094414_423_truthraw_fullres_restoration_v0_67 (1).trr`

Whole-file SHA-256:

`8277c50681e445f179ad5e36dcd0cad259e7f3d09f5e9c1a21c6de74529927c4`

File bytes:

`162,996,224`

## Result

**PASS** for complete on-device source-resolution container generation and finalization.

The v0.67 scientific container version is intentionally retained inside the v0.68 Android transaction wrapper because v0.68 changes lifecycle/commit semantics, not restoration science.

## Complete container structure

Header:

- valid `TRUTHRAW_FULLRES_RESTORATION_V0_67`;
- `width=4080`;
- `height=3072`;
- `tile_count=3072`;
- `total_bytes=162996224`;
- `END_HEADER` present.

Raster:

- exactly 3072 canonical tiles;
- tile sequence is complete and canonical;
- final tile is `x=4032, y=3008, w=48, h=64`;
- all Float32 components are finite;
- negative components: `300,802`;
- components > 1: `2`;
- minimum: approximately `-0.0056587439`;
- maximum: approximately `1.01300514`.

## Scientific lineage

Source SHA-256:

`578fad42dad6819b1d3f9a1f9cbfcc5c547b63ae01f3951f26dca988d655d10e`

Scientific Master SHA-256:

`faca6604479380c2de83917e977a4cfbcd17ea2a3acf6f842b753315f9987423`

Restoration derivative RGB SHA-256:

`faca6604479380c2de83917e977a4cfbcd17ea2a3acf6f842b753315f9987423`

The derivative digest equals the Scientific Master digest for this test source because no pixel became restoration-eligible.

Zero-Line SHA-256:

`ed3748e5f4a8feb2eaab7bb9cf39ce68bffe8c6723b76e52e80b21b856b2b030`

Scene-scale SHA-256:

`90cca2a7932116f9059174e50028817a0ecc66df182d107eda92b1955cb6fdaf`

Technical Backplane:

- exactly 180 bytes;
- magic `TRBACK01`;
- stored CRC32 `0x8cde5b54`;
- recomputed CRC32 over bytes 0..175 = `0x8cde5b54`.

## Restoration roles

Header counters:

- preserved pixels: `12,533,760`;
- censored source pixels: `0`;
- restored pixels: `0`;
- unresolved pixels: `0`;
- changed components: `0`.

Parsed role-mask:

- role 0 / `PRESERVE_SCIENTIFIC_MASTER`: `12,533,760`;
- role 1 / `AESTHETIC_REINTEGRATION_ONLY`: `0`;
- role 2 / `UNRESOLVED_LOSS`: `0`.

Therefore this file validates the full-resolution container and transaction path but does **not** exercise actual pixel restoration. A future restoration-effect test needs a source with admitted CFA samples at/above WhiteLevel.

## Relationship to the earlier 25,854,576-byte upload

Earlier uploaded snapshot:

`IMG_BNC_TRUTHRAW20260907_094414_423_truthraw_fullres_restoration_v0_67.trr`

had:

- size `25,854,576` bytes;
- all-zero 8192-byte header;
- exactly 487 complete canonical tile records;
- exactly `1,987,584` source pixels;
- last complete tile `x=2432, y=448, w=64, h=64`.

The earlier payload bytes after the zero header are **byte-for-byte identical** to the corresponding prefix of the complete v0.68 test artifact.

SHA-256 of earlier body prefix:

`d3458ee8c95939b028166b00d4e80c677e6e7449b1e59a0e64b924eef9ba8345`

SHA-256 of the same-length body prefix in the complete artifact:

`d3458ee8c95939b028166b00d4e80c677e6e7449b1e59a0e64b924eef9ba8345`

This materially revises the earlier interpretation. The old upload is fully consistent with an **in-progress snapshot uploaded before v0.67 finalization**, rather than proving that the device export itself had terminated at tile 487. Because the uploaded chat attachment is a snapshot, it would remain partial even if the device-side file continued growing afterward.

v0.68 remains useful because its foreground staging/header-last commit model prevents a user-visible partial destination from being mistaken for a completed artifact and makes long background execution more robust.

## Remaining real-device gate

The transaction/container path is complete for this source.

Still open:

- demonstrate a source with `censored_source_pixels > 0`;
- verify that only role-1 pixels differ from the base Scientific Master;
- verify role-2 remains unchanged/unresolved when support is insufficient;
- validate failure cleanup with an intentional hard process termination if desired.

