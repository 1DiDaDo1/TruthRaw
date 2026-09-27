# D.RAW Calibration Observation Binding v0.1

Status: **MAIN-INTEGRATED RESEARCH FOUNDATION — HOST VALIDATED; NO REAL CAMERA CALIBRATION PROMOTED**

This module gives the existing Open-World Calibration Registry deterministic
identities and an explicit admission binding to one D.RAW Observation.

It creates no calibration values and promotes no current Camera-5 calibration.

## Why

The existing `CalibrationDomain`, `CalibrationRecord` and
`CaptureConditions` already fail closed on device/camera/mode/raster/CFA and
numeric validity ranges. D.RAW additionally needs source-domain identity so
two superficially similar 4080×3072 records cannot silently share authority.

The binding domain therefore adds:

- `source_route_id`;
- `sample_domain_id`;
- `readout_domain_id`.

This directly preserves the earlier finding that a historical HONOR vendor DNG
and the current Camera2 RAW_SENSOR-derived DNG are different source domains
even when width, height, CFA or WhiteLevel look similar.

## Deterministic identities

v0.1 produces SHA-256 identities for:

1. the complete validity/binding domain;
2. the calibration record plus calibration-payload hash;
3. the target Observation capture context;
4. the final admission decision.

Optional floating-point capture/domain quantities are hashed using Python's
exact hexadecimal float representation, not locale-formatted decimals.

Calibration source-evidence hashes are treated as an unordered unique set and
are sorted before record hashing.

## Admission

A binding is admitted only when:

- the existing CalibrationRecord validates;
- hold-out validation is PASS with a hashed report;
- device and physical camera match;
- sensor pixel mode, raster and CFA match;
- source route, sample domain and readout domain match;
- every bounded capture condition is present and within range.

Otherwise the result remains `OUT_OF_DOMAIN` or `UNCERTIFIED`.

## Authority boundary

An admitted calibration binding:

- does not mutate Source Evidence;
- adds no physical frame to the target observation;
- adds no independent target-observation evidence count;
- does not grant a cross-observation relation;
- does not imply calibration transfer to another lens/readout domain;
- does not itself perform cross-observation fusion.

It only proves that one identified calibration record is applicable to one
identified capture context.

## Observation Record v0.3

The hashes produced here are the intended identities for the v0.3 fields:

- `calibration_record_sha256`;
- `validity_domain_sha256`;
- `holdout_report_sha256`.

No real Camera-5 calibration binding is added by this module. The tests use
synthetic fixtures only.
