# TruthNegative N2 Spatial Sidecar v0.1

Status: EXECUTABLE AUDIT SIDECAR — NO SCIENTIFIC OR APPEARANCE WRITEBACK.

This module serializes the N2 measured-CFA audit as a deterministic, spatially resolved JSON sidecar. It never stores candidate values inside the TruthNegative native container and never feeds them into Scientific Master, TruthNegative, Deep Scene, Appearance or display.

The sidecar binds the sealed source SHA-256, Scientific Master SHA-256, Open Scene authority-field SHA-256, TruthNegative state SHA-256, N2 candidate SHA-256, N2 global audit SHA-256 and N2 spatial SHA-256.

Each source-domain audit tile records its rectangle, sampled/eligible/candidate/preserved counts, protection reasons, total and removed residual energy, maximum proposed Stage-2 correction, border protection and CFA-phase sample counts.

The Android default remains a balanced 1/16 measured-CFA sample with samplingPeriod=8 and 64x64 source tiles. Candidate corrections remain candidate_applied=false.

The spatial sidecar is an observation/audit artifact. It creates no evidence and cannot promote a denoise candidate to measured data.


## Android integration validation

The spatial sidecar is wired into the D.RAW PRO Android route as an explicit user export:

`Export N2 Spatial Audit · JSON`

The export replays the admitted DNG through the same sealed source / Scientific Master / authority / TruthNegative preparation path, runs the audit-only N2 CFA sampler, serializes the per-tile report, writes it through the Android document destination, rereads the written bytes, verifies the JSON SHA-256 and reverifies the source seal.

Combined Android validation:
- branch: `integration/pro-truthnegative-continuous-primary-route-v075`
- head: `5d125988df9de1fd891300150b977a6c2196486b`
- GitHub Actions run: `36181558037` — SUCCESS
- signed ARM64 APK bytes: `6567631`
- APK SHA-256: `b71d11edfa1b81c5885abab856c6a248110d7e8c5fd3505149b60c608ee15ffb`
- signing certificate SHA-256: `a6288a4b7e9d18e908eeba37540cd962b687f2290f7e45d7c7ad3390ad61fd44`

This combined build also contains the Activity rotation-lifecycle fix and the dark-text UI changes. These UI/lifecycle changes do not alter the scientific pipeline.

The N2 spatial export remains audit-only:
- `candidateApplied=false`
- `createsNewEvidence=false`
- `scientificWritebackAllowed=false`
- source and TruthNegative identities are independently bound and checked.
