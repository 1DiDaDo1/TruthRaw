# TruthNegative v0.2 — Existing-House Binding

Status: **RESEARCH — NOT MAIN-PROMOTED**

v0.2 corrects the conceptual placement of TruthNegative after auditing the existing TruthRaw architecture and previous-chat lineage.

## Core correction

TruthNegative does not create a new reconstructed world.

TruthRaw already has the reconstructed world:

- Latent Camera Scene;
- camera-native scene-linear reconstructed RGB;
- Scientific Master;
- Dynamic Authority;
- Open Scene / Free Scientific Space.

TruthNegative v0.2 binds to that world as a **scientific-negative representation family**.

## Two objects

### TruthNegative Core Binding

Binds one admitted source lineage to one Scientific Master identity.

Required fields include:

- source evidence hash;
- ingress class;
- Scientific Master hash;
- Scientific Master scientific role;
- master sample encoding;
- compute/storage precision policy;
- physical/evidence counts;
- authority separation contract.

### TruthNegative Sensor Projection Contract

Describes an optional reconstructed sensor-like projection.

The target grid may be larger than the source.

It explicitly states:

- `createsNewEvidence=false`;
- `impliesPhysicalSensorGeometry=false`;
- `newTargetSupportAuthority=RECONSTRUCTED`;
- no target-site `MEASURED` claim unless an explicit separately validated source-to-target measurement model is admitted.

## Brand-independent ingress

Supported architecture classes:

- `NATIVE_CERTIFIED_SOURCE`
- `GATEHOUSE_DECODED_MEASUREMENT_HANDOFF`

v0.2 does not assert that every commercial RAW decoder is already implemented. It ensures the scientific contract does not depend on HONOR/DNG-only assumptions.

## Precision

The default research binding is:

- source: exact integer/packed evidence;
- reconstruction compute: F64 where branch-sensitive;
- current Scientific Master storage identity: IEEE-754 binary32 camera-native RGB;
- calibration/optimization/covariance: F64;
- higher precision: reference validation.

The precision profile is not evidence authority.

## Run

`python tools/truthnegative_existing_house_binding_v02.py --self-test`

The v0.2 module remains metadata/contract-only. Pixel reconstruction starts after this alignment gate is stable.
