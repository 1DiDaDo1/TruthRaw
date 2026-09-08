# TruthRaw Zero-Line v0.1 — first implementation report

## Decision

**ARCHITECTURE_VALIDATION_PASS**

The first executable TruthRange layer is working.

### Tested source series

HONOR BKQ-N49 BnCam tele, ISO:

`100, 200, 400, 800, 1600, 3200, 6400, 12800`

### Fixed zero-line gauge

`L0 = 2^-10 = 0.0009765625` arbitrary relative-light units.

`T = log2(L_proxy/L0)`

with:

`L_proxy = normalized RAW signal / (ISO × exposure_seconds)`

The gauge is intentionally relative and not claimed as watts, lux, photons or electrons.

## ISO-neutrality result

Across the eight real DNGs:

- p50 TruthRange spread = **0.093109 EV**
- p90 spread = **0.132132 EV**
- p99 spread = **0.133236 EV**

This is a strong first practical validation of the project statement:

**ISO belongs to capture provenance; it does not have to remain the numerical identity of the reconstructed scene.**

## High-side result

For ISO 400 onward, the source-clipping lower bound sits around +4.6 EV in this provisional gauge.

When the source reaches WhiteLevel, the new representation does not stop there:

`T >= T_clip_lower`

and `upper = +infinity`.

That is the first operational implementation of the user's "house can continue upward forever" idea.

## Low-side result

NoiseProfile offset terms define a finite evidence-dependent dark upper bound.

Below that point, TruthRange can represent:

`lower = -infinity`

with a finite evidence-dependent upper bound.

That is the operational implementation of the "house can continue downward forever" side.

## What has *not* been claimed

- The camera captured infinite dynamic range: **false**.
- The 0-line makes black/noise/gain/exposure interpretation unnecessary: **false**.
- ISO is a physical conversion-gain calibration: **not claimed**.
- The current relative gauge is absolute radiometry: **not claimed**.
- Clipped pixels have known exact values above clipping: **false**.

## Architectural conclusion

TruthRaw should distinguish permanently:

1. **TruthRange address space** — unbounded by construction;
2. **evidence window** — finite and capture-dependent;
3. **reconstructed support range** — may exceed the evidence window with uncertainty/bounds;
4. **presentation range** — finite export/display projection.

The next step is to attach this coordinate to the exact Latent Scene Master and v5.0g uncertainty rather than directly to pre-GainMap CFA evidence.
