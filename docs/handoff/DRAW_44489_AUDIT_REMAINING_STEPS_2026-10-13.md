# D.RAW / TruthRaw — 44489 — SUPERSEDED audit-note — 2026-10-13

Continuation code: **44489**

> **SUPERSEDED AS CURRENT ROUTING. DO NOT USE THIS FILE AS THE ACTIVE NEXT STEP.**

This file was added during a later chat after the previous-chat endpoint had been reconstructed incorrectly. It treated the read-only sample-domain audit as if it still had to be completed *before* real-device APK validation.

That ordering is stale.

The actual previous-chat endpoint was later restored and independently checked against repository history:

- content/scientific candidate checkpoint: `f7a25cf1c8b9c98c4d7b02654868cf11af4f5ade`;
- on that exact content checkpoint the Source Capability Raw-to-Linear Contract v0.1 Integrity workflow was successful, together with the relevant Runtime Evidence Envelope / LinearizationTable / Universal Intake / DngCreator Compatibility / Canonical checks;
- the candidate was intentionally still read-only / shadow diagnostic;
- the next authorized step was to build and run a stable-signed real-device test APK;
- only *after* that device validation was the sample-feeding phase to be considered.

Temporary APK-CI trigger commit:

`86519dee1c0f97398870d67ff4414f9f096b4f1a`

Restored trigger configuration commit:

`30d68684df0e347a8ae2bf52bf50650da8bfd25c`

The temporary trigger came and went without a persistent content-tree change relative to `f7a25cf1...`.

## Active interpretation

The test APK is for the real-device path:

`RAW/DNG -> existing parser/evidence -> Effective Black/Normalization + LinearizationTable -> Raw-to-Linear compositor -> Runtime Evidence Envelope`

The following remain intentionally false in this phase:

- `linearization_applied_to_image_samples=false`
- `black_subtraction_applied_to_image_samples=false`
- `normalization_applied_to_image_samples=false`
- `clipping_applied_to_image_samples=false`
- `full_raw_to_linear_mapping_validated=false`
- no Scientific Master pixel effect
- no Appearance pixel effect
- no scientific writeback
- no new MEASURED evidence

A visual change in PURE / ADVANCED / PRO caused by this read-only cable would therefore be suspicious, not a success criterion.

## Correct next phase ordering

1. real-device validation of the new read-only/shadow cable on real RAW/DNG input;
2. inspect resulting runtime records / diagnostics for correct single-cable behavior and fail-closed authority;
3. only after that validation, decide whether to implement/bind the phase where Linearization -> effective-black subtraction -> normalization -> clipping actually feeds sample values;
4. any such later sample-domain work must still preserve sealed evidence, provenance, authority separation, Scientific Master firewall and target-independent regressions.

This supersession is documentation/governance only and grants no new scientific authority.
