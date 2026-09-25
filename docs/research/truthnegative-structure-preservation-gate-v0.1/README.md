# TruthNegative Structure Preservation Gate v0.1

Status: EXECUTABLE N2 ADMISSION GATE — DOES NOT DENOISE.

The gate asks whether a local residual is even eligible for later noise suppression. It uses the admitted sigma plus first-derivative and Laplacian structure normalized by sigma.

Fail-conservative conditions always PRESERVE: unknown sigma, CENSORED sample or censor boundary, non-measured support, weak registration/visibility, or statistically strong local structure.

Even an eligible weak-structure sample is capped at 75% future residual suppression. This module changes no Scientific Master or TruthNegative value and creates no evidence.

Thresholds are deliberately research defaults, not sensor calibration. Promotion requires real RAW validation using flat fields, natural texture, edges/MTF and low-light scenes.
