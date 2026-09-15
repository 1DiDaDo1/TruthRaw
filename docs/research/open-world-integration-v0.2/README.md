# TruthRaw Open-World Integration v0.2

Status: **RESEARCH — NOT MAIN-PROMOTED**

This phase connects the five v0.1 open-world foundations to concrete TruthRaw execution boundaries without changing canonical reconstruction/detail code or the immutable Direct-CFA source.

## Scope rule

The source evidence remains sealed against rewriting. The reconstructed scene/world is not enclosed by the sealed-house metaphor and may span arbitrary open-world domains.

The integration invariant is:

> **Source evidence is immutable; world representation is open; authority never upgrades silently.**

## 1. Illumination -> CICM / Room Capsule authority bridge

`tools/open_world_integration_v02.py` adds `dispatch_illumination()`.

Consumers are explicit:

- `SCIENTIFIC_SCENE`
- `CICM_RELATIVE_WORLD`
- `CICM_CALIBRATED_FORWARD`
- `ROOM_CAPSULE_RELATIVE`

A `COUNTERFACTUAL` illumination record is rejected when a caller attempts to admit it as scientific-scene evidence. CICM and Room Capsule outputs remain `COUNTERFACTUAL` even when their input/reference illumination was physically measured.

The calibrated CICM forward path additionally requires an `APPLICABLE` calibration decision. `OUT_OF_DOMAIN` or `UNCERTIFIED` calibration fails closed.

This is an authority bridge, not yet a replacement for the existing CICM/Room Capsule C++ APIs. The next native integration should transport the same authority fields through their runtime records and provenance.

## 2. Calibration Registry resolver

v0.2 turns the v0.1 calibration record contract into a deterministic resolver.

`CalibrationRegistry.resolve()`:

- filters by calibration kind;
- validates capture-domain applicability;
- requires hold-out certification when requested;
- returns `OUT_OF_DOMAIN` rather than extrapolating;
- returns `UNCERTIFIED` when a matching domain lacks a passed hold-out;
- returns `AMBIGUOUS` when two certified records overlap instead of silently choosing newest/first;
- supports an explicit preferred calibration ID when the caller has a documented reason.

The registry also produces a deterministic SHA-256 fingerprint over calibration identity, domain, evidence and hold-out state.

This is the foundation for binding tele-camera black/linearity/noise/PRNU/shading/optics/colour/illuminant/spectral calibration to the exact Camera-5 capture domain.

## 3. Structure Evidence -> v4.7j / v4.7k execution gate

`gate_detail_and_acutance()` maps the Structure Evidence support class to a concrete canonical appearance route while preserving authority.

- `MEASURED_SUPPORTED` -> `AdaptiveDetailedCrisp` + `AdaptiveDetail`
- `RECONSTRUCTED_SUPPORTED` -> the same appearance tools may run, but provenance remains reconstructed
- `CENSORED_OR_WEAK` / `UNKNOWN` -> fail closed to `NeutralReference` + `Neutral`

No detail/acutance output is allowed to write the scientific master or become a newly measured-detail claim. Measured support describes the input evidence; sharpening/acutance remains appearance.

v0.2 deliberately does not inject arbitrary numeric strength multipliers into canonical v4.7j/v4.7k. It first establishes the authority gate. A later validated policy may add evidence-conditioned gain only with explicit tests and provenance.

## 4. Conservation / restoration artefact stack

`RestorationStack` is now a hash-chained reversible layer stack.

Each layer binds:

- immutable root-source SHA-256;
- parent layer SHA-256;
- restoration class;
- mask SHA-256;
- output/payload SHA-256;
- source-evidence hashes;
- method;
- confidence or hypothesis where applicable.

The stack never overwrites the source.

v0.2 additionally prevents a linear stack from re-entering evidence-supported authority after a `HYPOTHETICAL_VISUAL_RESTORATION` or `APPEARANCE_ONLY` layer. A future restoration DAG may branch from an earlier clean parent; silent authority recovery through a visually modified parent is forbidden.

## 5. Native Camera-5 200MP complete proof chain

`tools/camera5_200mp_evidence_bundle_v02.py` composes the existing independent gates into one end-to-end chain:

`runtime manifest -> runtime 200MP gate -> canonical RAW normalization -> RAW/DNG CFA identity -> evidence bundle`

A bundle PASS requires:

- one `DEVICE_RUNTIME_CAPTURE`;
- physical camera `5`;
- `RAW_SENSOR`;
- exactly `16320 x 12288`;
- matching source identity hashes;
- runtime gate PASS;
- canonical CFA mirror binding;
- DNG file hash binding;
- exact canonical RAW <-> DNG CFA sample identity;
- exactly `16320 * 12288` compared samples;
- no first mismatch.

The target classification is:

`CAMERA5_200MP_CAPTURE_CHAIN_PROVEN_APP_VISIBLE_CFA`

The CLI `prove_from_files()` can run the full chain on a real capture pair while keeping the large RAW processing streaming/row-based where the underlying tools support it.

A PASS remains strictly bounded: it proves one internally consistent app-visible Camera2 maximum-resolution CFA observation. It does not prove untouched photodiode/ADC output, one ADC sample per physical photodiode, electron calibration, optical truth, spectral truth, colour truth, or a second independent exposure.

## Tests

`tests/test_open_world_integration_v02.py` verifies:

- counterfactual illumination cannot enter the scientific scene;
- the same open-world virtual light can enter CICM while staying counterfactual;
- calibrated CICM forward prediction requires applicable calibration;
- overlapping calibrations fail as ambiguous unless explicitly selected;
- uncertified calibrations fail closed;
- supported structure maps to adaptive appearance without scientific authority upgrade;
- weak/censored structure maps to neutral appearance;
- restoration layers are hash chained;
- hypothetical restoration contaminates a linear visual branch and blocks later evidence-authority upgrade;
- the 200MP bundle passes only when all hashes, dimensions, sample counts and lower gates agree.

## Promotion boundary

Do not promote this phase to `main` until:

1. CI for the v0.2 contracts passes;
2. one real HONOR BKQ-N49 Camera-5 `16320 x 12288` RAW+DNG pair passes the full evidence-bundle CLI;
3. native CICM/Room Capsule authority fields are integrated without altering scientific-master authority;
4. the Calibration Registry is populated only from measured campaigns with real hold-outs;
5. detail/acutance integration is validated against real structure/MTF evidence;
6. restoration artefacts have a persistent storage/export format with reversible provenance.
