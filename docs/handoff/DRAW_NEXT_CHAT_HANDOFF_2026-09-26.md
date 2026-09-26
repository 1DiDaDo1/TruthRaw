# D.RAW next-chat handoff — 2026-09-26

## Start here

Official product name: **D.RAW**

Repository:

`1DiDaDo1/TruthRaw`

Current active architecture branch:

`architecture/lens-independent-free-world-observation-v01-2026-09-26`

Latest Android-code branch:

`research/appearance-highlight-headroom-sweep-v02-2026-09-26`

Latest fully Android-validated code checkpoint:

`402c72d6804f6cc393a5eab93c9d94d0687be111`

Default GitHub `main` is **not current**. It still points to:

`514f2f4bde6aba5a6709e176c03b22c3b9aea912`

Read first:

1. `START_HERE_NEW_CHAT.md`
2. `state/CURRENT_PROJECT_STATE_2026-09-26.json`
3. `docs/DRAW_MAIN_PROJECT_STATE_2026-09-26.md`
4. `docs/DOCUMENT_STATUS_INDEX_2026-09-26.md`
5. `docs/CORE_VISION_LENS_INDEPENDENT_FREE_WORLD_OBSERVATION_ARCHITECTURE.md`
6. `docs/research/lens-independent-free-world-observation-contract-v0.1/README.md`
7. `docs/research/lens-independent-free-world-observation-contract-v0.1/DRAW_OBSERVATION_CONTRACT_v0_1.json`
8. `docs/research/lens-independent-free-world-observation-contract-v0.1/SEAL_MANIFEST_v0_1.json`
9. this handoff
10. `docs/research/truthnegative-n2-full-colour-candidate-v0.1/README.md`
11. `docs/research/truthnegative-center-excluded-neighborhood-v0.2/README.md`
12. `docs/research/truthnegative-center-excluded-spatial-audit-v0.2.1/README.md`
13. `docs/research/truthnegative-n2-confidence-field-v0.3/README.md`
14. `docs/research/truthnegative-n2-factored-confidence-state-v0.3.1/README.md`
15. `docs/research/truthnegative-appearance-highlight-detail-audit-v0.1/README.md`
16. `docs/research/truthnegative-appearance-highlight-headroom-sweep-v0.2/README.md`
17. `docs/research/truthnegative-appearance-highlight-headroom-sweep-v0.2/evidence/DEVICE_RESULT_LAMP_SCENE_2026-09-26.md`

## Sealed lens-independent architecture

Canonical short law:

> **One Free World. Many sealed observations. One evidence law.**

The Free World is lens-independent. Main, ultra-wide, telephoto and future
sources enter as separately sealed D.RAW Observations with their own Source
Capability Envelope.

TruthNegative is per observation lineage and remains raster-independent.
Multiple observations compose in the Free World Observation Graph rather than
being silently merged.

Zero-Line / TruthRange remains `T=log2(L/L0)`. Sharing the coordinate family
does not prove a shared radiometric gauge. A cross-observation gauge relation
must be explicitly admitted before radiometric equality/fusion is claimed.

Branch-sensitive scientific calculation remains Float64; canonical Float32
storage is allowed only where validated. Precision never creates evidence.

The v0.1 architecture bytes are SHA-256 sealed. Do not edit them in place.
Create a versioned successor for semantic change.

This architecture branch changes no validated pixel route or APK.

## Permanent scientific laws

- Source Evidence / Direct CFA is immutable.
- Scientific Master is not rewritten by N2 or appearance diagnostics.
- TruthNegative is not rewritten by N2 or appearance diagnostics.
- `physicalFrameCount=1` and `independentEvidenceCount=1` unless genuinely
  new physical evidence is separately admitted.
- CENSORED is a bound, not an invented exact latent radiance.
- UNKNOWN does not gain authority from display appearance.
- Appearance/display never writes back into scientific state.
- Resource/precision/display changes do not create evidence.
- Honor/GCam/computational output cannot determine D.RAW truth/calibration.
- Representation may exceed the source; knowledge claims may not exceed the
  evidence.

## Current N2 position

The last proven N2 chain is:

```text
Direct CFA
 -> N2 v0.1 source-site gate
 -> private Stage-2 copy
 -> reconstruction-support closure
 -> same measured-preserving F64 full-colour reconstruction
 -> exact Scientific-Master source-pixel baseline gate
 -> temporary appearance-only B
 -> A/B/delta Risk/Quality
```

Hard device requirements:

- `baseline-mismatch=0`;
- `protected-changed=0`;
- support-guard suppression may be nonzero;
- reported radius must match backend `requiredHalo()`.

The current backend reports radius `3 px`.

Do not remove the reconstruction-support closure. It exists because a protected
source CFA site can still have its reconstructed RGB output changed by a
neighboring allowed CFA correction.

## N2 predictor / confidence research

v0.2 center-excluded predictor:

- center cannot select its own support;
- directional/multiscale support is fail-closed;
- noise independence remains unproven.

v0.2.1:

- whole-frame spatial sidecar;
- exact v0.1 candidate parity gate.

v0.3:

- vector-valued Confidence Field;
- no scalar probability;
- no denoise-strength map;
- `promotion_eligible=false`.

v0.3.1:

- factored logical state;
- useful facts stay separate;
- no relaxed percentage thresholds;
- legacy FULLY_COHERENT class retained only for comparison.

N2 remains diagnostic / appearance-only research.

## Latest Appearance device evidence

A second real lamp scene is now recorded at:

`docs/research/truthnegative-appearance-highlight-headroom-sweep-v0.2/evidence/DEVICE_RESULT_LAMP_SCENE_2026-09-26.md`

For that scene:

- baseline 100/100: 941 exact collapsed distinct pairs, source CENSORED = 0;
- shoulder 90/100: 0 exact collapsed pairs, below-knee mapped changes = 0;
- gamut/display clamp remains 1058 for every tested shoulder.

This does not promote 90/100. It sharpens the next appearance-only A/B test
and separates luminance peak-collapse from the remaining gamut/display clamp.

## New device-proven Appearance finding

A real-device Appearance Highlight Detail v0.1 JSON is now retained by summary
and cryptographic identity at:

`docs/research/truthnegative-appearance-highlight-detail-audit-v0.1/evidence/DEVICE_RESULT_2026-09-26.md`

Uploaded JSON SHA-256:

`ff9ec10c6012cc75578b21c9c8df694817086552a0474022d512ccb19e95e6df`

For the existing PRO 192x145 preview:

- display reference white = 100 nit;
- display peak = 100 nit;
- `no_highlight_headroom=true`;
- 502 source samples are above reference white;
- all 502 map exactly to display peak;
- source CENSORED count = 0;
- 851 / 55,343 distinct neighboring source-luminance pairs collapse to the
  same peak;
- source/mapped gradient retention ~= 0.9676483;
- maximum hidden adjacent source difference ~= 18.9555 nit.

Strongest 16x16 tile:

`x=80,y=48`

- 206/256 above reference white;
- 206/256 mapped at peak;
- 383/480 distinct neighbor pairs peak-collapsed;
- source CENSORED = 0.

Interpretation:

**The current 100/100 PRO Appearance configuration has proven display-stage
many-to-one highlight collapse on this tested scene.**

This is separate from source CENSORED authority.

## Honor camera image

A normal Honor camera-app image of the bright exterior surface is useful only
as:

`VISUAL_REFERENCE_ONLY`

It is not scientific evidence, not calibration and not a target that D.RAW
must numerically imitate.

## New v0.2 Appearance experiment

`TruthNegative Appearance Highlight Headroom Sweep v0.2`

is now implemented and Android-green.

The normal PRO preview is unchanged.

All variants keep the physical SDR peak at 100 nit and move only the appearance
shoulder:

- 100/100 baseline;
- 90/100;
- 80/100;
- 70/100.

The same v0.7 Appearance resolver is used for every variant.

The sweep records collapse, mapped-gradient retention, gamut/display clamp,
source CENSORED separation and lower-range preservation.

It chooses no winner and cannot promote itself.

Validation:

- standalone GCC/Clang/ASan/UBSan run `36254075689`: SUCCESS;
- signed Android run `36254294042`: SUCCESS;
- validated code SHA:
  `402c72d6804f6cc393a5eab93c9d94d0687be111`.

APK:

- artifact ID `10910177012`;
- artifact name `draw-appearance-highlight-headroom-v02-debug-arm64`;
- APK bytes `6951735`;
- APK SHA-256
  `d46b40c7650ad0b1102438bc1a31b1c4f8ee7c4c14050af633c435c5b0405e8b`.

## Immediate next real-device step

Prefer the exact same admitted DNG as the v0.1 device evidence.

Press:

`Export Appearance Headroom Sweep v0.2 · JSON`

Then inspect all four variants.

First gate:

the 100/100 baseline should reproduce the same source/state behavior when the
same DNG and route are used.

Do **not** automatically choose the variant with the lowest collapse count.

Jointly inspect:

1. peak-collapse reduction;
2. gradient retention;
3. gamut/display clamp behavior;
4. `below_knee_changed`;
5. source CENSORED count;
6. later visual appearance in a parallel preview.

Only after the metrics support a candidate should a visual A/B appearance
preview be added.

The normal PRO output must remain unchanged until a separate promotion decision.

## Repository boundary

Do not merge the entire accumulated development line into default GitHub
`main` as a side effect of this experiment.

A default-main promotion should be a separate, deliberate repository operation
with branch/history review. Current project navigation must use the 2026-09-26
bootstrap above.
