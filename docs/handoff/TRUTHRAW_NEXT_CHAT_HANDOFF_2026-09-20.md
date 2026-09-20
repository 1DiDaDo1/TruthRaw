# TruthRaw next-chat handoff — 2026-09-20

**READ THIS FIRST.**

Active branch:

`integration/truthraw-suite-v0-83-authority-aware-hdr`

Current app:

`0.49-v0.83-authority-aware-hdr`

Latest fully green CI:

`35523343893`

CI head:

`6c86a321eea44028f5d6cebbe99c0ff7df99159e`

Latest APK:
- bytes: `6,498,017`
- SHA-256: `88d8ac29b64ddef67668c026c05711974c645e3fc1ef6da4b8fb942bda4066a7`
- artifact id: `10609178219`
- artifact ZIP SHA-256: `5a4112454d163a233072a7525f29a75de62fe43f69a7f50d2e35a619cad1822f`

This is the current integration/research line, not a main/canonical promotion.

## Permanent law

> Measured where measured. Reconstructed where necessary. Never invented.

> Representation can exceed the source. Knowledge claims cannot exceed the evidence.

Source Evidence remains immutable. Every child state preserves ancestry, authority, uncertainty/support and intervention history.

## Current validated chain

`v0.73 camera admission -> v0.74 Scientific-Master stripe performance -> v0.75 bit-exact preview core -> v0.76 responsive camera UI -> v0.77 canonical ancestry -> v0.78 per-channel authority -> v0.79 bound uncertainty admission -> v0.80 canonical Adaptive Detail -> v0.81 canonical Output Acutance -> v0.82 observed illumination state -> v0.83 HDR authority contract`

## v0.83 closure

Schema:

`TruthRawHdrAuthorityContract/0.83`

v0.83 separates two meanings that previously shared one UI control:

1. **presentation HDR** — allowed as appearance/output;
2. **scientific HDR gain** — only allowed when local output authority supports it.

Current generic/imported and Camera-5-derived runtime result:

- scientific HDR authority = `BLOCKED`;
- blocked reason = `NO_PER_OUTPUT_CHANNEL_AUTHORITY`;
- Natural HDR ON -> `APPEARANCE_ONLY`;
- Natural HDR OFF -> `DISABLED`;
- scientificGainAllowed=false;
- censoredExactRecoveryAllowed=false;
- unknownHeadroomAllowed=false;
- illuminationCreatesHeadroom=false.

The actual existing HDR pixels/gain math are unchanged by v0.83.

v0.83 binds:
- source evidence SHA;
- Scientific Master SHA;
- canonical Open Scene SHA;
- v0.78 channel-authority SHA;
- v0.79 uncertainty-admission SHA;
- v0.82 illumination-state SHA;
- authority counts;
- output-pixel count;
- presentation HDR gain-pixel count;
- censor-suppression state;
- physical/evidence count 1/1.

Future scientific admission states exist in the contract:
- `DIRECT_EVIDENCE_BOUND`;
- `RECONSTRUCTION_UNCERTAINTY_BOUND`.

They are not the current runtime result.

## Why scientific HDR is still blocked

v0.78 has source/Open-Scene per-channel authority, but Advanced does not yet have a canonical mapping from those source/channel records to each final output RGB channel after reconstruction and resampling.

Without that mapping, presentation gain cannot be relabeled scientific gain.

Current generic/camera DNG also retains:
- missing RGB channels UNKNOWN;
- RECONSTRUCTED=0;
- no admitted reconstructed uncertainty through v0.79.

## v0.82 illumination boundary retained

Source-bound CCT/Duv is context only.

CCT/Duv, profile illuminant codes, future SPD or appearance relighting cannot bypass:
- RGB/channel authority;
- censor bounds;
- uncertainty admission;
- output mapping.

`illuminationCreatesHeadroom=false` remains mandatory.

## v0.81/v0.80 retained

v0.81:
`final resize -> canonical v4.7k -> existing HDR rebase -> shoulder -> OETF`.

Zero upstream HDR remains unity; censored support remains gain 1.

v0.80:
canonical v4.7j Adaptive Detail is appearance/detail compensation only; frozen v4.7i Scientific-Master reconstruction remains unchanged.

## v0.79/v0.78 retained

v0.79:
- Camera-5 derived processing DNG -> `BLOCKED_SOURCE_DOMAIN_MISMATCH`;
- generic unattested import -> `BLOCKED_NO_SOURCE_ATTESTATION`;
- exact historical v5.0g tele vendor-DNG -> `ELIGIBLE_TRACE_GATE_OPEN`, not ADMITTED.

v0.78 current generic/camera DNG:
- direct uncensored CFA = CALIBRATED_ESTIMATE;
- direct clipped CFA = CENSORED + bound;
- missing channels = UNKNOWN;
- RECONSTRUCTED=0.

## Frozen / do not silently change

- immutable Source Evidence
- physicalFrameCount=1
- independentEvidenceCount=1
- PURE v0.63
- Zero-Line/L0
- scene-scale
- Technical Backplane
- frozen v4.7i Scientific-Master reconstruction
- Restoration v0.67
- canonical Open Scene v0.70 parent
- role-mask projection v0.71
- projection lifecycle v0.72
- Camera-5 topology admission
- appearance never writes back
- counterfactual never evidence
- 16320x12288 Camera-5 envelope is not proven 200 MP Scientific Master

## Immediate next branch

`integration/truthraw-suite-v0-84-output-channel-authority-map`

Objective:

Construct a canonical per-output-channel authority/support map through reconstruction and preview resampling **without changing HDR gain yet**.

Hard rules:
- CENSORED remains a bound;
- UNKNOWN remains unknown;
- RECONSTRUCTED requires v0.79 admitted uncertainty;
- resampling may combine support but cannot upgrade authority;
- compute tiles are execution geometry, never authority regions;
- v0.82 illumination context cannot create RGB authority;
- v0.80/v0.81 appearance/output processing cannot write authority backwards.

Only after v0.84 can `perOutputChannelAuthorityAvailable` legitimately become true in v0.83.

After v0.84:
1. admit scientific HDR gain only where the v0.84 output map permits;
2. Restoration v2 with local authority/support/uncertainty;
3. DNG/TIFF/EXR conformance + original-source replay;
4. current Camera-5 noise/PTC uncertainty calibration;
5. historical v5.0g F64 trace certification;
6. multi-vendor proprietary RAW promotion gates.

## Open real-device validation

- v0.81 Output Acutance/HDR-rebase smoke test;
- v0.82 illumination-state smoke test;
- v0.83 presentation/scientific HDR separation smoke test;
- same-device v0.74/v0.75 performance timing;
- projection conformance/source replay.

## Read next

1. `state/CURRENT_PROJECT_STATE_2026-09-20.json`
2. `docs/TRUTHRAW_V083_HDR_AUTHORITY_2026-09-20.md`
3. `docs/research/hdr-authority-v0.83/README.md`
4. `docs/TRUTHRAW_V082_ILLUMINATION_STATE_2026-09-20.md`
5. `docs/TRUTHRAW_V081_OUTPUT_ACUTANCE_V47K_2026-09-20.md`
6. `docs/TRUTHRAW_V080_ADAPTIVE_DETAIL_V47J_2026-09-20.md`
7. `docs/TRUTHRAW_V079_BOUND_UNCERTAINTY_ADMISSION_2026-09-20.md`
