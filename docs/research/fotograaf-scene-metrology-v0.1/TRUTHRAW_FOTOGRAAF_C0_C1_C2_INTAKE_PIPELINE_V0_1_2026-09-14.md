# TruthRaw FotoGraaf C0 -> C1/C2 calibration intake pipeline v0.1 — 2026-09-14

**Status: SOFTWARE INTAKE PIPELINE IMPLEMENTED / REAL HONOR CALIBRATION BYTES STILL REQUIRED**

This step turns the already-defined FotoGraaf acquisition protocol into an operational intake path for the first real Honor tele calibration captures.

It does not fit a model, does not alter reconstruction, and does not grant `CALIBRATED_PHYSICAL` authority.

Permanent laws remain:

**Measured where measured. Reconstructed where necessary. Never invented.**

**Representation can exceed the source. Knowledge claims cannot exceed the evidence.**

## 1. Why an intake layer is necessary

The acquisition protocol defines what must be captured. C0 seals one exact camera/lens/mode/sample-domain identity. The dataset-manifest contract binds calibration evidence bytes. Before v0.1 there was still a practical gap: a large C1/C2 acquisition could be copied into a folder, but no single fail-closed step revalidated every C0 seal, rehashed every source, checked every metadata sidecar, split sample domains correctly, and produced manifest candidates.

This intake layer closes that gap.

The path is now:

`controlled RAW/DNG + capture sidecar + C0 record`

`-> re-run exact C0 seal for every capture`

`-> parse C1/C2 acquisition metadata`

`-> partition by exact C0 scopeKeySha256`

`-> generate one dataset-manifest candidate per exact scope`

`-> compute Phase-A C1/C2 completeness`

`-> later CalibrationPack fitting/validation`

No step in this path changes a later photographed scene's `physicalFrameCount=1` or `independentEvidenceCount=1`.

## 2. Important improvement: sample-domain partitioning is automatic

The exact-ISO8192 research demonstrated that numerically adjacent ISO values can belong to different serialized noise/sample domains. Therefore a large calibration capture session must **not** be flattened into one dataset merely because the device/lens and nominal ISO ladder look continuous.

The intake builder groups captures by exact C0 `scopeKeySha256`.

Because `captureSampleDomainId` is a C0 scope field, a special exact-ISO8192-associated domain and the ordinary domain necessarily become different manifest candidates if C0 classifies them differently.

The same rule also separates changes in physical camera ID, capture API domain, firmware, focus-state class, stabilization state, dimensions, CFA topology or sample representation.

**ISO magnitude never merges those scopes.**

## 3. Input trio for every calibration capture

Every C1/C2 capture admitted by intake has three physical records:

1. exact RAW/DNG source bytes;
2. exact capture metadata snapshot/sidecar bytes;
3. exact C0 identity record that binds those two files.

The intake tool re-runs `verify_fotograaf_c0_capture_identity_v0_1.py` logic for every item. A stale source, edited sidecar, stale hash, changed focus state, changed firmware identity, unresolved camera identity, processed-RGB input, merged multi-frame input, or other C0 mismatch fails before the capture can enter a calibration manifest.

The metadata snapshot must additionally bind:

- exact `sourceEvidenceSha256`;
- exact `scopeKeySha256`;
- exposure time;
- ISO metadata;
- independently classified `gainReadoutStateId`;
- BlackLevel identity;
- WhiteLevel identity;
- GainMap/opcode identity;
- focus state;
- stabilization state;
- temperature observation.

For C2 it additionally records:

- `controlledSignalLevelId`;
- `sourceStabilityReferenceId`;
- whether the level is part of the saturation bracket.

## 4. C1 completeness logic

The official Phase-A plan remains the authority for acquisition volume.

Across the intake session, C1 requires at least seven acquisition anchors. These are **capture-planning anchors**, not an assumption that there are exactly seven physical gain/readout domains.

For every acquisition anchor:

- at least four distinct exposure times;
- at least sixteen independent dark frames per anchor/exposure cell.

Every physical capture must remain byte-unique. Reusing the same source bytes as if they were an independent repeat fails closed.

Dark input still means physically photon-blocked camera input. A black scene photograph does not become a C1 dark merely by assigning the `DARK` role.

## 5. C2 completeness logic

Across the intake session, C2 requires at least seven acquisition anchors.

For every anchor:

- at least twelve controlled signal-level IDs;
- at least eight independent captures per signal level;
- at least four held-out validation levels;
- at least three signal levels marked as saturation-bracket levels;
- a source-stability reference identity on every C2 metadata snapshot.

The FIT/VALIDATION distinction is carried into the dataset manifest. The same physical source file cannot be reused across roles or splits as an independent observation.

## 6. Candidate manifests may exist before the acquisition is complete

Long calibration sessions need checkpoints. Therefore v0.1 permits an incomplete acquisition to produce **candidate** manifests and an intake bundle with status:

`CANDIDATE_INCOMPLETE`

This is intentionally not authority.

When the official C1/C2 requirements are met, the bundle status becomes:

`PHASE_A_C1_C2_COMPLETE_CANDIDATE`

Even that state still does not grant calibrated physical authority. It only means the capture-count/partition/evidence-binding prerequisites for the C1/C2 acquisition candidate are complete.

`--require-complete` converts incompleteness into a fail-closed command result for automation that is only allowed to proceed after the acquisition matrix is complete.

## 7. Files

Contract:

`docs/research/fotograaf-scene-metrology-v0.1/TRUTHRAW_FOTOGRAAF_CALIBRATION_INTAKE_CONTRACT_V0_1.json`

Builder/verifier:

`tools/build_fotograaf_calibration_intake_v0_1.py`

Tests:

`tests/test_build_fotograaf_calibration_intake_v0_1.py`

CI:

`.github/workflows/fotograaf-calibration-intake-v0-1.yml`

Session template:

`docs/research/fotograaf-scene-metrology-v0.1/TRUTHRAW_HONOR_TELE_C0_C1_C2_INTAKE_SESSION_TEMPLATE_V0_1.json`

## 8. Intended physical use

After the first controlled Honor tele capture run, place the exact files under one acquisition root and populate the session JSON. Then run:

```text
python3 tools/build_fotograaf_calibration_intake_v0_1.py \
  --intake-contract docs/research/fotograaf-scene-metrology-v0.1/TRUTHRAW_FOTOGRAAF_CALIBRATION_INTAKE_CONTRACT_V0_1.json \
  --c0-contract docs/research/fotograaf-scene-metrology-v0.1/TRUTHRAW_FOTOGRAAF_C0_CAPTURE_IDENTITY_CONTRACT_V0_1.json \
  --manifest-contract docs/research/fotograaf-scene-metrology-v0.1/TRUTHRAW_FOTOGRAAF_CALIBRATION_DATASET_MANIFEST_CONTRACT_V0_1.json \
  --plan docs/research/fotograaf-scene-metrology-v0.1/TRUTHRAW_HONOR_TELE_CALIBRATION_ACQUISITION_PLAN_V0_1.json \
  --session <session.json> \
  --root <acquisition-root> \
  --output-dir <intake-output>
```

When the full C1/C2 matrix is believed complete, add `--require-complete`.

The output contains:

- one dataset manifest per exact C0 scope;
- one intake bundle binding all generated candidates;
- exact source/metadata/C0 hashes;
- acquisition anchor and gain/readout identifiers;
- a completeness report.

## 9. Current physical status

**C0 software gate: IMPLEMENTED.**

**C0 physical Honor tele identity seal: OPEN until a controlled capture-sidecar run exists.**

**C1/C2 intake software: IMPLEMENTED by this step.**

**C1/C2 real Honor calibration dataset: NOT YET ACQUIRED.**

The previously supplied scene ISO series and dark research frames remain valuable source-bound test evidence. They are not silently relabeled as protocol-compliant calibration captures because they predate the full C0 acquisition identity record and controlled C1/C2 protocol.

## 10. Next gate after real bytes arrive

Once a real session reaches `PHASE_A_C1_C2_COMPLETE_CANDIDATE`, the next scientific step is not immediate production correction. It is:

`dataset manifests -> fit candidate models -> held-out validation -> uncertainty validation -> CalibrationPack -> exact scene admission -> shadow MeasurementLab -> Physical Promotion Gate`

Only that complete chain can make a quantity eligible for explicit production-promotion review.
