# Main camera — Orientation Quarantine + Host Scientific Route

Date: 2026-09-27

Status: **PROVEN DERIVED-INGRESS EXECUTION; FINAL OBSERVATION ADMISSION NOT YET GRANTED**

## Original-source run

The current host wrapper invokes the existing common D.RAW pipeline directly.

Against the immutable original main DNG it failed closed before Scientific
Master construction:

- source SHA-256:
  `a85cac58601d6cc8138bf8f9372e5161b85ab1e89afa70372f97c46461dcea79`
- result: `PIPELINE_REJECTED`
- code: `7006`
- reason: `TileNativeDngSource rejected DNG: unsupported Orientation`

Byte inspection found Orientation tag 274, SHORT count 1, value 9, at value
field offset 138.

## Quarantine derivation

A local derivative was created without changing the original source.

Only byte offset 138 changed:

`09 -> 01`

Derived ingress SHA-256:

`7c8eb85c568f6bc0ec3ae007de1658ec86d38b1144059d0fe0bc294a3e17bf08`

The serialized CFA payload stayed byte-identical:

`4818fd406cc528984ff57e036db49ac394ce0bb6b6d0830e2f9f1ad95ecc8c3c`

CFA payload bytes: `25,165,824`.

Presentation/world orientation authority remains UNKNOWN.

## Unchanged common-pipeline result

The CI-built Host Scientific Route executable was run twice against the
derived ingress container. Both complete JSON outputs were byte-identical.

Result JSON SHA-256:

`a24df150c8a2db4b4a124c896bcd5044dc3673e45de9f512842650166383c018`

Key resulting identities:

- Scientific Master:
  `c26939efe5e58a0d32846e905d156789e35b674bb2d1c03288bbabc53e243202`
- authority field:
  `e18f0038b36ccca8502e5ec29108145408223084088ab883176d8390838d6b2a`
- TruthNegative Continuous v0.5:
  `cd3acbd90ce47efc2015f9676e7ef0ed25a16248acdb26bdb278ed1ca332a60c`
- D.RAWnegative v0.1:
  `87955cae86a3c9318b24990018208a4982379366d4bb4ae78e830c8d1cf0ccf7`

The pipeline remained source-local:

- common gauge admitted = false;
- cross-observation radiometric equality = false;
- cross-observation radiometric fusion = false;
- Float64 branch-sensitive compute = true;
- counts = 1 physical frame / 1 independent evidence item;
- creates new evidence = false;
- scientific writeback = false.

Colour authority is `SOURCE_METADATA_BOUND`, not independent calibration.

The adapter continues to report:

- exact CFA samples available = true;
- scientific admission ready = true;
- stored-sample sensel semantics certified = false;
- direct sensor ADC claim allowed = false.

Therefore this result does not upgrade capture/readout/sensel provenance.

## Current admission boundary

These scientific identities belong to the **derived ingress bytes**. The
original physical-source SHA and the derived ingress SHA must remain connected
as one physical observation before a final D.RAW Observation Record is
promoted.

The next required step is an explicit Scientific Ingress Lineage Binding that
preserves:

`original physical source -> derived quarantine ingress -> Scientific Master`

without creating a second frame or evidence item.
