# TruthNegative existing-house alignment audit — 2026-09-19

Status: **RESEARCH ARCHITECTURE CORRECTION / BRANCH-LOCAL**

## Why this audit exists

The first TruthNegative v0.1 draft correctly separated measured from reconstructed support, but it risked sounding as though TruthNegative introduced a new reconstructed world between Measurement and the Scientific Master.

That would duplicate architecture TruthRaw already had.

Review of the project and the two immediately preceding project conversations shows that the reconstructed “new house” already exists and is substantially implemented.

## Existing architecture recovered from the project

TruthRaw already has:

1. immutable source RAW/CFA evidence;
2. a measurement/de-ISP domain;
3. a measured-preserving reconstruction backend;
4. a camera-native, scene-linear, full-colour reconstructed state;
5. a deterministic Scientific Master identity;
6. Dynamic Authority / uncertainty / support;
7. Open Scene / Free Scientific Space;
8. downstream virtual observation, counterfactual, appearance and export projections.

The modern Scientific Master is explicitly the reconstructed camera-native RGB state before camera-to-XYZ and before appearance/tone/export.

The current exact Scientific Master digest contract serializes camera-native R,G,B as IEEE-754 binary32 little-endian samples.

The project precision law is nevertheless mixed:

`exact source integer/packed evidence -> F32 only where demonstrated safe -> F64 branch-sensitive reconstruction -> F64 calibration/optimization/covariance -> optional validated F32 Scientific-Master storage -> higher precision reference validation`.

Therefore the “new TruthRaw float32/64 world” is not a new TruthNegative invention. It is existing TruthRaw architecture.

## Brand-independent ingress

The scientific architecture is not HONOR-specific.

Two ingress classes must remain conceptually separate:

### Native/certified source ingress

A directly supported source can enter through a native source adapter such as the strict Tile-Native DNG Source / `IRawTileSource` path.

### External/vendor RAW ingress

For other RAW brands/formats, the previously established Gatehouse concept remains the correct architecture:

`external/vendor RAW -> isolated decoder/Gatehouse -> sealed Decoded Measurement Handoff -> IRawTileSource/Main House`

The external decoder may produce an admitted decoded representation, but it may not become a second truth source or silently upgrade provenance.

The current checked-out branch contains the native DNG tile source and generic URI ingress. The broader external-RAW Gatehouse is an architectural requirement recovered from prior project work; it is not claimed here as already fully reimplemented in this branch.

## Correct role of TruthNegative

TruthNegative is **not a second new house** and must not duplicate the Latent Camera Scene or Scientific Master.

The corrected role is:

> **TruthNegative is a scientific-negative representation family bound to the already reconstructed TruthRaw house.**

It has two faces.

### A. TruthNegative Core

A provenance/authority view of the existing reconstructed camera-native scene state.

It binds:

- source evidence identity;
- measurement/reconstruction lineage;
- Scientific Master identity;
- precision contract;
- authority/support/uncertainty identities;
- one-frame/one-evidence invariants.

It does not create a second scientific state merely by naming it a negative.

### B. TruthNegative Sensor Projection

An optional reconstructed sensor-like / CFA-like / dense raster projection derived from the bound reconstructed state.

Examples:

- source-sized reconstructed sensor projection;
- 2x or 4x linear dense projection;
- `16320x12288` reconstructed negative.

Such a projection may resemble a RAW-sensor raster for compatibility, analysis or photographic-negative workflows, but it remains reconstructed. It is never the original sealed RAW and never proves hidden physical photodiodes.

## Correct flow

The project should be read as:

`RAW from any admitted brand`
`-> native ingress OR Gatehouse decoded-measurement handoff`
`-> immutable Source Evidence / admitted measurement identity`
`-> Measurement / de-ISP`
`-> Latent Camera Scene / reconstructed camera-native float state`
`-> Scientific Master identity`
`-> Dynamic Authority / uncertainty / support`
`-> Open Scene / Free Scientific Space`

TruthNegative then binds to that scientific state:

`Scientific Master + lineage + authority -> TruthNegative Core`

and can produce:

`TruthNegative Core -> reconstructed dense sensor-negative projection`

The projection is downstream representation, not upstream evidence.

## Float32 / Float64 rule

TruthNegative must inherit, not reinvent, TruthRaw precision semantics.

- Source evidence stays exact integer/packed where possible.
- Branch-sensitive reconstruction and scientific optimization use F64 where required.
- The current Scientific Master identity is binary32 camera-native RGB.
- F32 storage is permitted only where the existing validation gate says the F64-computed result can be represented safely.
- A future F64 master-storage profile may be researched, but simply storing more bits may not be described as more evidence.

## RGB versus reconstructed CFA

The project history already settled an important point:

- camera-native reconstructed RGB/full-colour scene state is the natural Scientific Master;
- reconstructed CFA/rawsensor is a downstream projection/compatibility representation;
- remosaicing a reconstructed RGB state does not turn it back into measured CFA evidence.

TruthNegative therefore must not force the entire new house back into a Bayer mosaic.

A dense sensor-like negative is an optional projection of the richer master.

## Consequence for the current Camera-5 experiment

The current physical-5 public route provides a useful source domain around `4080x3072`.

TruthNegative may eventually generate a `16320x12288` dense reconstructed negative from that source-bound scientific state.

But the reconstruction must not be described as “recovering the 15/16 missing physical sensor pixels.”

Instead:

- the sealed source supplies finite measurements;
- the TruthRaw new house supplies a reconstructed full-colour continuous/scene representation;
- TruthNegative can sample that reconstructed house onto a denser sensor-like raster;
- every newly sampled site remains reconstructed and carries uncertainty/support.

## Parallel HONOR route remains open

HONOR-route research remains independent.

It continues to ask whether a different OEM/HAL/RAW14/privileged route can reveal stronger physical source evidence.

If it does, TruthNegative can rebuild from that stronger admitted source.

TruthNegative itself may never be cited as proof that the stronger physical route existed.
