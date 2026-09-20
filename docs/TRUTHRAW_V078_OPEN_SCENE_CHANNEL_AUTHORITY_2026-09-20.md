# TruthRaw v0.78 — Open Scene Channel Authority — 2026-09-20

## Status

Active integration branch:

`integration/truthraw-suite-v0-78-open-scene-channel-authority`

Android version:

`0.44-v0.78-open-scene-channel-authority`

CI run:

`35519235748`

Result:

**SUCCESS**

Green jobs:
- Channel authority exactness
- Existing scientific lineage contracts
- Android v0.78 channel authority

APK artifact:
- artifact id: `10608121004`
- artifact ZIP SHA-256: `f7125be3edbbaf4017fc1fee6aa57e7d384c16e1d9ab4a288ab1400eebc17ae1`
- APK bytes: `6,407,245`
- APK SHA-256: `7a02fc07472158003423a1232a2c320956c3f80b7ff02d94c303fa46a1e181ff`

## What v0.77 added underneath

v0.77 introduced `TruthRawCanonicalAncestry/0.77`.

The format-neutral ancestry graph binds:

`sealed source evidence -> Scientific Master -> Zero-Line -> scene-scale -> 180-byte Technical Backplane -> canonical Open Scene -> derivative raster -> restoration role-mask`.

The same ancestry manifest is carried by Restoration DNG, TIFF and EXR.

This closes the earlier provenance asymmetry where DNG carried richer lineage than TIFF/EXR.

PURE v0.63 is intentionally untouched.

## v0.78 scientific change

v0.78 adds an immutable child authority sidecar to canonical Open Scene v0.70.

Authority and uncertainty knowledge are now separate axes.

Per RGB channel the authority sidecar can represent:
- `CALIBRATED_ESTIMATE`
- `RECONSTRUCTED`
- `CENSORED`
- `UNKNOWN`

Uncertainty knowledge can independently be:
- `UNRESOLVED`
- `SOURCE_NOISE_PROFILE_P95`
- `BACKEND_BOUND_P95`
- `CERTIFIED_VARIANCE`

### Current generic admitted-DNG policy

For generic/imported or current camera-derived DNG:
- direct uncensored CFA channel -> `CALIBRATED_ESTIMATE`, support=1, uncertainty=`UNRESOLVED`
- direct clipped CFA channel -> `CENSORED` with explicit source-RAW-code lower bound
- two missing RGB channels -> `UNKNOWN`
- `RECONSTRUCTED` count -> **0**

This is deliberate.

No reconstructed authority is permitted without:
1. an admitted uncertainty-binding SHA-256;
2. the exact reconstruction backend identity;
3. finite nonnegative p95 uncertainty;
4. positive finite support;
5. source/backend-domain compatibility.

Therefore v0.78 improves the state representation without silently upgrading knowledge claims.

## Advanced binding

Advanced now exports two immutable Open Scene identities:
1. canonical Open Scene v0.70 artifact SHA-256;
2. channel-authority v0.78 artifact SHA-256.

The v0.78 identity is a child of the exact v0.70 identity.

Advanced remains appearance/derivative-only and cannot:
- modify the Scientific Master;
- create new evidence;
- turn UNKNOWN into measurement;
- turn CENSORED into an exact latent value;
- promote reconstructed channels without admitted uncertainty.

## Frozen science retained

Unchanged:
- `TRUTHRAW_PURE_SELF_BINDING_V0_63`
- Zero-Line / L0 semantics
- scene-scale
- Technical Backplane
- physical frame / independent evidence = 1/1
- Restoration v0.67 algorithm
- canonical Open Scene v0.70 parent semantics
- role-mask projection v0.71
- projection lifecycle v0.72
- Camera-5 source-first/topology-admission rules
- TN-3 remains downstream only

## v5.0g note

The repository contains a prospective tele uncertainty PASS for an exact historical HONOR BKQ-N49 4080x3072 vendor-DNG source class plus v4.7i.

The exact historical v5.0g feature semantics replay is green in v0.78 CI.

That result is **not automatically transferable** to the current v0.73+ camera-derived DNG container.

The F64 reconstruction-trace/source-domain binding remains a promotion gate.

## Still open

1. Real-device v0.78 smoke test on current camera/import paths.
2. Admit a source/backend-bound uncertainty model before allowing any `RECONSTRUCTED` channel authority.
3. Complete exact F64 reconstruction-trace binding for the historical v5.0g tele domain.
4. Build a separate uncertainty/PTC calibration path for the current camera-derived DNG domain; never copy the historical tele model by assumption.
5. Integrate v4.7j detail as support/authority-aware scientific reconstruction, not generic sharpening.
6. Integrate v4.7k output acutance only downstream of scientific detail and never as measured-detail authority.
7. Build richer illumination/light-state separation for daylight, darkness and artificial illumination.
8. Upgrade single-frame HDR to consume local channel authority, censor bounds and uncertainty rather than appearance-only heuristics.
9. Restoration v2 must consume local authority/support/uncertainty and keep aesthetic reintegration retreatable.
10. Close independent DNG/TIFF/EXR conformance and original-source restoration replay.
11. Continue multi-vendor RAW admission: Nikon NEF still requires uncertainty/noise, source-bound color and held-out validation before Scientific Master promotion.
12. The 16320x12288 Camera-5 envelope remains acquisition evidence only; full-population/native-sensor scientific admission remains unproven.

## Immediate next integration

Recommended next branch:

`integration/truthraw-suite-v0-79-bound-uncertainty-admission`

Goal:

Create the exact bridge from source-bound uncertainty evidence to v0.78 channel authority. It must fail closed for the current camera-derived DNG until its own model is calibrated and validated.
