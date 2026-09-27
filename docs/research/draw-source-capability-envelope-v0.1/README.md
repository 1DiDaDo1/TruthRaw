# D.RAW Source Capability Envelope v0.1

Status: **RESEARCH INTEGRATION CANDIDATE**

The Source Capability Envelope is the machine-readable knowledge map attached
to one admitted D.RAW Observation.

Its purpose is not to make every source complete. Its purpose is to make every
known **and unknown** capability explicit before the source can participate in
cross-observation reasoning.

Permanent rule:

> **Unknown is a valid place in the Free World. Missing placement is not.**

## Lens-independent does not mean lens-blind

A main, ultra-wide, telephoto, front or external camera enters the same Free
World architecture, but retains its own instrument/source identity.

Lens role never upgrades authority. The envelope stores what this exact source
domain can support and what remains unknown.

## Source-domain identity

The envelope separates:

- capture/source route;
- sample domain;
- readout domain;
- sensor pixel mode;
- raster/CFA topology.

A field may explicitly be UNKNOWN. If its authority is UNKNOWN, v0.1 forbids a
fabricated identity value.

This is intentionally compatible with the Calibration Observation Binding
v0.1 rule that calibration cannot move between source/sample/readout domains
merely because image dimensions or CFA labels look similar.

## Capability map

Every envelope must contain these capability positions:

- sampling geometry;
- geometry / pose;
- radiometry;
- colorimetry;
- spectral;
- optical support;
- noise / uncertainty;
- temporal;
- provenance.

The authority and identity values must agree exactly with the parent
D.RAW Observation Record v0.3.

Dynamic-range/censoring support, Zero-Line gauge, temporal footprint and
optical-support state are also bound explicitly.

## Knowledge growth

v0.1 does not permit a Capability Envelope to grant:

- a cross-observation relation;
- cross-observation fusion;
- implicit calibration transfer;
- source-evidence mutation;
- Scientific Master writeback;
- appearance writeback.

A later admitted calibration may populate a currently unknown or source-bound
capability through an explicit versioned successor/binding. The old state
remains provenance.

## Camera-5 migration

The Camera-5 lamp-scene envelope contains only knowledge already present in
Observation Record v0.3.

In particular:

- sensor pixel mode: UNKNOWN;
- readout-domain identity: UNKNOWN;
- spectral support: UNKNOWN;
- optical scientific support: UNKNOWN;
- temporal footprint: UNKNOWN;
- no calibration bindings.

Those are deliberate scientific states, not missing implementation data.

The envelope itself gets a deterministic SHA-256 over canonical sorted-key
JSON. It creates no evidence.
