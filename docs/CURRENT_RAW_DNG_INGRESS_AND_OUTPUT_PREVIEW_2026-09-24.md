# TruthRaw — current RAW/DNG ingress + output preview — 2026-09-24

## Purpose

This document is the current main-project truth for two areas that were easy to overstate in the UI:

1. what "Open RAW / DNG" actually admits into the Scientific-Master pipeline;
2. what the visible output preview represents after an export.

Permanent law:

**Representation can exceed the source. Knowledge claims cannot exceed the evidence.**

## Common admitted scientific route

For a DNG that passes the current strict admission profile, the main route is:

```
document handle / camera-processing DNG
-> independent source SHA-256 seal
-> source-bound DNG colour binding
-> MultiVendorRawSourceAdapter v0.1
-> TileNativeDngSource v0.1
-> random-access CFA/sample tiles
-> Stage-2
-> active F64 branch-sensitive reconstruction
-> controlled Float32 canonical Scientific Master storage
-> Dynamic Authority / Open Scene
-> Open Scene Field v0.85
-> local policy v0.86
-> PURE / Advanced / Restoration / TruthNegative TN-4
-> output-specific Unified Output Preview v0.1
```

A camera-origin processing DNG is not allowed to inherit scientific authority merely because
its acquisition parent was already sealed. The processing DNG is independently resealed and
admitted. The upstream Camera2 RAW_SENSOR lineage remains acquisition provenance.

The ingress layer stores document handles and lightweight metadata. It does not first copy the
entire RAW payload into one in-memory buffer.

## DNG admission profile

The current `TileNativeDngSource v0.1` is intentionally strict and fail-closed.

Current admitted core requirements include:

- classic TIFF/DNG (TIFF magic 42); BigTIFF is rejected;
- exactly one supported CFA IFD unless an explicit IFD offset is supplied;
- `PhotometricInterpretation = CFA`;
- unsigned integer, one sample per pixel;
- 16-bit TIFF sample storage;
- `Compression = 1` (uncompressed);
- 2x2 RGB Bayer CFA: BGGR, RGGB, GRBG or GBRG;
- finite WhiteLevel and one or four finite BlackLevel values with WhiteLevel above every black phase;
- exactly one storage model: strips or tiles;
- optional NoiseProfile only under its admitted six-DOUBLE/RGB binding;
- optional OpcodeList2 only when every entry is the admitted GainMap form and each CFA phase is covered exactly once.

Therefore compressed DNG, BigTIFF, packed 10/12/14-bit TIFF storage and unsupported CFA/storage
topologies remain fail-closed. "DNG" does not mean every possible DNG encoding is automatically admitted.

## Proprietary RAW support matrix

### DNG

Status: **FULL SCIENTIFIC ROUTE WHEN ADMITTED**

The strict DNG adapter is the current full route into F64 reconstruction, canonical Scientific
Master, Open Scene v0.85, TN-4/local authority and the output/export stack.

### Nikon NEF

Status: **MEASUREMENT-ONLY SUBSET**

A strict uncompressed-16 CFA NEF decoder exists for sample-domain inspection. Exact CFA sample
codes may be inspected, but Scientific Master, colour calibration and full photographic processing
remain blocked until the missing calibration/authority contracts are admitted.

### Canon CR3/CR2, Sony ARW/SRF/SR2, Fujifilm RAF, Panasonic RW2, Olympus/OM ORF,
Pentax PEF, Leica RWL, Hasselblad 3FR/FFF, Phase One IIQ, Sigma X3F, Samsung SRW,
Kodak DCR/KDC, Minolta MRW, Mamiya MEF and generic .raw

Status: **IMMUTABLE HANDLE / DECODER PENDING**

These formats are recognized and can enter the workspace as immutable source handles, but no
validated scientific decoder adapter is currently connected. They must not be described as if they
already receive the DNG Scientific-Master/F64/Open-Scene route.

Future proprietary support must follow:

```
format-specific parser/decoder
-> exact CFA/sample-domain contract
-> common IRawTileSource
-> same F64 Scientific-Master/Open-Scene core
```

There is no separate Canon, Nikon, Sony, Fuji or phone-specific scientific truth model.

## Unified Output Preview v0.1

The output preview is presentation, not evidence.

Core rule:

**Same stored/selected output primary route, smaller display projection.**

The UOP1 contract records source dimensions, preview dimensions, source-space code, sampled primary
pixel count and the stored display orientation in quarter turns. The Android loader checks this
contract before exposing a bitmap.

The preview renderer:

- consumes the chosen output primary directly or an exact bounded representation of that stored primary;
- adds no new HDR, sharpening, detail, restoration or relight;
- converts only as required into display-linear sRGB;
- clips only the display projection to [0,1] and applies the sRGB transfer function;
- never alters negative or >1 values in the stored primary;
- never grants authority;
- never writes back into Scientific Master or Open Scene.

Current route bindings include:

- PURE Float32 DNG -> random-access F64 Scientific-Master primary;
- Full Colour Scientific Master Float32 DNG -> same camera-native Scientific-Master primary;
- Advanced Render/Edit Float32 DNG -> final Render/Edit primary;
- TruthNegative 200MP Float32 DNG -> dense TruthNegative primary;
- TruthNegative TN-4 scientific negative -> TN-4 Scientific-Master primary;
- Full-res Restoration / restoration projections -> restoration derivative primary;
- compatibility Linear DNG -> exact bounded-U16 primary representation;
- saved JPEG output -> preview decoded from the actually committed JPEG bytes.

The preview also carries the stored output-orientation contract. The UI must display each outcome
using its own stored orientation rather than blindly reusing the current UI rotation.

## UI wording synchronized on 2026-09-24

The active product UI now states:

- launcher: `DNG volledig · proprietary RAW adapter-afhankelijk`;
- launcher architecture line: active F64 reconstruction + Open Scene v0.85 + TN-4 + UOP v0.1;
- Advanced header: `Canonical Open Scene v0.85 · gedeeld met TN-4/TRR/projecties`;
- Advanced Restoration: Open Scene Field v0.85 + local policy v0.86;
- PRO Precision: `F32 canonical storage · F64 compute actief`;
- PRO explicitly exposes Open Scene Field v0.85, TN-4 local authority v0.4, local policy v0.86 and UOP v0.1;
- PRO explicitly explains DNG full-route vs proprietary decoder-pending support;
- visible Scientific Negative button says TN-4;
- Settings no longer claims the old v0.84.2 architecture.

## Current code-bearing checkpoint

Main integration branch:

`integration/truthraw-suite-v0-84-3-float32-full-colour-scientific-master`

Code-bearing checkpoint immediately before this documentation sync:

`c020af855dfe67bad6b0c9ef9f03d44f74d10999`

This checkpoint contains the current per-output stored-orientation preview contract on top of the
F64/Open-Scene/TN-4/UI-ingress synchronization work.

