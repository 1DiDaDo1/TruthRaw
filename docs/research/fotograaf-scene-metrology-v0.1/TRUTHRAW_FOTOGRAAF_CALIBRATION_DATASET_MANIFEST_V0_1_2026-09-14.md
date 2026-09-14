# TruthRaw FotoGraaf Calibration Dataset Manifest v0.1 — 2026-09-14

**Status: RESEARCH EVIDENCE-BINDING PROTOCOL / FAIL-CLOSED**

`FotoGraafCalibrationPack` can only be trusted if the captures used to fit and validate it are themselves sealed, scope-bound and reproducible. This document defines that evidence-binding layer.

A dataset manifest does **not** make a calibration physically correct. It proves which exact bytes, scope identity and fit/validation role a calibration result claims to depend on.

## Canonical chain

`calibration capture bytes`
→ per-file SHA-256
→ `CalibrationDatasetManifest`
→ canonical manifest SHA-256
→ `FotoGraafCalibrationPack.datasetManifestSha256`
→ claim-specific promotion gate

The later photographed scene still remains one physical frame / one independent scene-evidence root.

## Per-capture binding

Every calibration capture must record:

- unique `captureId`;
- calibration module C1–C7;
- protocol role such as `DARK`, `FLAT`, `UNIFORM_SIGNAL`, `COLOR_TARGET`, `RELATIVE_RADIANCE`, `ABSOLUTE_RADIANCE` or `LIGHT_RIG`;
- `FIT` or `VALIDATION` split;
- relative file name;
- exact SHA-256 and byte length;
- exact `scopeKeySha256`;
- metadata-snapshot SHA-256;
- exposure time and ISO metadata;
- BlackLevel/WhiteLevel identity;
- GainMap/opcode identity;
- focus state;
- stabilization state;
- temperature observation.

A repeated reference to the same file SHA-256 is not an independent repeat and fails closed.

## Fit versus validation

Every capture belongs to exactly one split in v0.1:

- `FIT`
- `VALIDATION`

A single `captureId` cannot appear in both. The pack validator separately requires the module-level fit and validation dataset identities to remain disjoint.

## Temperature record

Temperature is either:

`MEASURED`
- sensor/probe identity;
- value in °C;
- optional uncertainty;

or:

`UNAVAILABLE`
- explicit reason.

`UNAVAILABLE` may still be structurally recorded, but it cannot support a temperature-calibrated pack.

## External references

SPD measurements, spectral target data, radiance references, irradiance references, geometry references and material/BRDF references are also exact files with SHA-256 and byte length.

When `--root` is supplied, the manifest verifier re-reads every referenced capture/reference file and fails on a byte-length or SHA mismatch.

## Canonical dataset digest

The canonical manifest digest is:

`SHA256(UTF-8 canonical JSON)`

where canonical JSON uses:

- sorted keys;
- separators `(',', ':')`;
- UTF-8;
- `ensure_ascii=false`;
- top-level `manifestSha256` omitted from the digest input.

That digest becomes the future `FotoGraafCalibrationPack.datasetManifestSha256`.

## Authority boundary

A passing dataset-manifest verifier means:

**PASS — exact calibration evidence inventory and scope binding.**

It does **not** mean:

- physical calibration is accurate;
- a module has met its capture-count minimum;
- an external instrument is traceable;
- `CALIBRATED_PHYSICAL` is authorized.

Those stronger decisions remain in the Calibration Pack admission gate.

The permanent rule is:

**Calibration evidence may constrain the measurement operator. It never becomes extra evidence for a later photographed scene.**
