# TruthRaw next-chat handoff — 2026-09-20

**READ THIS FIRST.**

Active branch:

`integration/truthraw-suite-v0-82-illumination-state`

Current app:

`0.48-v0.82-illumination-state`

Latest fully green CI:

`35522724167`

CI head:

`86f92217a700a52bb26583d1f967a072e3e4c660`

Latest APK:
- bytes: `6,491,089`
- SHA-256: `9dd30ca31c810e71080ae7bdb512e2c3145bb4f5eaadb48ff0e95fdacea1e6fb`
- artifact id: `10609012748`
- artifact ZIP SHA-256: `5875b9f72670fd87c153e0193b6f598d8c4601906b34c2e80e432c1dd71a2b6b`

This is the current integration/research line, not a main/canonical promotion.

## Permanent law

> Measured where measured. Reconstructed where necessary. Never invented.

> Representation can exceed the source. Knowledge claims cannot exceed the evidence.

Source Evidence remains immutable. Every child state preserves ancestry, authority, uncertainty/support and intervention history.

## Current validated chain

`v0.73 camera admission -> v0.74 Scientific-Master stripe performance -> v0.75 bit-exact preview core -> v0.76 responsive camera UI -> v0.77 canonical ancestry -> v0.78 per-channel authority -> v0.79 bound uncertainty admission -> v0.80 canonical Adaptive Detail -> v0.81 canonical Output Acutance -> v0.82 observed illumination state`

## v0.82 closure

Schema:

`TruthRawObservedIlluminationState/0.82`

Purpose:

Keep illumination authority separate from RGB/channel authority.

Current admitted knowledge:
- for dual-illuminant DNG color binding where the existing source-bound producer exposes resolved source white, v0.82 records x/y, CCT and a signed CIE-1960 uv Planckian-locus polyline Duv estimate;
- authority for those coordinates is `SOURCE_METADATA_BOUND_ESTIMATE`;
- delegated single-illuminant paths remain white-point UNKNOWN because the current public producer audit does not expose a bound resolved source white.

Current explicit UNKNOWN:
- DAYLIGHT / ARTIFICIAL / MIXED classification;
- SPD;
- physical light direction;
- spatial/angular extent;
- temporal modulation/flicker.

Hard non-claims:
- CCT is not SPD;
- DNG CalibrationIlluminant1/2 are profile calibration references, not proof of scene illumination;
- warm/cool CCT does not classify daylight/artificial;
- missing timing evidence cannot become flicker evidence.

v0.82 state binds:
- source evidence SHA;
- Scientific Master SHA;
- canonical Open Scene v0.70 SHA;
- sourceEvidenceId;
- colorBindingId;
- exact white-point coordinates when known;
- profile calibration-reference codes;
- evidence count 1/1.

Always:
- createsNewEvidence=false;
- scientificMasterModified=false;
- channelAuthorityModified=false;
- counterfactual=false.

## Previous closed boundaries

### v0.81 Output Acutance

Canonical v4.7k is strictly post-final-resize.

`final resized linear SDR -> v4.7k -> HDR rebase -> shoulder -> OETF/presentation`

Output acutance can re-express only existing positive HDR transport. It cannot create HDR gain where upstream gain was unity. Censored support remains gain 1.

### v0.80 Adaptive Detail

Canonical v4.7j is appearance/detail compensation only. Frozen v4.7i Scientific-Master reconstruction remains unchanged. No optical/sensor detail authority is created.

### v0.79 uncertainty admission

Current Camera-5 processing DNG: `BLOCKED_SOURCE_DOMAIN_MISMATCH`.

Generic import without source attestation: `BLOCKED_NO_SOURCE_ATTESTATION`.

Exact historical v5.0g vendor-DNG tele domain: `ELIGIBLE_TRACE_GATE_OPEN`, not ADMITTED.

Therefore current generic/camera-derived DNG still has no admitted reconstructed-channel uncertainty.

### v0.78 channel authority

Current generic/camera-derived DNG:
- direct uncensored CFA = CALIBRATED_ESTIMATE;
- direct clipped CFA = CENSORED + explicit source-code lower bound;
- missing RGB channels = UNKNOWN;
- RECONSTRUCTED = 0.

## Frozen / do not silently change

- immutable Source Evidence
- physicalFrameCount=1
- independentEvidenceCount=1
- PURE self-binding v0.63
- Zero-Line/L0
- scene-scale
- Technical Backplane
- frozen v4.7i Scientific-Master reconstruction
- Restoration algorithm v0.67
- canonical Open Scene parent v0.70
- role-mask projection v0.71
- projection lifecycle v0.72
- Camera-5 topology admission law
- appearance never writes back
- counterfactual never evidence
- 16320x12288 Camera-5 envelope is not automatically 200 MP Scientific Master

## Immediate next branch

`integration/truthraw-suite-v0-83-authority-aware-hdr`

Objective:

Build a local single-frame HDR authority contract using:
- Scientific Master;
- v0.78 per-channel authority;
- explicit CENSORED bounds;
- v0.79 admitted uncertainty where available;
- v0.82 illumination state as context only.

Hard rules:
- CENSORED remains a bound, never an exact latent value;
- UNKNOWN never gains HDR headroom;
- CCT/Duv cannot create HDR authority;
- no admitted uncertainty means no reconstructed-channel HDR promotion;
- one physical frame remains one independent evidence item.

The first v0.83 implementation should **not** increase current Camera-5 authority. It should make the current limits explicit and make later promotion possible only when real uncertainty/calibration evidence arrives.

After v0.83:
1. Restoration v2 using local per-channel authority/support/uncertainty;
2. independent DNG/TIFF/EXR conformance + source replay;
3. current Camera-5 noise/PTC uncertainty campaign;
4. historical v5.0g F64 trace certification;
5. multi-vendor proprietary RAW promotion gates.

## Open real-device validation

- v0.81 Output Acutance/HDR-rebase smoke test;
- v0.82 illumination-state smoke test;
- v0.82 dual-illuminant white-point smoke test when such a source is available;
- same-device v0.74/v0.75 performance timing;
- projection conformance/source replay.

## Read next

1. `state/CURRENT_PROJECT_STATE_2026-09-20.json`
2. `docs/TRUTHRAW_V082_ILLUMINATION_STATE_2026-09-20.md`
3. `docs/research/illumination-state-v0.82/README.md`
4. `docs/TRUTHRAW_V081_OUTPUT_ACUTANCE_V47K_2026-09-20.md`
5. `docs/TRUTHRAW_V080_ADAPTIVE_DETAIL_V47J_2026-09-20.md`
6. `docs/TRUTHRAW_V079_BOUND_UNCERTAINTY_ADMISSION_2026-09-20.md`
7. `docs/TRUTHRAW_V078_OPEN_SCENE_CHANNEL_AUTHORITY_2026-09-20.md`
