# TruthRaw Core Vision — TruthNegative / Scientific Negative

**Status: RESEARCH-BRANCH CORE VISION — NOT YET CANONICAL MAIN PROMOTION**

Branch: `research/truthnegative-v0-1-scientific-negative-foundation`

This document defines the TruthNegative research line. It extends the existing sealed-source / Scientific-Master architecture without changing the authority of the source evidence and without closing or replacing the parallel HONOR route investigation.

> **v0.2 existing-house correction:** TruthNegative does not create a second reconstructed world between Measurement and the Scientific Master. The project already has the Latent Camera Scene, camera-native float Scientific Master and Open Scene / Free Scientific Space. TruthNegative is a scientific-negative representation family bound to that existing reconstructed house.

## 1. Why TruthNegative exists

TruthRaw now has a concrete Camera-5 case where the app-visible transport can allocate a `16320x12288` U16 envelope while the meaningful populated prefix is exactly `25,067,520` bytes, matching one `4080x3072` U16 CFA raster.

The project must not treat the empty part of the larger envelope as measured pixels.

But the source representation also does not have to remain the final spatial representation.

TruthNegative is the explicit scientific intermediate for that freedom.

## 2. Definition

> **TruthNegative is an authority-aware reconstructed scientific negative derived from one sealed evidence root. It may have a richer spatial, numerical or scene representation than the source, while preserving exact provenance and never relabelling reconstructed support as measured evidence.**

TruthNegative is **not**:

- the original sensor negative;
- a replacement source RAW;
- a DNG export;
- a claim that missing physical sensor samples have been recovered;
- an AI texture fill;
- a 200 MP measurement claim.

It is a reconstruction object.

## 3. Photographer / darkroom analogy

The source RAW is the exposed and sealed historical negative.

TruthNegative is analogous to a carefully developed working negative made from that evidence for reconstruction and printing.

The project may create a larger or denser working representation, just as a printing workflow can create a larger print surface, but it must retain which information was measured and which was developed/reconstructed.

For the current Camera-5 example:

- the `16320x12288` app-visible envelope proves that a larger transport surface exists;
- the `4080x3072` populated domain proves the meaningful app-visible RAW sample domain observed in that route;
- TruthNegative may target a denser representation such as `16320x12288`;
- the newly created positions remain `RECONSTRUCTED` unless a separate physical measurement supports them.

The scientific question is therefore not “how do we pretend the blank paper was already inked?”

It is:

> **What continuous scene/camera-domain field is best supported by the measured source, and how should that field be sampled onto a richer output lattice while retaining uncertainty and provenance?**

## 4. Continuous domain before target raster

TruthNegative should not be conceptually defined by a target pixel grid.

Preferred model:

`sealed CFA evidence -> measurement model -> continuous/latent camera-scene field -> finite target sampling grid`

The finite target grid may be:

- source-sized;
- 2x linear;
- 4x linear / 16x sample count;
- another validated projection size;
- non-raster scientific storage.

A `16320x12288` projection is therefore a **sampling choice**, not an evidence upgrade.

## 5. Authority model

TruthNegative inherits the existing TruthRaw authority vocabulary:

- `MEASURED`
- `CALIBRATED_ESTIMATE`
- `RECONSTRUCTED`
- `CENSORED`
- `UNKNOWN`
- `COUNTERFACTUAL`
- `APPEARANCE_ONLY`

Rules:

1. sealed source samples remain measured only in their admitted source identity/domain;
2. newly inferred spatial support is `RECONSTRUCTED`;
3. clipped support stays `CENSORED` unless only a bound is known;
4. unsupported support stays `UNKNOWN`;
5. appearance choices never upgrade TruthNegative;
6. counterfactual light never becomes captured truth;
7. a reconstructed target lattice does not increase `physicalFrameCount` or `independentEvidenceCount`.

## 6. Coordinate caution

A source CFA coordinate is not automatically identical to one exact coordinate in a denser target raster.

The current `4080x3072 -> 16320x12288` factor of four per axis must **not** be treated as proof that every source sample maps to every fourth physical sensor site.

Until an explicit sampling/optics/readout model is validated, the source-to-TruthNegative coordinate relation remains a reconstruction model.

This prevents a subtle but serious error: spatial upsampling must not silently become a claim about hidden physical photodiode positions.

## 7. Precision

TruthNegative follows the existing precision law:

`exact packed/integer source evidence`
`-> F64 branch-sensitive reconstruction / optimization / covariance`
`-> controlled storage representation only after equivalence validation`

F32 may be used only in stages demonstrated safe for the relevant operation.

## 8. Reconstruction requirements

A future TruthNegative reconstruction should progressively bind:

- CFA topology and colour-role identity;
- exposure/gain/readout provenance;
- black/white/censor state;
- noise model and local uncertainty;
- local gradients and edge orientation;
- cross-channel/CFA correlation;
- optical PSF/MTF/CA/shading where independently calibrated;
- clipping/censor constraints;
- scene continuity and material/geometry support where justified.

A visually plausible learned prior may assist a reconstruction proposal, but cannot by itself create scientific authority.

## 9. Measured anchors versus reconstructed field

TruthNegative preserves source measurements as immutable anchors.

A measured anchor may constrain the reconstructed field exactly at the source measurement relation established by the forward model.

The reconstructed field surrounding those anchors may be smoother, denser and full-colour, but its authority remains reconstructed.

A later renderer may sample the field at `16320x12288` or another resolution while retaining a parallel authority/support map.

## 10. Relationship to the existing reconstructed house

TruthRaw already reconstructs the new house:

`SOURCE EVIDENCE`
`-> MEASUREMENT / DE-ISP`
`-> LATENT CAMERA SCENE / CAMERA-NATIVE FLOAT RECONSTRUCTION`
`-> SCIENTIFIC MASTER IDENTITY`
`-> DYNAMIC AUTHORITY / OPEN SCENE / FREE SCIENTIFIC SPACE`
`-> APPEARANCE / EXPORT`

TruthNegative does not insert a second truth layer into that chain.

Instead:

`SCIENTIFIC MASTER + SOURCE LINEAGE + AUTHORITY/SUPPORT -> TRUTHNEGATIVE CORE BINDING`

and optionally:

`TRUTHNEGATIVE CORE -> RECONSTRUCTED SENSOR-NEGATIVE / CFA-LIKE DENSE PROJECTION`

The current Scientific Master remains the richer camera-native full-colour scene-linear state. A reconstructed CFA/rawsensor-like TruthNegative is a downstream scientific representation/projection and may not replace that master or impersonate the original sensor RAW.

This preserves the earlier project conclusion that reconstructed RGB/full-colour camera-native state is the natural scientific master, while reconstructed CFA/remosaic is a derived representation.

## 11. Relationship to the HONOR route research

TruthNegative and HONOR acquisition research are parallel.

The HONOR route continues to ask:

> Can a different OEM/privileged/acquisition path expose a larger physically populated RAW domain?

TruthNegative asks:

> Given the admitted source evidence we already have, what is the best authority-aware reconstruction we can build without claiming new measurement?

Neither branch may use the other to inflate its authority.

A future HONOR discovery may provide stronger source evidence and thereby improve TruthNegative. TruthNegative cannot be used to prove that such hidden source evidence existed.

## 12. Current first target

The first target is deliberately conservative:

- input family: sealed `4080x3072` Camera-5 CFA evidence;
- optional representation target: `16320x12288`;
- source evidence count: 1;
- target measured-claim count at v0.1 foundation: 0;
- target newly created spatial support: reconstructed only;
- source-to-target physical coordinate mapping: unresolved until explicitly modelled;
- no generative semantic texture;
- no appearance writeback.

## 13. Permanent TruthNegative law

> **A richer negative may be developed from the evidence. The developed negative may never pretend to be the original exposure.**

Equivalent TruthRaw law:

> **Measured where measured. Reconstructed where necessary. Never invented.**