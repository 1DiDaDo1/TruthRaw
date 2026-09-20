# TruthRaw v0.80 — Canonical Adaptive Detail v4.7j — 2026-09-20

## Status

**CLOSED INTEGRATION — CI + Android APK PASS**

Integration branch:

`integration/truthraw-suite-v0-80-adaptive-detail-v47j`

Android version:

`0.46-v0.80-adaptive-detail-v47j`

v0.80 replaces the legacy Advanced Detailed/Crisp appearance backend with canonical v4.7j Adaptive Detail.

It does **not** change the scientific v4.7i reconstruction translation unit.

## Why the adapter exists

The canonical v4.7j combined native core contains:
- the same reconstruction implementation as v4.7i before the appearance section;
- an expanded appearance ABI carrying `AppearanceContext.noiseSigmaAt2Pct`;
- the new `AdaptiveDetailedCrispAppearance`.

The v4.7i source hashes are already part of historical uncertainty/backend bindings.

Therefore Android continues to compile the frozen:

`canonical/reconstruction/v4.7i/native/src/core.cpp`

and adds only:

`adaptive_detail_v47j_adapter.cpp`

which implements the old v4.7i `IAppearanceBackend` ABI.

This avoids changing the reconstruction/backend identity merely to obtain a newer appearance layer.

## Canonical v4.7j behavior

The adapter preserves the canonical v4.7j algorithm:
- micro band from radius-1 local mean;
- fine band from radius-1 vs radius-2;
- texture band from radius-2 vs radius-5;
- NoiseProfile-derived sigma at 2% signal;
- signal/noise confidence gating;
- local activity gating;
- non-semantic hard-edge guard;
- local 3x3 support limiter;
- luminance-only RGB scaling;
- no skin/semantic segmentation.

Required appearance halo: 5 pixels.

Backend name:

`adaptive_detailed_crisp_multiband_hard_edge_guard_v47j`

Color policy:

`luminance_only_rgb_direction_preserved_no_semantic_segmentation`

## Noise binding

The exact canonical v4.7j NoiseProfile scalar is derived once from source metadata:

`sigma_2pct = sqrt(mean_c(max(S_c * 0.02 + O_c, 0)))`

If no valid NoiseProfile exists, sigma=0 and the canonical v4.7j fallback confidence path is used.

The exact Float32 sigma bits are carried in Advanced provenance when Detail is enabled.

## Strong parity gate

The v0.80 research harness compiles:
1. the compatibility adapter against the frozen v4.7i ABI;
2. the actual canonical v4.7j C++ core in an isolated reference namespace.

The outputs are compared float-bit for float-bit for multiple:
- NoiseProfile sigma regimes;
- tile sizes;
- core positions;
- edge/texture patterns.

It also verifies that v4.7i and v4.7j `core.cpp` are byte-identical up to the start of the appearance section.

The standalone parity workflow has passed:
- GCC Release
- Clang Release
- Clang ASan/UBSan

## Advanced child binding

When Detail is enabled, Advanced binds:

`TruthRawAdaptiveDetailBinding/0.80`

to:
- canonical Open Scene v0.70 SHA;
- channel authority v0.78 SHA;
- uncertainty-admission decision v0.79 SHA;
- exact NoiseProfile sigma Float32 bits;
- canonical v4.7j backend/policy identity.

When Detail is disabled:
- detail backend code = 0;
- sigma payload = 0;
- detail binding SHA = all-zero / absent.

The Kotlin consumer rejects a mismatch.

## Authority boundary

Adaptive Detail is a downstream appearance/detail-compensation layer.

It cannot:
- modify Scientific Master;
- modify Zero-Line or scene-scale;
- modify Technical Backplane;
- upgrade v0.78 authority;
- change v0.79 uncertainty admission;
- turn UNKNOWN into RECONSTRUCTED;
- turn CENSORED into an exact value;
- create optical/sensor evidence;
- claim recovery of frequencies absent from physical evidence.

Any future **scientific detail authority** is a separate problem and requires local structure/support plus suitable optics/MTF evidence.

## Existing canonical v4.7j validation

The canonical package already records:
- 29/29 native unit/integration PASS;
- Python↔C++ parity PASS;
- four real RAW reconstruction regressions PASS;
- four real scene detail regressions PASS;
- people/skin guard PASS;
- synthetic MTF/halo/noise gates PASS.

It remains waiting for:
- Vulkan/SPIR-V implementation/parity;
- Magic8 Pro on-device validation.

## Next after v0.80

v4.7k Output Acutance stays separate and post-resize:

`final SDR resize -> output acutance -> recompute/derive HDR gain against final SDR base -> OETF/encode`.

It is not part of Scientific Master or optical-detail authority.


## Validation closure

Full integration CI run: `35520838931`

All green:
- GCC canonical v4.7j adapter
- Clang canonical v4.7j adapter
- Clang ASan/UBSan Adaptive Detail
- v0.78-v0.79 authority and uncertainty gates
- existing scientific lineage contracts
- Android v0.80 Adaptive Detail

Artifact:
- GitHub artifact id: `10608565903`
- artifact ZIP SHA-256: `169d618560a3f0ba900d0669f496b1898df524d6390348d4d6ad5dcceacccfcd`
- APK bytes: `6,449,825`
- APK SHA-256: `3523a34d3188eb13567e95508fa1aedfe0aee585609a73c93cfda24e790ee4bb`

v0.80 is therefore the current closed integration baseline for Advanced Detail.
