# TruthRaw DNG v0.6 — standard compatibility validation

**Status:** `RESEARCH_PASS_TIFFFILE_LIBRAW__DNG_SDK_2724_OPEN`

## Standards correction

Adobe DNG 1.7.1 Chapter 5 defines linear reference values using BlackLevel/WhiteLevel normalization and says values above 1.0 should be clipped. Therefore the prior v0.5 idea of placing TruthRaw scene overrange above DNG WhiteLevel is retained only as a historical experiment, not a canonical DNG encoding.

v0.6 uses a finite virtual headroom window inside the DNG range. The unrestricted signed/overrange TruthRaw Scene Master remains authoritative outside the DNG container.

## Gates

- tifffile structure + exact payload round-trip: **PASS (3/3)**
- LibRaw 0.21.4 open/unpack/process: **PASS (3/3)**
- Adobe DNG SDK 1.7.1 build 2724 exact-hash validation: **OPEN / not run**
- Adobe certification: **not claimed**

## Exact candidates

### IMG_BNC_TRUTHRAW20260907_094414_423.dng
- DNG: `IMG_BNC_TRUTHRAW20260907_094414_423__TRUTHRAW_LINEAR_V06_STANDARD_COLOR_PREVIEW.dng`
- SHA-256: `ed0585c89f1f935ab860725cf8086410ca97561aaa17a3595b7f3f228e147635`
- source WhiteLevel-censored samples: 0
- TruthRaw scene range: -0.005658744 … 1.013005137
- compatibility window: 1.25×
- BaselineExposure compensation: +0.321928 EV
- max scene decode quantization error: 1.02519989e-05
- tifffile: PASS
- LibRaw 0.21.4: PASS
- DNG SDK 2724: OPEN

### IMG_BNC_TRUTHRAW20260907_094416_197.dng
- DNG: `IMG_BNC_TRUTHRAW20260907_094416_197__TRUTHRAW_LINEAR_V06_STANDARD_COLOR_PREVIEW.dng`
- SHA-256: `fbb69811f40a57b40b258de15fe54ab6c120fc807588d96f11e8e4f4306ec009`
- source WhiteLevel-censored samples: 288
- TruthRaw scene range: -0.005020548 … 1.730840802
- compatibility window: 2.0×
- BaselineExposure compensation: +1.000000 EV
- max scene decode quantization error: 1.63912773e-05
- tifffile: PASS
- LibRaw 0.21.4: PASS
- DNG SDK 2724: OPEN

### IMG_BNC_TRUTHRAW20260907_094423_122.dng
- DNG: `IMG_BNC_TRUTHRAW20260907_094423_122__TRUTHRAW_LINEAR_V06_STANDARD_COLOR_PREVIEW.dng`
- SHA-256: `dd2ca6522da5e0d5e148210e349afa1503532ff2027a73afe5befdc06ef3710e`
- source WhiteLevel-censored samples: 1607
- TruthRaw scene range: -0.003741123 … 1.075762868
- compatibility window: 1.25×
- BaselineExposure compensation: +0.321928 EV
- max scene decode quantization error: 1.02519989e-05
- tifffile: PASS
- LibRaw 0.21.4: PASS
- DNG SDK 2724: OPEN

## Promotion rule

Do not promote these exact hashes to DNG-SDK-validated until build 2724 (or a later explicitly chosen reference) has run on the exact files. A repack or metadata change creates a new hash and requires revalidation.