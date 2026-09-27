# D.RAW Source Pre-Admission Manifest v0.2

Status: **CURRENT PRE-ADMISSION SUCCESSOR CANDIDATE**

v0.2 is a strict successor to the Phase-A manifest from Source Admission
Package v0.1. It exists because the first real main-camera candidate proves
that serialized storage facts may be known while the physical capture/sample
domain is still unknown.

Permanent rule:

> **A known storage domain must never be mistaken for a known capture/readout domain.**

## What v0.2 changes

v0.1 required one `sample_domain_id`. That was too coarse.

v0.2 separates:

- serialized storage domain — what bytes/container/topology are actually present;
- capture sample domain — physical/readout sample-domain identity, if proven;
- readout domain — gain/readout state identity, if proven;
- sensor pixel mode — if proven.

Each physical-domain field has its own `value + authority` pair.

`UNKNOWN` requires a null value. A string such as `"UNKNOWN"` is not a
valid identity.

## Main-camera discovery that motivated v0.2

The recovered 2026-09-14 main candidate has:

- logical rear host Camera 0;
- active physical result Camera 2;
- requested physical camera = null;
- RAW_SENSOR;
- 4096×3072 BGGR;
- one physical frame / one independent evidence item;
- source DNG hash verified from the original bytes;
- exact sensor/image timestamp match.

Its legacy acquisition record explicitly listed `captureSampleDomainId` and
`gainReadoutStateId` as missing.

Therefore the correct pre-admission state is:

- serialized storage domain: SOURCE_BOUND;
- capture sample domain: UNKNOWN;
- readout domain: UNKNOWN;
- sensor pixel mode: UNKNOWN.

No calibration, Scientific Master, D.RAWnegative, graph relation or fusion is
created by this manifest.

## Relationship to later admission

Pre-Admission v0.2 is still Phase A only.

The source must later pass the normal scientific route to produce:

1. D.RAW Observation Record v0.3;
2. Source Capability Envelope v0.1 or a future versioned successor;
3. Final Source-Local Admission Package.

Until then the source remains:

`CANDIDATE_NOT_SCIENTIFICALLY_ADMITTED`
