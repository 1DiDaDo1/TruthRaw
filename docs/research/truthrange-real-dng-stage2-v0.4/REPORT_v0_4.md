# TruthRaw v0.4 Real-DNG Stage-2 Validation Report

## Decision

**`REAL_DNG_STAGE2_BRIDGE_BNCAM_DOMAIN_PASS`**

## What was closed

The project can now deterministically reconstruct the exact Stage-2 input domain from the eight real HONOR BKQ-N49 BnCam tele DNGs without treating DNG export or display rendering as the scientific master.

## Hard gates

1. Classic-TIFF parser and DNG tag contract: PASS.
2. Uncompressed 16-bit CFA strip decode: PASS.
3. Four phase BlackLevel values: PASS.
4. WhiteLevel=1023: PASS.
5. BGGR CFA identity: PASS.
6. OpcodeList2 complete-byte parse: PASS.
7. Four GainMap opcodes, no unsupported opcode silently ignored: PASS.
8. GainMap phase coverage exactly one map per CFA site: PASS.
9. GainMap applied exactly once: PASS.
10. Native C++ synthetic malformed-input rejection: PASS.
11. Stdlib Python synthetic TIFF/DNG reference test: PASS.
12. Full decoded-RAW SHA parity vs independent TIFF decode for 8/8 sources: PASS.
13. Full Stage-2 float-bit sum parity native vs independent NumPy oracle: PASS 8/8.
14. Full Stage-2 float-bit XOR parity native vs independent NumPy oracle: PASS 8/8.

## Real Stage-2 maxima by ISO

- ISO 100: `0.4262324869632721`
- ISO 200: `0.7648361325263977`
- ISO 400: `1.449364185333252`
- ISO 800: `1.4858797788619995`
- ISO 1600: `1.6815977096557617`
- ISO 3200: `1.7266061305999756`
- ISO 6400: `1.7218064069747925`
- ISO 12800: `1.7202461957931519`

Source clipping counts over the same sequence are:

`0, 0, 0, 0, 0, 18, 15, 50`.

This proves operationally that Stage-2 values above 1.0 and source clipping are different concepts.

## GainMap result

All captures use four `13×17×1` GainMaps selected by 2×2 CFA phase. Stored entries across the complete series span:

`1.0009765625 .. 2.439453125`.

Every capture has a distinct complete OpcodeList2 SHA-256, so the map is source-bound.

## Parity precision

- native vs NumPy aggregate max absolute error: `2.636779683484747e-16`;
- native vs stdlib sampled GainMap max absolute error: `1.1687185264008804e-07`;
- native vs stdlib sampled Stage-2 max absolute error: `1.9691490826811986e-08`;
- full-field bit fingerprints: exact for 8/8.

## Claim boundary

The bridge is domain-specific and fail-closed. It does not convert its bounded source evidence into infinite measured dynamic range. It only places that evidence correctly into the unbounded TruthRaw scene representation, preserving clipping as censoring and >1 values as valid scene-linear coordinates.

## Next

**v0.5: real Stage-2 -> Latent Camera Scene -> dense measured/reconstructed uncertainty.**

The first task is to run these exact source-bound Stage-2 fields through the v0.2 measured-preserving camera-RGB adapter and attach the v0.3 uncertainty classes without weakening the existing `topologyCertified=false` boundary for missing RGB channels.
