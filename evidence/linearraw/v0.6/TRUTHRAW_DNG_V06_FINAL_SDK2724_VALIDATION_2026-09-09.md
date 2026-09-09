# TruthRaw LinearRaw v0.6 — final DNG SDK 2724 validation

**Status: DNG_SDK_1_7_1_2724_EXACT_HASH_PASS**

This closes the DNG container-compatibility gate for the exact three final hashes below. The LinearRaw DNG remains a finite compatibility projection of the TruthRaw Scene Master; it does not replace or bound the unrestricted scientific Scene Master.

## Validator provenance

- Validator banner: `dng_validate, version 1.7.1 (2724) (64-bit)`
- Validator binary SHA-256: `81d91bbab17d89d5324f88fc726666754730096dc508d61ea1233ce62707d71e`
- Official Adobe SDK ZIP SHA-256: `740fbe95c69e09e9cd17654a5e4fef2d7021254b06fd2b8c5557b79a1496b50c`
- Official source route: `https://www.adobe.com/go/dng_sdk`
- Release: DNG SDK 1.7.1 build 2724, 2026-09-08
- GitHub build branch: `validation/dng-sdk-2724-build`
- Build head: `294739b01edf20e9741cc6693bdacea8c1dc2533`
- Workflow run: `34391293123`
- Artifact ID: `10119962236`
- Artifact digest: `sha256:c7021a2010eef5a3c54c9fe337f89dd76a437198a4aebc44df2db014853bb909`
- Linux compatibility-only patch: add standard `<cstring>` include to `XMPStream_IO.cpp` for GCC 13 `memcpy` declaration. No DNG semantic code was changed.

## Failure found and corrected

The first 2724 validation of the predecessor v0.6 hashes exposed misplaced IFD tags:

- `DefaultScale`, `DefaultCropOrigin`, `DefaultCropSize`, and `ActiveArea` were incorrectly duplicated on reduced RGB preview IFD0.
- `Orientation` was incorrectly duplicated on the main LinearRaw SubIFD.

The final writer path preserves the historical pre-fix writer and applies a fail-closed wrapper correction in `tools/build_truthraw_linearraw_v06_standard_sdk2724.py`.

## Exact final hashes

| Capture | SHA-256 | Compatibility window | Scene range | Source-white censored | SDK 2724 |
|---|---|---:|---:|---:|---|
| 094414 | `c792c8d91cf84e9b7caa7c75122bdfed6973793d886957102d6fef603a79bda5` | 1.25x | -0.00565874 … 1.01300514 | 0 | PASS, 0 errors / 0 warnings |
| 094416 | `4ad1f42365a16ba36a012f82facca82c595610ab915429158e0de99b37f870b5` | 2.0x | -0.00502055 … 1.73084080 | 288 | PASS, 0 errors / 0 warnings |
| 094423 | `483b275125affe6cd4c5ced75aeab56bedd008e1c18b84fc710c4bfe558bd8eb` | 1.25x | -0.00374112 … 1.07576287 | 1607 | PASS, 0 errors / 0 warnings |

## Closed gates

- tifffile structure + byte-exact LinearRaw payload round-trip: **PASS 3/3**.
- Adobe DNG SDK 1.7.1 build 2724 exact-hash validation: **PASS 3/3, zero errors and zero warnings**.
- DNG storage clipping: **PASS, zero out-of-range stored samples 3/3**.
- `OpcodeList2` copy guard: **PASS absent 3/3**, preventing silent double GainMap application in the derived export.
- Source `NoiseProfile` copy guard: **PASS absent 3/3**, so CFA-domain noise metadata is not falsely relabeled as derived RGB uncertainty.

## Boundary retained

This closes only the DNG compatibility/export gate. RGB covariance, missing-channel topology certification, independent L2 target color calibration, physical illuminant/SPD, PTC/electrons/full-well/read noise, optics PSF/MTF/CA/flare/shading, and final `FULL_PHYSICAL` promotion remain open.

LibRaw was not successfully reinstalled after the final hash change in the current runtime, so the predecessor-hash LibRaw result is explicitly **not inherited** to these final hashes.
