# TruthRaw FotoGraaf physical acquisition handoff — 2026-09-14

Status: **ACTIVE CALIBRATION-ACQUISITION CONTINUATION OVERLAY**

This handoff preserves the newest FotoGraaf work after the original knowledge-preservation pack was created. It must be read before continuing C0/C1/C2 physical calibration acquisition.

## Current chain

The research software path is now:

`capture-time identity evidence`

`-> finalized RAW/DNG bytes`

`-> Capture Evidence Seal v0.1`

`-> C0 exact identity seal`

`-> C1/C2 Calibration Intake v0.1`

`-> one dataset-manifest candidate per exact C0 scope`

`-> later model fit + held-out validation + uncertainty validation`

`-> CalibrationPack`

`-> exact scene admission`

`-> MeasurementLab shadow comparison`

`-> Physical Promotion Gate`

No stage changes a later photographed scene's normal `physicalFrameCount=1` or `independentEvidenceCount=1`.

## C0 / intake implementation anchors

C0 capture identity gate was added at commit:

`ba89ba40a1308f55cf6c9b94f82c35c46c2355ec`

C0 -> C1/C2 intake was added at:

`11d40d994c87760baec6ab7b82acd1677228e63b`

The dedicated `FotoGraaf Calibration Intake v0.1` workflow passed on that implementation line.

The intake builder:

- re-runs C0 for every capture;
- rehashes source and metadata bytes;
- keeps FIT/VALIDATION role information;
- partitions by exact `scopeKeySha256`;
- therefore prevents ordinary and exact-ISO8192-associated sample domains from being flattened together;
- does not allow ISO magnitude to select/merge sample domains;
- permits `CANDIDATE_INCOMPLETE` checkpoints but grants no calibration authority;
- recognizes `PHASE_A_C1_C2_COMPLETE_CANDIDATE` only after the planned acquisition-count matrix is satisfied.

Official Phase-A C1/C2 minimums remain:

- C1: seven acquisition anchors, four exposure times per anchor, sixteen independent darks per anchor/exposure cell;
- C2: seven acquisition anchors, twelve signal levels per anchor, eight repeats per level, four held-out validation levels, three saturation-bracket levels.

The nominal ISO anchors are planning anchors only. They do not define the actual gain/readout/sample domain.

## Capture Evidence Seal v0.1

Added at commit:

`39c6b538d7f46b835c33f55f48460c92c19ccf66`

Dedicated `FotoGraaf Capture Evidence Seal v0.1` job: **PASS** on that head.

The sealer formalizes a two-stage authority model:

1. acquisition-time capture evidence is recorded while camera/source-stack state still exists;
2. after the RAW/DNG bytes are finalized, those exact bytes are hashed and bound into the metadata snapshot + C0 record.

Explicitly rejected as authority shortcuts:

- `DNG_METADATA_ONLY` for physical camera ID;
- `EXIF_ONLY`;
- `ISO_ONLY`;
- `ISO_DERIVED`;
- filename/user guessing.

Important architectural finding:

The current TruthRaw Android UI is an **import path** using `ACTION_OPEN_DOCUMENT`. It can verify imported bytes but cannot retroactively invent acquisition-time `physicalCameraId` or equivalent hard C0 evidence. This limitation is intentional and scientifically correct.

Therefore a real Honor calibration run requires a controlled Camera2/source-side capture companion or another source application that emits equivalent acquisition-time identity evidence.

## Exact ISO8192 rule remains active

The previously observed exact-ISO8192-associated sample domain remains an observation, not a causal sensor diagnosis.

Two independent exact ISO8192 dark captures reproduced the high-scale/censored domain while nearby ISO8184 and higher ISO10244 remained ordinary.

Therefore:

- simple `ISO >= 8192` threshold: **REJECTED**;
- exact-ISO8192-associated discrete sample domain: **PASS AS OBSERVATION**;
- physical cause: **OPEN**;
- calibration selection by ISO magnitude: **FORBIDDEN**.

The new C0/intake path enforces this by using exact sample-domain identity instead of numeric ISO ordering.

## Old research frames remain test evidence

Previously supplied scene ISO ladders, response tests and covered dark frames remain valuable source-bound research evidence. They are **not silently promoted** to protocol-compliant calibration captures because they predate the complete acquisition-time C0 evidence envelope and controlled CalibrationPack acquisition protocol.

## Next implementation gate

Build a controlled Honor/Camera2 or source-side acquisition companion that emits the capture evidence envelope at capture time. It must record, rather than infer later:

- logical/system camera ID;
- physical camera ID;
- source/capture API domain;
- firmware/build identity;
- focus/stabilization state;
- direct-CFA topology;
- sample-domain classifier result;
- gain/readout-state classifier result;
- exact exposure/ISO provenance;
- calibration-role/anchor state;
- available temperature observation;
- C2 source-stability/signal-level state where applicable.

Only after the final DNG is committed may the post-finalization sealer bind its exact bytes.

Do not modify the Scientific Master/reconstruction route to solve this acquisition problem.
