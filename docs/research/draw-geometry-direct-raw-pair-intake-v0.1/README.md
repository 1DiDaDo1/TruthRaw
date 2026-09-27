# D.RAW Geometry Direct RAW Pair Intake v0.1

Status: **DIRECT RAW/DNG PAIR INTAKE CANDIDATE**

This module lets a geometry campaign ingest a MAIN + ULTRA_WIDE pair without
requiring Camera2 as the capture authority.

The permanent rule introduced here is broader than this module:

> **Evidence may be sealed. Thought may not be sealed.**

D.RAW therefore separates immutable evidence from revisable interpretation.

## What is sealed

Once captured and admitted, these may be immutable:

- original source bytes;
- source SHA-256;
- byte length;
- evidence count;
- physical observation identity;
- provenance statements as made at the time;
- a specific versioned certificate or validation result.

## What is not sealed

These remain open to later improvement through new evidence or versioned
successors:

- projection model family;
- distortion model family;
- relation hypotheses;
- validity-domain interpretation;
- calibration strategy;
- thresholds;
- feature detector;
- target family;
- new relation axes;
- new sensor/source types;
- future reconstruction models.

A v0.1 result may remain historically true without becoming the final theory.

## Direct pair flow

```text
MAIN original RAW/DNG
ULTRA_WIDE original RAW/DNG
        |
        v
seal pair
        |
        v
SEALED_PAIR_AWAITING_SOURCE_LOCAL_ADMISSION
        |
        +--> each source follows its own valid D.RAW ingress
        |    (Camera2 optional)
        |
        v
bind source-local admissions
        |
        v
ADMISSION_BOUND_PAIR_READY_FOR_FEATURE_EXTRACTION
        |
        v
feature extraction / geometry fit / held-out validation
        |
        v
possible GEOMETRY relation certificate
```

The pair intake never creates a geometry relation itself.

## Operator attestation

The operator may attest:

- camera system remained rigid;
- target did not move between the MAIN and ULTRA_WIDE exposure;
- the two files belong to one pose ID;
- subset is TRAINING or HOLDOUT.

These are preserved as:

`OPERATOR_ATTESTATION_ONLY`

They are not converted into measured geometry.

## Source-local admission binding

The bind step intentionally does not hard-code one admission schema version.
A future `D.RAW/SourceAdmissionPackage/*` successor may be accepted if it
still supplies the required semantic fields and invariants:

- `ADMITTED_SOURCE_LOCAL`;
- exact physical source SHA;
- physical observation ID;
- one physical frame / one independent evidence item;
- no relation or fusion granted by source admission;
- no scientific writeback.

This is a deliberate open-world evolution rule.

## Camera2

Camera2 remains valuable optional acquisition evidence. It is not a required
path into D.RAWnegative or into this pair intake.

## No premature relation authority

Even a fully bound pair still has:

- geometry relation status = UNKNOWN;
- coordinate transform allowed = false;
- equality allowed = false;
- relation fusion allowed = false;
- calibration transfer allowed = false.

The geometry campaign and its held-out validation remain the only path to a
future admitted geometry certificate.
