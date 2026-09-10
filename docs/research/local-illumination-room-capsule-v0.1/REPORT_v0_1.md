# TruthRaw Room Capsule v0.1 implementation report

Status: `RESEARCH_PASS_MOBILE_LOCAL_RELIGHT_FRAMEWORK_PHYSICAL_RELIGHT_BLOCKED`

## Implemented

- adaptive mobile memory planner;
- vector-boundary local room representation;
- 8-byte packed geometry sample contract;
- fixed-size tile workspace independent of source megapixel count for a given device tier;
- compact ambient Boundary Illumination Envelope;
- tiny reusable light-state descriptors;
- directional and point-light relative geometry terms;
- uncertainty/confidence fail-toward-ambient behavior;
- exact outside-room passthrough;
- evidence-neutral ledger;
- explicit physical-relight fail-closed state.

## Memory policy

By default the room subsystem receives only 1/16 of Android `memoryClass`, clamped to 2–64 MiB, and no more than 8 MiB on a low-RAM tier unless a caller supplies a stricter explicit working-set ceiling. Geometry quality is reduced before the budget is violated.

This budget is deliberately separate from RAW decode/reconstruction memory.

## Falsification fixtures

- 12 MP / 25% room on a 128 MiB low-RAM policy keeps 1/4 geometry;
- ~200 MP full-frame room on the same policy must automatically coarsen to at least 1/16 geometry;
- both use the same transient tile workspace on the same device tier;
- 1000 stored light states remain <=64 kB;
- outside-room RGB is bitwise unchanged by the apply path;
- dark ambient lowers output predictably;
- a facing normal receives a directional light term while a back-facing normal does not;
- zero-confidence geometry rejects synthesized direct-light structure and falls toward the boundary envelope;
- a request for physically calibrated intrinsic relighting is rejected in v0.1;
- physical frame count and independent evidence count remain one.

## Scientific boundary

This module is not an intrinsic-image solver. It cannot establish how an unknown surface would physically respond to a new spectral illuminant from a single source frame. Its relative-light field is a counterfactual appearance tool and compact spatial front-end for later CICM/physical calibration work.

## Hardening found during adversarial audit

- An explicit caller memory ceiling is authoritative and is never rounded upward to an internal minimum.
- The Boundary Illumination Envelope now contributes an optional dominant directional term, so a compact outside-world influence is actually used rather than merely stored.
- A request for the reserved physical-relight semantics now returns a non-valid result; even a caller that ignores the status cannot accidentally apply the synthetic multiplier.

## Room-only scheduling

The plan stores the selected room bounds and derives a tile grid only over that local domain. A 25% 12-MP room therefore schedules only the room-intersecting tile grid rather than walking the full frame; the vector boundary can clip further inside each tile. This reduces compute without reducing output resolution.
