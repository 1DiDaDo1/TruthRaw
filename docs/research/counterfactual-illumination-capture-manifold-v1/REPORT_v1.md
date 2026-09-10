# TruthRaw CICM v1 — implementation report
**Date:** 2026-09-10
**Status:** `RESEARCH_PASS_FRAMEWORK_PHYSICAL_HONOR_CALIBRATION_BLOCKED`

## What changed
A new counterfactual layer was implemented between the admitted scene representation and hypothetical virtual capture. It is explicitly separate from Virtual Observation Manifold (same-evidence reparameterization) and from appearance/S-curve processing.

## Key distinction
- VOM EV/gain: same evidence, signal and uncertainty transform together, no SNR gain.
- CICM relative world: hypothetical radiance/exposure scale, still no physical SNR claim.
- CICM calibrated capture: a different hypothetical acquisition. Illumination/shutter can change expected collected electrons and temporal SNR, but only under exact-bound calibration.

## Temporal sensor model in v1
For admitted nonnegative scene signal `L`, neutral relative illumination `I`, bound sensitivity `C [e-/s/scene-unit]`, and shutter `t`:

`signal_e = L * I * C * t`

`dark_e = dark_current_e_per_s * t`

`variance_e = signal_e + dark_e + read_noise_e_rms^2`

`SNR_e = signal_e / sqrt(variance_e)`

Pre-clip DN uses the independently supplied sensor-mode system gain. Effective saturation charge is the lower of physical full well and ADC headroom mapped back to electrons. Saturated predictions are censored; no exact above-saturation value is claimed.

This is intentionally a temporal expected-value model only. PRNU/DSNU, row/column structure, hot pixels, optics, spectral quantum efficiency and spatial color shading are outside v1.

## Best Simulated Capture
The implemented selector maximizes calibrated electron-domain SNR among caller-supplied shutter × independently calibrated sensor-mode candidates, rejecting censored candidates and enforcing a configurable minimum headroom. Research-fixture calibration cannot authorize this physical selection.

Without motion/subject constraints, the selector naturally prefers longer shutter until headroom/censoring limits it. This is expected physics, not a universal photography recommendation.

## Sun/night interpretation
No physical SUN/NIGHT preset exists yet. `illuminationScale` is a neutral relative hypothesis. A full relight capable of new shadows, specular behavior, material-dependent color and daylight/moonlight spectral differences is reserved and fail-closed.

## Evidence / zero-line
Every output retains one physical frame and one independent evidence root. Counterfactual observations never become source evidence. The original scientific master and TruthRange `L0` are unchanged.

## Native falsification results

After strengthening exact source/Scene-Master/zero-line and calibration-evidence bindings:

- GCC C++20 Release with `-Wall -Wextra -Werror`: PASS
- Clang C++20 Release with `-Wall -Wextra -Werror`: PASS
- Clang ASan/UBSan: PASS
- relative world fixture: `+1 EV` exactly for illumination ×4 and relative shutter ×0.5
- synthetic calibrated fixture SNR: `2.66312`; doubling neutral illumination raises it to `4.074`
- equal electron-noise modes with downstream gain change preserve electron-domain SNR while DN scale changes
- 1000-point illumination sweep: `741` uncensored, `259` censored; SNR monotonic over every uncensored point
- invalid negative light, invalid master hash, missing calibration, missing independent-evidence hash, optical binding mismatch and scene-scale binding mismatch all fail closed
- research-fixture calibration may calculate numeric predictions but cannot authorize physical Best Simulated Capture
- full geometry/BRDF/spectral relight remains explicitly `FULL_RELIGHT_NOT_IMPLEMENTED` even if a caller marks all required inputs present; v1 refuses to fake a renderer

## Promotion boundary

This module can be promoted only as a **research framework contract**. Its synthetic independently-measured fixture validates code paths, not the HONOR BKQ-N49 tele sensor. Real Honor physical-forward outputs remain blocked until a real calibration-evidence package is admitted and hash-bound.
