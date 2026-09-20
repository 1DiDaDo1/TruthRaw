# TruthRaw next-chat handoff — 2026-09-20

**READ THIS FIRST.**

Active branch:

`integration/truthraw-suite-v0-80-adaptive-detail-v47j`

Current app:

`0.46-v0.80-adaptive-detail-v47j`

Latest fully green CI:

`35520838931`

Latest APK:
- bytes: `6,449,825`
- SHA-256: `3523a34d3188eb13567e95508fa1aedfe0aee585609a73c93cfda24e790ee4bb`
- artifact id: `10608565903`
- artifact ZIP SHA-256: `169d618560a3f0ba900d0669f496b1898df524d6390348d4d6ad5dcceacccfcd`

This is the current integration/research line, not a main/canonical promotion.

## Permanent law

> Measured where measured. Reconstructed where necessary. Never invented.

> Representation can exceed the source. Knowledge claims cannot exceed the evidence.

Source Evidence remains immutable. Every child state must preserve ancestry, authority, uncertainty/support and intervention history.

## Current validated chain

`v0.73 camera admission -> v0.74 Scientific-Master stripe performance -> v0.75 bit-exact 512 preview core -> v0.76 responsive camera UI -> v0.77 canonical ancestry -> v0.78 per-channel authority -> v0.79 bound uncertainty admission`

### v0.73

Real-device route demonstrated:

`Gebruik camera -> Camera-5 source-first capture -> sealed 16320x12288 envelope -> exact topology admission -> 4080x3072 processing DNG -> Main House -> finalized Scientific Preview -> PURE Float32`.

The 16320x12288 envelope is **not** a proven 200 MP Scientific Master.

### v0.74 / v0.75

Performance execution was changed without changing scientific identity:
- Scientific Master SHA unchanged;
- exact L0 bits unchanged;
- Backplane unchanged;
- preview pixels/exposure/HDR state unchanged.

Theoretical logical source-call count for 4080x3072 fell from 7680 toward ~336. Same-device timing benchmark is still open.

### v0.76

Portrait/landscape camera UI + clear shutter. No camera science changed.

### v0.77

Schema:

`TruthRawCanonicalAncestry/0.77`

Format-neutral ancestry binds:

`source -> Scientific Master -> Zero-Line -> scene-scale -> 180-byte Backplane -> Open Scene -> derivative raster -> role-mask`.

Restoration DNG/TIFF/EXR carry the same ancestry manifest.

PURE v0.63 remains untouched.

### v0.78

Schema:

`TruthRawOpenSceneChannelAuthority/0.78`

Per RGB channel:
- CALIBRATED_ESTIMATE
- RECONSTRUCTED
- CENSORED
- UNKNOWN

Uncertainty knowledge is a separate axis.

Current generic/camera-derived DNG remains:
- direct uncensored CFA = CALIBRATED_ESTIMATE
- clipped direct CFA = CENSORED + explicit source-code lower bound
- missing channels = UNKNOWN
- RECONSTRUCTED = 0

### v0.79

Schema:

`TruthRawBoundUncertaintyAdmission/0.79`

This is the gate between uncertainty evidence and v0.78 authority.

Current decisions:
- Camera-5 derived processing DNG -> `BLOCKED_SOURCE_DOMAIN_MISMATCH`
- generic imported DNG without source attestation -> `BLOCKED_NO_SOURCE_ATTESTATION`
- exact historical v5.0g tele vendor-DNG class + exact backend/model assets -> `ELIGIBLE_TRACE_GATE_OPEN`

Even the historical exact tele class does **not** reach ADMITTED because the accepted exact F64 reconstructed-quantity trace certificate is still absent.

A fake/nonzero trace hash cannot unlock it.

Advanced now binds three immutable identities:
1. canonical Open Scene v0.70 SHA;
2. channel authority v0.78 SHA;
3. uncertainty-admission decision v0.79 SHA.

Current `reconstructedAuthorityAllowed=false` is mandatory.

## Historical v5.0g boundary

Prospective uncertainty PASS is valid only for the exact old HONOR BKQ-N49 22.48 mm tele **vendor-DNG** source class and exact v4.7i backend/hash.

Do not transfer it to:
- Camera-5 derived processing DNG;
- MotionCam/direct-CFA;
- Main/Wide/Front;
- Google/computational DNG;
- any different backend/hash/sample domain.

The exact historical feature semantics replay is green. The exact F64 reconstructed-quantity trace binding is still open.

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
- appearance never writes back
- counterfactual never evidence

## Important detail correction

Canonical v4.7j is an **appearance/detail-compensation** layer. It is validated to preserve the Scientific Master and chromaticity, but it does not create new optical/sensor detail authority.

Therefore the next app integration may replace the old Detailed/Crisp appearance with v4.7j, but:
- Scientific Master stays unchanged;
- v0.78 authority stays unchanged;
- v0.79 admission stays unchanged;
- output must not claim recovered optical frequencies.

Any future scientific-detail authority needs separate structure/support and optical/MTF evidence.

v4.7k remains later and strictly post-resize output acutance.

## v0.80 closure

Canonical v4.7j Adaptive Detail is now the closed Advanced detail baseline.

Important:
- exact adapter output parity with canonical v4.7j on GCC + Clang;
- ASan/UBSan PASS;
- frozen v4.7i scientific reconstruction core retained;
- v0.78 authority unchanged;
- v0.79 uncertainty admission unchanged;
- detail child binding is appearance-only and carries exact NoiseProfile sigma bits;
- no recovered optical/sensor information claim.

## Immediate next branch

`integration/truthraw-suite-v0-81-output-acutance-v47k`

Objective:
integrate canonical v4.7k strictly after final preview resize and before final display encoding, with HDR gain recomputed/derived against the acutance-adjusted SDR base.

Critical ordering:

`resized linear SDR base -> v4.7k output acutance -> final HDR gain relation -> OETF/ARGB presentation`

Do not apply v4.7k to Scientific Master, RAW/CFA, reconstruction, Open Scene authority or restoration state.

## Read next

1. `state/CURRENT_PROJECT_STATE_2026-09-20.json`
2. `docs/TRUTHRAW_V079_BOUND_UNCERTAINTY_ADMISSION_2026-09-20.md`
3. `docs/research/bound-uncertainty-admission-v0.79/README.md`
4. `docs/TRUTHRAW_V078_OPEN_SCENE_CHANNEL_AUTHORITY_2026-09-20.md`
5. `docs/research/open-scene-channel-authority-v0.78/README.md`
6. `docs/research/canonical-ancestry-spine-v0.77/README.md`
7. `canonical/detail/v4.7j/README_v4_7j.md`
8. `canonical/output-acutance/v4.7k/README_v4_7k.md`
