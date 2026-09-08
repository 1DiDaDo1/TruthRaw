# TruthRaw LinearRaw v0.4 — release-candidate interoperability report

## Status

**EXTERNAL_INTEROP_STRONG_PASS — ADOBE `dng_validate` EXECUTION OPEN**

v0.4 is the first candidate in this export sequence that simultaneously preserves the scientific pixel master and passes the current structural preflight, LibRaw, and libtiff gates.

It remains **DERIVED_RECONSTRUCTED_RAW**, not an original sensor/ADC RAW and not a claim that reconstructed RGB components are newly measured photons.

## Why v0.4 exists

- **v0.1 rejected:** compression was written as TIFF `32946 / AdobeDeflate`; LibRaw rejected the file at open.
- **v0.2 superseded:** uncompressed output became LibRaw-readable, but the main IFD lacked `NewSubFileType` and `OriginalRawFileDigest` was not paired with embedded `OriginalRawFileData`.
- **v0.3 superseded:** those structural issues were corrected, but `Orientation=1` lost the original sensor-raster orientation metadata.
- **v0.4:** `Compression=1`, `NewSubFileType=0`, source `Orientation=3`, exact source `UniqueCameraModel`, no unpaired OriginalRawFileDigest.

No reconstructed pixel values changed during the v0.2→v0.4 container corrections.

## Independent gates

- Relevant DNG-SDK validation logic mirrored as static preflight: **PASS**
- LibRaw 0.21.4 `open_file`: **PASS**
- LibRaw `unpack`: **PASS**
- LibRaw processing: **PASS**
- libtiff all 3072 scanlines: **PASS**
- libtiff decoded payload SHA-256 equals the expected scientific pixel payload: **PASS**
- Actual Adobe SDK `dng_validate`: **OPEN / not executed in this sandbox**

## Files

### `IMG_BNC_TRUTHRAW20260907_094414_423__TRUTHRAW_DERIVED_LINEAR_RAW_v0_4.dng`

- File SHA-256: `13e32e0967dd9726f31dfbec33a1461c4f4c2f98e4f67789eed1cc5f7766b9fc`
- Pixel payload SHA-256: `9f08de2fbe2efdb646fd92749a732a5682d1c235eda17a85544b3bc1b11f83be`
- Size: 75,205,424 bytes
- 4080×3072 × 3, 16-bit LinearRaw
- `PhotometricInterpretation=34892`
- `Compression=1`
- `NewSubFileType=0`
- `Orientation=3`
- Static preflight: **PASS** (34/34 checks)
- LibRaw: **PASS**
- libtiff payload: **PASS**
- Uncertainty scope: **GENERALIZATION_REVIEW_OPEN_HIGH_RISK_TAIL_UNDERCOVERAGE**; Q5 ratio `0.705044`

### `IMG_BNC_TRUTHRAW20260907_094449_565__TRUTHRAW_DERIVED_LINEAR_RAW_v0_4.dng`

- File SHA-256: `347cd68ade21f99607028b23ed7cdc0c6b498885d236e9c6443624228747766f`
- Pixel payload SHA-256: `71b42d01c2e0a54e0807ad671529d7dddebb3e0982c1d7246515b12b49db26eb`
- Size: 75,205,408 bytes
- 4080×3072 × 3, 16-bit LinearRaw
- `PhotometricInterpretation=34892`
- `Compression=1`
- `NewSubFileType=0`
- `Orientation=3`
- Static preflight: **PASS** (34/34 checks)
- LibRaw: **PASS**
- libtiff payload: **PASS**
- Uncertainty scope: **FROZEN_TELE_UNCERTAINTY_TAIL_PASS**; Q5 ratio `0.917171`

## Promotion boundary

The next promotion gate is intentionally binary: run an actual Adobe DNG SDK `dng_validate` executable against these exact file SHA-256 values. A different repack is a different candidate and must be revalidated.