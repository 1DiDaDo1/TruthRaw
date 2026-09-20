# TruthRaw v0.83 — HDR Authority Contract — 2026-09-20

## Status

**CLOSED INTEGRATION — CI + Android APK PASS**

Integration branch:

`integration/truthraw-suite-v0-83-authority-aware-hdr`

Android version:

`0.49-v0.83-authority-aware-hdr`

Schema:

`TruthRawHdrAuthorityContract/0.83`

v0.83 separates presentation HDR from scientifically admitted HDR gain.

It does not initially change the current HDR pixels/gain-map math.

## Current result

For the current generic/imported and Camera-5-derived DNG routes:

- scientific HDR authority = `BLOCKED`;
- blocked reason = `NO_PER_OUTPUT_CHANNEL_AUTHORITY`;
- Natural HDR on -> presentation authority = `APPEARANCE_ONLY`;
- Natural HDR off -> presentation authority = `DISABLED`;
- scientific gain allowed = false.

This is intentional.

## Why scientific HDR remains blocked

v0.78 already records the source-side RGB authority field, but Advanced does not yet have a canonical per-output-channel authority map after reconstruction + resize + presentation transforms.

Without that local output mapping, a gain value cannot be called scientific merely because the source contained some calibrated channels.

Current generic/camera-DNG also still has:
- RECONSTRUCTED = 0;
- UNKNOWN missing RGB channels;
- no admitted reconstructed-channel uncertainty through v0.79.

Therefore presentation HDR remains useful, but it is not promoted to scientific radiance recovery.

## Scientific admission contract

Scientific HDR gain requires all of:

1. canonical per-output-channel authority available;
2. no gain-driving UNKNOWN channel;
3. every gain-driving RECONSTRUCTED channel has admitted uncertainty;
4. CENSORED support is excluded from exact-value gain;
5. all source/master/Open Scene/authority/uncertainty/illumination identities are bound.

Future permitted scientific states:
- `DIRECT_EVIDENCE_BOUND`;
- `RECONSTRUCTION_UNCERTAINTY_BOUND`.

These are testable but not the current runtime result.

## Censoring

CENSORED remains an inequality/bound.

v0.83 always keeps:

`censoredExactRecoveryAllowed=false`

v0.81 already keeps censored output gain at unity. v0.83 records that as a prerequisite rather than relabeling the censored latent value.

## UNKNOWN

`unknownHeadroomAllowed=false`

An UNKNOWN channel cannot acquire scientific HDR headroom from:
- tone mapping;
- gain maps;
- CCT/Duv;
- presentation brightness;
- counterfactual relighting;
- another camera/readout model.

## Illumination boundary

v0.82 illumination state is context only.

Even a future calibrated SPD cannot bypass:
- channel authority;
- censor bounds;
- uncertainty admission;
- output-channel mapping.

Therefore:

`illuminationCreatesHeadroom=false`

## Presentation HDR

Presentation HDR remains allowed as an appearance transform.

It is explicitly represented as:

`APPEARANCE_ONLY`

This makes the current product behavior usable without conflating display enhancement with evidence-supported radiance recovery.

## Ancestry

The v0.83 state binds:
- source evidence SHA;
- Scientific Master SHA;
- canonical Open Scene v0.70 SHA;
- v0.78 channel-authority SHA;
- v0.79 uncertainty-admission SHA;
- v0.82 illumination-state SHA;
- channel-authority counts;
- output pixel count;
- presentation HDR gain-pixel count;
- censor-suppression state;
- evidence count 1/1.

## Standalone falsification

The host tests require:
- current runtime -> BLOCKED / NO_PER_OUTPUT_CHANNEL_AUTHORITY;
- known illumination cannot promote HDR authority;
- UNKNOWN blocks scientific gain;
- RECONSTRUCTED requires admitted uncertainty;
- unsuppressed CENSORED blocks scientific gain;
- direct-evidence-only future path can admit;
- reconstruction+uncertainty future path can admit;
- HDR-disabled state rejects stale non-zero gain pixels;
- invalid ancestry/evidence count fails closed;
- deterministic state identity.

## Next gate

The next HDR step should construct a canonical **per-output-channel authority map** through reconstruction and resize.

Only after that map exists may local scientific HDR gain be attempted.

Current v0.83 does not claim that gate is closed.


## Validation closure

Full integration CI run:

`35523343893`

All green:
- GCC HDR authority contract
- Clang HDR authority contract
- Clang ASan/UBSan HDR authority contract
- v0.82 illumination state
- v0.81 canonical Output Acutance
- v0.80 canonical Adaptive Detail parity
- v0.78-v0.79 authority and uncertainty gates
- existing scientific lineage contracts
- Android v0.83 HDR Authority
- APK verification
- artifact upload

Artifact:
- GitHub artifact id: `10609178219`
- artifact ZIP SHA-256: `5a4112454d163a233072a7525f29a75de62fe43f69a7f50d2e35a619cad1822f`
- APK bytes: `6,498,017`
- APK SHA-256: `88d8ac29b64ddef67668c026c05711974c645e3fc1ef6da4b8fb942bda4066a7`
- CI head: `6c86a321eea44028f5d6cebbe99c0ff7df99159e`

v0.83 is therefore the current closed integration baseline.

## Immediate next integration

Recommended next branch:

`integration/truthraw-suite-v0-84-output-channel-authority-map`

Goal:
construct a canonical per-output-channel authority/support map through reconstruction and preview resampling, without changing HDR gain.

This is the missing scientific gate identified by v0.83.

The map must:
- start from v0.78 per-channel source/Open-Scene authority;
- preserve CENSORED as bounds;
- preserve UNKNOWN as unknown;
- admit RECONSTRUCTED only where v0.79 uncertainty is admitted;
- propagate support through resize without treating compute tiles as authority regions;
- never infer authority from v0.82 CCT/Duv;
- remain independent from v0.80/v0.81 appearance/output acutance.

Only after v0.84 exists may v0.83's `perOutputChannelAuthorityAvailable` legitimately become true.
