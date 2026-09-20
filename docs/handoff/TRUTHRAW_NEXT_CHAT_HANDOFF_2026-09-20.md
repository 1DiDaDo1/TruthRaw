# TruthRaw next-chat handoff — 2026-09-20

**READ THIS FIRST.**

Active branch:

`integration/truthraw-suite-v0-81-output-acutance-v47k`

Current app:

`0.47-v0.81-output-acutance-v47k`

Latest fully green CI:

`35521878189`

CI head:

`daf7949561a077988e44a71acd9f64bfb323a618`

Latest APK:
- bytes: `6,463,957`
- SHA-256: `fa3cb7ef4fc3c64221e89c7a0c7e6da49bf3ce1061d75d988bf942faa8d2b250`
- artifact id: `10609071299`
- artifact ZIP SHA-256: `1c094d99f5f0d634354d71259bfa262511e53e6fa7f5bd91aa3b590aacecb936`

This is the current integration/research line, not a main/canonical promotion.

## Permanent law

> Measured where measured. Reconstructed where necessary. Never invented.

> Representation can exceed the source. Knowledge claims cannot exceed the evidence.

Source Evidence remains immutable. Every child state must preserve ancestry, authority, uncertainty/support and intervention history.

## Current validated chain

`v0.73 camera admission -> v0.74 Scientific-Master stripe performance -> v0.75 bit-exact 512 preview core -> v0.76 responsive camera UI -> v0.77 canonical ancestry -> v0.78 per-channel authority -> v0.79 bound uncertainty admission -> v0.80 canonical Adaptive Detail -> v0.81 canonical Output Acutance`

### v0.73

Real-device route demonstrated:

`Gebruik camera -> Camera-5 source-first capture -> sealed 16320x12288 envelope -> exact topology admission -> 4080x3072 processing DNG -> Main House -> finalized Scientific Preview -> PURE Float32`.

The 16320x12288 envelope is acquisition evidence, **not** a proven 200 MP Scientific Master.

### v0.74 / v0.75

Performance execution changed without changing scientific identity:
- Scientific Master SHA unchanged;
- exact L0 bits unchanged;
- Backplane unchanged;
- preview pixels/exposure/HDR state unchanged.

Theoretical logical source-call count on 4080x3072 dropped from 7680 toward ~336. Same-device wall/CPU timing benchmark remains open.

### v0.76

Responsive portrait/landscape camera UI + clear shutter. No camera science changed.

### v0.77

Schema:

`TruthRawCanonicalAncestry/0.77`

Format-neutral derivative ancestry binds:

`source -> Scientific Master -> Zero-Line -> scene-scale -> 180-byte Backplane -> Open Scene -> derivative raster -> role-mask`.

Restoration DNG/TIFF/EXR carry the same ancestry manifest.

PURE v0.63 remains frozen.

### v0.78

Schema:

`TruthRawOpenSceneChannelAuthority/0.78`

Per RGB channel:
- CALIBRATED_ESTIMATE
- RECONSTRUCTED
- CENSORED
- UNKNOWN

Uncertainty knowledge is a separate axis.

Current generic/camera-derived DNG:
- direct uncensored CFA = CALIBRATED_ESTIMATE;
- clipped direct CFA = CENSORED + source-code lower bound;
- missing channels = UNKNOWN;
- RECONSTRUCTED = 0.

### v0.79

Schema:

`TruthRawBoundUncertaintyAdmission/0.79`

Current decisions:
- Camera-5 derived processing DNG -> `BLOCKED_SOURCE_DOMAIN_MISMATCH`;
- generic imported DNG without source attestation -> `BLOCKED_NO_SOURCE_ATTESTATION`;
- exact historical v5.0g tele vendor-DNG class + exact backend/model assets -> `ELIGIBLE_TRACE_GATE_OPEN`.

Even the historical exact tele class does not reach ADMITTED because the exact accepted F64 reconstructed-quantity trace certificate is still absent.

A fake/nonzero trace hash cannot unlock the gate.

### v0.80

Schema/domain:

`TruthRawAdaptiveDetailBinding/0.80`

Canonical v4.7j Adaptive Detail replaced the legacy Advanced Detailed/Crisp appearance.

Important boundary:
- frozen v4.7i reconstruction translation unit retained;
- v4.7j adapter is float-bit exact against canonical v4.7j on GCC/Clang;
- ASan/UBSan PASS;
- detail is appearance/detail compensation only;
- Scientific Master, v0.78 authority and v0.79 uncertainty admission unchanged;
- no optical/sensor evidence claim.

### v0.81

Schema/domain:

`TruthRawOutputAcutanceBinding/0.81`

Canonical v4.7k is integrated strictly at the finite output boundary.

Exact order:

`final resized linear SDR base -> canonical v4.7k Output Acutance -> HDR gain rebase -> highlight shoulder -> sRGB OETF / ARGB presentation`

Critical falsified-and-fixed rule:

> Output acutance may re-express existing positive HDR transport, but may not create new HDR gain where upstream HDR gain was unity.

Therefore:
- upstream zero/no-HDR gain -> output gain remains exactly 1;
- censored support -> output gain remains exactly 1;
- only pre-existing positive HDR transport is rebased;
- gain remains bounded by the existing display maximum.

v4.7k does not touch RAW/CFA, Scientific Master, Open Scene authority, uncertainty admission, Restoration science or optical/sensor truth.

## Frozen / do not silently change

- immutable Source Evidence
- physicalFrameCount=1
- independentEvidenceCount=1
- PURE self-binding v0.63
- Zero-Line/L0
- scene-scale
- Technical Backplane
- Restoration algorithm v0.67
- canonical Open Scene parent v0.70
- role-mask projection v0.71
- projection lifecycle v0.72
- Camera-5 topology admission law
- frozen v4.7i scientific reconstruction core
- appearance never writes back
- counterfactual never evidence

## Historical v5.0g boundary

Prospective uncertainty PASS is valid only for the exact historical HONOR BKQ-N49 22.48 mm tele **vendor-DNG** source class and exact v4.7i backend/hash.

Do not transfer it to:
- Camera-5 derived processing DNG;
- MotionCam/direct-CFA;
- Main/Wide/Front;
- computational DNG;
- any different backend/hash/sample domain.

Exact historical feature replay is green. Exact F64 reconstructed-quantity trace binding is still open.

## Open validation / science

- real-device v0.81 Output Acutance/HDR-rebase smoke test
- same-device v0.74/v0.75 performance timing vs historical ~87.96 s
- independent DNG/TIFF/EXR conformance
- original-source replay for effectful Restoration
- current Camera-5-domain noise/PTC/uncertainty calibration
- historical v5.0g exact F64 trace certification
- 16320x12288 native/full-population scientific admission remains unproven and disallowed
- Nikon NEF Scientific Master promotion remains blocked pending uncertainty/noise, source-bound colour and held-out validation

## Immediate next branch

`integration/truthraw-suite-v0-82-illumination-state`

Objective:

Build a richer illumination state while keeping **illumination authority separate from image/channel authority**.

The v0.82 state should represent only what evidence supports:
- daylight/artificial-light indicators;
- CCT and Duv when derivable/bound;
- direction/spatial-extent evidence where actually observed;
- temporal modulation/flicker evidence when timing/row-time supports it;
- source metadata as metadata, not SPD proof;
- UNKNOWN where SPD, direction, geometry or temporal state is not evidenced.

Do **not** treat CCT as full SPD.

After v0.82:
1. authority-aware single-frame HDR driven by local v0.78 authority, censor bounds and admitted uncertainty;
2. Restoration v2 using local per-channel authority/support/uncertainty;
3. projection conformance + source replay;
4. current Camera-5 noise/PTC uncertainty campaign;
5. historical v5.0g F64 trace certification;
6. multi-vendor proprietary RAW promotion gates.

## Read next

1. `state/CURRENT_PROJECT_STATE_2026-09-20.json`
2. `docs/TRUTHRAW_V081_OUTPUT_ACUTANCE_V47K_2026-09-20.md`
3. `docs/TRUTHRAW_V080_ADAPTIVE_DETAIL_V47J_2026-09-20.md`
4. `docs/TRUTHRAW_V079_BOUND_UNCERTAINTY_ADMISSION_2026-09-20.md`
5. `docs/TRUTHRAW_V078_OPEN_SCENE_CHANNEL_AUTHORITY_2026-09-20.md`
6. `docs/research/output-acutance-v0.81/README.md`
7. `canonical/output-acutance/v4.7k/README_v4_7k.md`
8. `docs/research/adaptive-detail-v47j-adapter-v0.80/README.md`
