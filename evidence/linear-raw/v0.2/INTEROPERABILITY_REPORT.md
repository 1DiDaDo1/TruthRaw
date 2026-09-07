# TruthRaw LinearRaw v0.2 — interoperability report

## Decision

**v0.1 is rejected for external DNG interoperability. v0.2 passes LibRaw and libtiff.**

The scientific reconstructed pixel master did not change. v0.2 only replaces the problematic TIFF AdobeDeflate path with an uncompressed DNG raw IFD (`Compression=1`).

## Gates

- tifffile reopen: **PASS**
- libtiff complete scanline read: **PASS**
- libtiff payload SHA-256 == tifffile payload SHA-256: **PASS**
- LibRaw `open_file`: **PASS**
- LibRaw `unpack`: **PASS**
- LibRaw `dcraw_process`: **PASS**
- Adobe DNG SDK / `dng_validate`: **OPEN — not executed in this sandbox**

## Files

### IMG_BNC_TRUTHRAW20260907_094414_423__TRUTHRAW_DERIVED_LINEAR_RAW_v0_2.dng
- DNG SHA-256: `7d28c352129424162a3f1a2363e769fb86abdf21977433a38883351fd8cfad0d`
- Size: 75,205,504 bytes
- LinearRaw: 4080×3072 × 3 channels, 16-bit, Compression=1
- Pixel payload SHA-256: `9f08de2fbe2efdb646fd92749a732a5682d1c235eda17a85544b3bc1b11f83be`
- Exact v0.1 pixel payload preserved: **True**
- LibRaw: **PASS** (`open=0`, `unpack=0`, `process=0`)
- libtiff full payload: **PASS**

### IMG_BNC_TRUTHRAW20260907_094449_565__TRUTHRAW_DERIVED_LINEAR_RAW_v0_2.dng
- DNG SHA-256: `ffd5f0fdfd469ef6e6714ef70563e7d6726c33c7215c970e2c1a4a474f90d637`
- Size: 75,205,488 bytes
- LinearRaw: 4080×3072 × 3 channels, 16-bit, Compression=1
- Pixel payload SHA-256: `71b42d01c2e0a54e0807ad671529d7dddebb3e0982c1d7246515b12b49db26eb`
- Exact v0.1 pixel payload preserved: **True**
- LibRaw: **PASS** (`open=0`, `unpack=0`, `process=0`)
- libtiff full payload: **PASS**

## Scientific boundary

This proves the DNG can be independently parsed and developed by LibRaw and independently read by libtiff. It does not turn reconstructed RGB components into newly measured photons. The file remains `DERIVED_RECONSTRUCTED_RAW`.

Adobe DNG SDK validation remains required before TruthRaw calls this a production DNG export.