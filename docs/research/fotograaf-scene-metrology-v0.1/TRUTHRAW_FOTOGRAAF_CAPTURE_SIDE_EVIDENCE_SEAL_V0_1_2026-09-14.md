# TruthRaw FotoGraaf capture-side evidence seal v0.1 — 2026-09-14

**Status: RESEARCH CAPTURE BRIDGE / FAIL-CLOSED / NO CALIBRATION AUTHORITY**

This module closes the next practical gap between the physical Honor tele calibration session and the C0 -> C1/C2 intake pipeline.

The calibration intake requires exact source bytes, an exact metadata sidecar and an exact C0 identity record. The existing TruthRaw Android UI is currently an **import-oriented** route: it receives RAW files through `ACTION_OPEN_DOCUMENT`. That route can rehash imported bytes, but it cannot retroactively manufacture capture-time facts such as the physical Camera2 camera identity if the source capture app did not preserve them.

Therefore FotoGraaf now distinguishes:

`capture-time evidence envelope`

from:

`post-finalization C0 seal`.

## 1. Two-stage design

### Stage A — source-side acquisition evidence

A Camera2 calibration capture harness or source capture application records an evidence envelope while the capture state still exists.

The envelope includes observed identity such as:

- logical/system camera ID;
- physical camera ID;
- capture API/source route;
- firmware/build fingerprint;
- focus state;
- stabilization state;
- direct-CFA topology;
- capture/sample-domain classifier output;
- gain/readout-state classifier output;
- exposure and ISO metadata;
- calibration-role information.

### Stage B — seal after bytes are finalized

After the RAW/DNG file has been fully written, the sealer:

1. hashes the exact finalized source bytes;
2. canonicalizes the exact C0 scope;
3. creates the calibration metadata snapshot;
4. hashes that snapshot;
5. creates the C0 identity record;
6. hashes the C0 record;
7. re-runs the existing C0 verifier against the physical source and metadata bytes.

Only then is the capture eligible to enter the C1/C2 intake pipeline.

## 2. What cannot be recovered after the fact

The following rule is now explicit:

**Ordinary DNG/EXIF metadata alone may not prove `physicalCameraId`.**

Similarly:

- ISO magnitude may not select `captureSampleDomainId`;
- ISO magnitude may not select `gainReadoutStateId`;
- focal length or filename guessing may not replace capture-bound identity;
- an imported RAW without sufficient capture-time evidence remains usable as source/test evidence but cannot be silently upgraded into protocol-compliant physical calibration evidence.

This matters directly for the exact-ISO8192 observation. Two files with the same ISO metadata are allowed to seal into different sample domains when an admitted capture-stack classifier says they are different. Conversely, a method named `ISO_DERIVED` or `ISO_ONLY` is rejected by the sealer.

## 3. Current Android app limitation is intentional

The active TruthRaw research UI currently opens user-selected RAW/DNG files through Android's document picker. That is suitable for reconstruction and artifact validation, but it is not the acquisition-time authority source required by C0.

FotoGraaf therefore does **not** add a fake `physicalCameraId` inference to the import path.

A future controlled Camera2 calibration-capture companion may emit the evidence envelope directly. Until that exists, the capture-side envelope can also come from another source application only if it preserves the required acquisition identity with admitted methods and evidence IDs.

## 4. Envelope authority methods

The contract uses explicit method allowlists.

Examples admitted for camera identity include:

- `CAMERA2_LOGICAL_CAMERA_ID_QUERY`;
- `CAMERA2_PHYSICAL_CAMERA_RESULT`;
- source-app equivalents that preserve the same acquisition fact.

Examples admitted for sample-domain identity include:

- `RAW_PAYLOAD_AND_METADATA_CLASSIFIER`;
- `CAPTURE_STACK_DOMAIN_OBSERVATION`;
- `SOURCE_APP_SAMPLE_DOMAIN_ID`.

Examples admitted for gain/readout-state identity include:

- `CAPTURE_STACK_GAIN_READOUT_CLASSIFIER`;
- `SOURCE_APP_GAIN_READOUT_STATE_ID`;
- `CALIBRATION_CLASSIFIER_V0_1`.

Explicitly rejected inference methods include:

- `DNG_METADATA_ONLY` for physical-camera identity;
- `EXIF_ONLY`;
- `ISO_ONLY`;
- `ISO_DERIVED`;
- filename/user guesses.

These method names are provenance classes, not proof by themselves. The capture harness must bind them to an `evidenceId` describing the acquisition result/control record that produced the observation.

## 5. Measurement sidecar

The generated metadata snapshot binds:

- source SHA-256;
- C0 scope SHA-256;
- exposure time;
- ISO metadata as provenance;
- classified gain/readout state;
- BlackLevel identity;
- WhiteLevel identity;
- GainMap/opcode identity;
- focus/stabilization state;
- temperature observation.

For C2 it also binds:

- controlled signal-level ID;
- source-stability-reference ID;
- saturation-bracket flag.

The snapshot does not grant calibration authority.

## 6. Relation to C1/C2 intake

When the sealer writes its `metadata/` and `c0/` files into the same acquisition root that contains `captures/`, the resulting file names can be placed directly into the C0/C1/C2 intake session.

Recommended root:

```text
<acquisition-root>/
  captures/
  envelopes/
  metadata/
  c0/
  references/
```

Run the sealer with `--root` and `--output-dir` pointing to this same acquisition root for direct intake compatibility.

The next step is then:

`sealed capture trio -> build_fotograaf_calibration_intake_v0_1.py -> exact-scope dataset-manifest candidates`.

## 7. Files

Contract:

`docs/research/fotograaf-scene-metrology-v0.1/TRUTHRAW_FOTOGRAAF_CAPTURE_EVIDENCE_ENVELOPE_CONTRACT_V0_1.json`

Sealer:

`tools/seal_fotograaf_calibration_capture_v0_1.py`

Tests:

`tests/test_seal_fotograaf_calibration_capture_v0_1.py`

CI:

`.github/workflows/fotograaf-capture-evidence-seal-v0-1.yml`

## 8. Current status

- capture evidence envelope contract: **IMPLEMENTED**;
- exact post-finalization source hashing: **IMPLEMENTED**;
- metadata-snapshot generation: **IMPLEMENTED**;
- C0-record generation + immediate C0 revalidation: **IMPLEMENTED**;
- ISO-derived sample/gain-domain inference: **REJECTED**;
- import-only RAW path as physical-camera authority: **REJECTED**;
- controlled Honor Camera2/source-side envelope emitter: **NEXT IMPLEMENTATION GATE**;
- real Honor calibration bytes: **OPEN**;
- calibration authority granted by sealing: **NO / FORBIDDEN**.

## 9. Next implementation gate

The next software step is a small controlled acquisition companion that records the admitted envelope fields from Camera2/source-stack state at capture time and only lets the post-finalization sealer bind them after the DNG bytes are finalized.

That companion must remain separate from the normal photo reconstruction route: its job is measurement provenance, not image enhancement.
