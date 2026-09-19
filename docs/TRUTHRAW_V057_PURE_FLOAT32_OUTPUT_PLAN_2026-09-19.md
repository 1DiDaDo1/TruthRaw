# TruthRaw v0.57 — recovered TRUTHRAW PURE Float32 output in unified app

Date: 2026-09-19

Status: **INTEGRATION CANDIDATE / HISTORICAL PURE PIXEL ROUTE RECOVERED / MODERN CERTIFICATE + DYNAMIC-AUTHORITY BINDING STILL PENDING**

Branch:

`integration/truthraw-suite-v0-57-pure-float32-output`

Parent:

`integration/truthraw-suite-v0-56-multivendor-raw-ingress`

## Goal

Bring the historically proven 2026-09-14 TRUTHRAW PURE Float32 writer into the same Android app that now owns:

- multi-vendor file ingress;
- camera as a second ingress route;
- finalized Scientific Preview;
- bounded 16-bit Linear DNG compatibility output.

This step restores the scientific Float32 output route without changing Scientific Master equations.

## Exact route

The v0.57 export route is:

`sealed DNG source`
→ source-bound color binding
→ current Scientific Master / TruthRange / Technical Backplane finalization gate
→ exact camera-native Scientific Master identity
→ canonical 64×64 Scientific Master replay
→ exact Scientific Master digest comparison
→ `cameraToXyzD50`
→ IEEE-754 Float32 XYZ-D50 LinearRaw DNG.

The historical writer is reused rather than rewritten.

## Pixel/authority invariants

The PURE writer must report:

- `scientificMasterIdentityVerified=true`;
- `artifactCommitted=true`;
- `representationOnly=true`;
- `scientificMasterModified=false`;
- `appearanceApplied=false`;
- `counterfactualObservationCreated=false`;
- `physicalFrameCount=1`;
- `independentEvidenceCount=1`.

Finite negative values and finite values above 1.0 are counted and preserved. They are not clipped to [0,1].

## Transactional release

The native writer does not write directly to the user-visible document.

It first writes to an app-private temporary file through an `ITransactionalByteSink`.

Only after:

1. exact Scientific Master digest match;
2. writer commit;
3. post-export source re-verification;
4. all authority/evidence flags pass;

does Kotlin copy the private committed artifact to the user-selected destination.

A failed gate deletes/does not release the destination artifact.

## Relationship to the existing 16-bit Linear DNG

Keep both outputs distinct:

- **TRUTHRAW PURE Float32** — XYZ-D50 IEEE Float32, signed/overrange values preserved, exact Scientific Master digest gate;
- **Linear DNG 16-bit compatibility** — bounded unsigned compatibility projection, may clip at finite representation boundaries.

The 16-bit route may not redefine PURE.

## Historical certification boundary

The 2026-09-14 full product path also had `TRCERT01` / DNG certificate embedding.

v0.57 restores the pixel route and exact Scientific Master identity gate only.

It does **not** yet claim that a new v0.57 artifact carries the complete historical certificate record or the current canonical PTC v1.1 integration.

Legacy `TRCERT01` remains for validating historical artifacts.

A new modern certificate binding must be versioned.

## Dynamic Authority boundary

The historical PURE writer predates current Dynamic Authority/Open Scene.

v0.57 deliberately does not modify PURE pixels using Dynamic Authority.

Next integration must bind the exact current Dynamic Authority identity as provenance/authority metadata or a versioned certificate/sidecar, while preserving the exact same PURE pixel equation and Scientific Master hash.

## Input format boundary

v0.57 PURE export can only run after the common ingress has produced a finalized native scientific route.

At present this means the strict DNG path.

CR3/NEF/ARW/RAF/RW2/etc. remain decoder-pending and therefore cannot create PURE output yet.

## Nonclaims

v0.57 does not claim:

- universal DNG compatibility;
- independent physical color calibration;
- current Dynamic Authority serialization inside PURE;
- current PTC v1.1 DNG embedding;
- proprietary professional-camera RAW decoding;
- that Float32 representation creates extra measured sensor evidence.
