# CICM <-> Room Capsule v0.1 contract

Room Capsule is a sparse spatial front-end for CICM.

- CICM owns counterfactual-world semantics and evidence boundaries.
- Room Capsule owns ROI selection, compact local geometry, boundary illumination, mobile budgeting and room-only tile scheduling.
- v0.1 Room Capsule emits only relative appearance multipliers/lighting descriptors.
- These values are not Best Conditioning EV, not Source ISO, not Simulated Capture ISO and not independent evidence.
- The scientific master and TruthRange zero line remain unchanged.
- A later physical bridge may feed calibrated local incident-radiance states into CICM only after intrinsic geometry/material/spectrum and sensor calibration are explicitly bound.

## Dependency binding

For v0.1 promotion, the satisfied CICM dependency is `71dcd031b309658ef39b99b6c2b46031f09165d9` on `main`. Room Capsule does not copy CICM state; its CI re-verifies the CICM module before validating Room Capsule.
