# TruthRaw Bound Uncertainty Admission v0.79

Status: **integration candidate — admission gate only, no reconstruction change**

v0.79 answers one question:

> May a specific uncertainty model be used to upgrade missing-channel authority for this exact source/sample-domain/reconstruction quantity?

The answer is fail-closed.

## Exact historical scope

The only registered candidate model is historical v5.0g-p1:

- HONOR
- BKQ-N49
- HONOR vendor DNG
- 4080x3072
- BGGR
- WhiteLevel 1023
- focal length 22.48 mm
- valid NoiseProfile
- exact v4.7i production backend hashes
- exact frozen v5.0g extractor/schema/model/binding hashes
- exact prospective holdout result
- exact PTC uncertainty bridge

Even when all of those match, current v0.79 returns:

`ELIGIBLE_TRACE_GATE_OPEN`

because the project still lacks an accepted certificate binding historical v5.0g uncertainty coordinates to the exact F64 Scientific-Master reconstructed quantity.

The registry intentionally contains an all-zero expected F64 trace certificate, making `ADMITTED` unreachable.

## Current Camera-5 derived DNG

The current product route:

`Camera2 RAW_SENSOR envelope -> exact admitted prefix -> derived processing DNG -> Main House`

is a different source domain.

It returns:

`BLOCKED_SOURCE_DOMAIN_MISMATCH`

even when dimensions, CFA or WhiteLevel happen to resemble the old vendor-DNG tele class.

This prevents calibration/model transfer by superficial metadata similarity.

## Consequence for v0.78

Until v0.79 returns an actually admitted decision:
- v0.78 `RECONSTRUCTED` count remains 0;
- direct uncensored CFA stays `CALIBRATED_ESTIMATE`;
- clipped direct CFA stays `CENSORED`;
- missing channels stay `UNKNOWN`.

v0.79 changes no pixels, no Scientific Master, no Zero-Line, no Backplane and no existing uncertainty model.

## Next scientific work

Two independent ways forward exist:

1. Historical v5.0g: produce and validate the missing exact F64 trace certificate.
2. Current Camera-5 derived DNG: calibrate and prospectively validate its own source-domain PTC/noise/uncertainty model.

The second path must not reuse v5.0g merely because the raster is 4080x3072.

## Machine-readable current admission invariant

`reconstructedAuthorityAllowed=false`

This remains mandatory until an exact accepted F64 trace certificate and runtime p95 field path are both present.
