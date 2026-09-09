# TruthRaw TruthRange Real-DNG → Stage-2 Bridge v0.4

Status: **RESEARCH PASS — BnCam SOURCE-DOMAIN BRIDGE CLOSED; NOT A GENERAL DNG DECODER**

v0.4 closes the missing real-source bridge between the sealed HONOR BnCam DNG and the signed Stage-2 domain used by the v4.7i / TruthRange research stack.

## Exact source path

For the supported BnCam source domain:

`immutable CFA code -> per-phase BlackLevel subtraction -> (WhiteLevel - phaseBlack) normalization -> source-bound OpcodeList2 GainMap exactly once -> signed Stage-2`

No white balance, color matrix, demosaic, appearance, tone curve, sharpening, SDR/HDR mapping or DNG re-export is part of this bridge.

The Stage-2 equation is:

`stage2 = ((raw - blackPhase) / (WhiteLevel - blackPhase)) * gainMap(y,x)`

The arithmetic preserves negative numerical estimators and values greater than 1.0.

## Real BnCam OpcodeList2 result

Eight HONOR BKQ-N49 tele DNGs spanning ISO 100..12800 were decoded.

Every file contains exactly:

- `OpcodeList2` bytes: `3908`
- opcode count: `4`
- opcode ID: `9` (`GainMap`)
- minimum version: `1.3.0.0`
- flags: `1`
- payload bytes per opcode: `960`
- GainMap geometry: `13 × 17 × 1`
- spacing: `(1/12, 1/16)`
- origin: `(0,0)`
- CFA pitch: `2 × 2`

The four areas bind directly to the BGGR phases:

- `(1,1)` -> R
- `(0,1)` -> G1
- `(1,0)` -> G2
- `(0,0)` -> B

The eight complete OpcodeList2 SHA-256 values are all different. GainMap evidence is therefore bound to the exact source DNG; a single universal map must not be substituted.

Across the series the stored GainMap entries range from `1.0009765625` through `2.439453125`, so the spatial correction is materially non-unity.

## Independent full-frame parity

The native C++ path was checked against an independent `tifffile + NumPy` oracle over all `12,533,760` CFA samples in each of the eight DNGs.

For every capture:

- decoded RAW `uint16` SHA-256 from the stdlib reader equals the independent TIFF decode;
- Stage-2 float-bit sum matches exactly;
- Stage-2 float-bit XOR matches exactly;
- min/max and all integer counts match exactly;
- maximum aggregate numeric difference is `2.636779683484747e-16` (floating accumulation only).

Native gain samples vs the independent stdlib reference differ by at most `1.1687185264008804e-07`; native Stage-2 samples differ by at most `1.9691490826811986e-08`.

## Important >1 result

Real Stage-2 values exceed `1.0` beginning at ISO 400 even when source WhiteLevel clipping is still zero.

Therefore:

**Stage-2 > 1 is not clipping.**

WhiteLevel clipping is determined from the immutable source CFA codes and remains censor evidence. GainMap-corrected scene-linear values above 1.0 must remain representable in the new house.

## Portable reference and native implementation

Two independent implementation surfaces are retained:

1. `reference/dng_stage2_reference_v0_4.py`
   - Python standard library only;
   - classic-TIFF parser;
   - uncompressed 16-bit CFA strips;
   - DNG BlackLevel / WhiteLevel / CFA tags;
   - big-endian OpcodeList2 parser;
   - GainMap pixel-centre interpolation.

2. `native/dng_stage2_v0_4.*`
   - C++17;
   - same fail-closed source contract;
   - full-frame native Stage-2 path;
   - CLI for external source-DNG audits.

The implementation rejects unsupported layouts rather than silently guessing.

## DNG SDK semantic binding

The parser/interpolator was checked against the same Adobe DNG SDK source mirror commit already used in TruthRaw validator provenance:

`hfiguiere/dng_sdk @ 11e5eb4259c2a7ad92d66e152e4974441d2e226a`

Relevant source semantics:

- `dng_opcode_list.cpp`: big-endian opcode-list layout and exact byte consumption;
- `dng_gain_map.cpp`: GainMap payload and interpolation;
- `dng_misc_opcodes.cpp`: `dng_area_spec` layout.

This is a source-semantic reference, not a new Adobe certification claim.

## Scientific boundary

v0.4 closes the **HONOR BnCam real-DNG -> Stage-2** bridge only.

It does not claim:

- arbitrary-DNG compatibility;
- physical electron calibration;
- topology-certified uncertainty for missing RGB channels;
- RGB covariance;
- absolute radiometry;
- newly measured photons above clipping.

## Next gate

Feed the exact real Stage-2 fields into the v0.2 Latent Camera Scene adapter and v0.3 measured-channel uncertainty field, then bind v5.0g measured-role anchors directly to the real Stage-2 source geometry.
