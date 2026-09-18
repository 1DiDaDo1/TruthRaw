# HONOR Camera-5 v0.38 physical-only INT32 candidate sweep result — 2026-09-18

Status: **DEVICE MATRIX COMPLETE — 3/3 ATTACHMENT PASS, 3/3 CAPTURE PASS, NO RAW TOPOLOGY DIFFERENTIAL**

## Scope

v0.38 screens the three remaining v0.36 physical-only INT32 candidates one key per physical capture:

- P01 / I: `EnableVSR`, tag `0x801F000B`, INT32(1)
- P02 / J: `ExtendedMaxZoom`, tag `0x801F000A`, INT32(1)
- P03 / K: `enableQLL`, tag `0x801F000D`, INT32(1)

Exactly one unknown vendor key is written per run. There are no combinations and no value enumeration. Vendor names and numeric values remain semantically uninterpreted.

v0.38b changes only UI/system-bar placement and is scientifically identical to v0.38.

## Intervention result

All three runs report:

- physical-only availability topology pass;
- builder set pass;
- builder readback = 1;
- built-request readback = 1;
- session parameters attached = true;
- capture permitted = true;
- vendorKeysWritten = 1;
- semantic promotion disabled.

Therefore all three candidate requests passed framework/session attachment and completed the physical Camera-5 MAX source-first capture chain.

## Topology result

All three runs independently report:

- app-visible RAW_SENSOR envelope: 16320x12288;
- source bytes: 401,080,320;
- row stride: 32,640;
- pixel stride: 2;
- Stage 3.6 non-zero rows: 768;
- Stage 3.6 zero rows: 11,520;
- first non-zero row: 0;
- last non-zero row: 767;
- payload last non-zero byte offset: 25,067,518;
- payload extent: 25,067,520 bytes;
- exactly one advertised standard RAW byte-count match;
- selected interpretation candidate: 4080x3072;
- candidate payload is exact source prefix [0, 25,067,520), no transform;
- source authority unchanged;
- geometry semantic promotion disabled.

No P01/P02/P03 run produced a measurable envelope/populated-prefix geometry differential.

## Per-run identities

P01 / I / EnableVSR:
- source SHA-256: `69e752450bdc7976e0cef4a63ead2e49b8a9db2a813b32a0d3e0a5d3c19d0e82`
- exact-prefix payload SHA-256: `e4ce55fae84729fc187a8f4cc41d5167ab06f974677bb1ecedf85691b08fd47a`

P02 / J / ExtendedMaxZoom:
- source SHA-256: `3d1f46ce36068f111294a30bc7919d34e83e7cee99a0e6c0e343d5e30f934f05`
- exact-prefix payload SHA-256: `6af65519fce853eecad8e56cf5bc8aeb021f77b9cb7446b4eb11cfb3f15f607e`

P03 / K / enableQLL:
- source SHA-256: `094b7dbea8bca7eb1c94604fc72aa00926ba1d20cbe0c6d98005831b925bf694`
- exact-prefix payload SHA-256: `c45ee089e4e90a978c38b5ff151c42e1c67f1ac291ef2356da2e93a16aa2ab5b`

## Secondary P01 pixel-domain observation

P01 differs from P02/P03 at the **pixel-content level**, not at the route/topology level:

- P01 candidate payload min/max code: 0 / 1023;
- P01 has 109,304 zero-valued U16 samples inside band 0, so `contiguousPrefixAllSamplesNonZero=false`;
- P02 candidate payload min/max: 38 / 551 and all 12,533,760 payload samples are non-zero;
- P03 candidate payload min/max: 34 / 824 and all 12,533,760 payload samples are non-zero.

This is **not promoted to a vendor-key effect**. P01 was captured at ISO 102400 and ~70 ms, while P02/P03 used materially different auto-exposure states. The scenes/time also differ. A causal pixel-value claim would require a matched A/B control under fixed capture conditions.

It does not change the v0.38 route conclusion because the populated-prefix boundary, non-zero row extent, source-envelope geometry and unique 4080x3072 byte-count match are unchanged.

## Coarse result-side route indicators

Across all three runs, the physical Camera-5 result snapshot remains in the same coarse route class:

- HONOR `binningFactor = 4`;
- HONOR `isInSensorZoom = 0`;
- `sensorZoomRatio = 1.0054214`;
- `AECRealCropWindow` begins `[11,8,4058,3055]`;
- `allISPCropWindow = [0,0,16320,12288,0,0,16320,12288]`;
- `rawBinningFactorUsed = true`.

These names/values remain observations, not decoded vendor semantics.

## Bounded conclusion

The v0.36 physical-only cohort is now closed at numeric stimulus 1:

- H / inSensorZoomEnable / BYTE(1): attachment pass, capture pass, no topology differential (v0.37)
- I / EnableVSR / INT32(1): attachment pass, capture pass, no topology differential
- J / ExtendedMaxZoom / INT32(1): attachment pass, capture pass, no topology differential
- K / enableQLL / INT32(1): attachment pass, capture pass, no topology differential

Combined with v0.30/v0.31 and v0.33-v0.35, the evidence no longer supports continuing with blind vendor-key/value enumeration.

## Next decision

Move from **control-value search** to **capture-context geometry diagnosis**.

The device advertises three materially different RAW geometry contexts that can be tested without assigning vendor semantics:

1. standard RAW 4080x3072;
2. MAXIMUM_RESOLUTION RAW 8160x6144;
3. MAXIMUM_RESOLUTION high-resolution RAW 16320x12288.

The next experiment should compare these output contexts with no unknown vendor session intervention and the same source-first audit. This directly tests whether the 25,067,520-byte 4080x3072 payload is invariant across requested RAW envelope geometry, or whether a 50 MP context can populate a larger RAW domain.

This is a context test, not a new vendor-key sweep.
