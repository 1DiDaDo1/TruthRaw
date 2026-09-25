# TruthNegative N2 Spatial Sidecar v0.1

Status: EXECUTABLE AUDIT SIDECAR — NO SCIENTIFIC OR APPEARANCE WRITEBACK.

This module serializes the N2 measured-CFA audit as a deterministic, spatially resolved JSON sidecar. It never stores candidate values inside the TruthNegative native container and never feeds them into Scientific Master, TruthNegative, Deep Scene, Appearance or display.

The sidecar binds the sealed source SHA-256, Scientific Master SHA-256, Open Scene authority-field SHA-256, TruthNegative state SHA-256, N2 candidate SHA-256, N2 global audit SHA-256 and N2 spatial SHA-256.

Each source-domain audit tile records its rectangle, sampled/eligible/candidate/preserved counts, protection reasons, total and removed residual energy, maximum proposed Stage-2 correction, border protection and CFA-phase sample counts.

The Android default remains a balanced 1/16 measured-CFA sample with samplingPeriod=8 and 64x64 source tiles. Candidate corrections remain candidate_applied=false.

The spatial sidecar is an observation/audit artifact. It creates no evidence and cannot promote a denoise candidate to measured data.
