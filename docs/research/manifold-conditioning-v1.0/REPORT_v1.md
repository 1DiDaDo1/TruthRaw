# Manifold Conditioning v1 — research report

## Result
The contract layer is implemented and passes local strict GCC, Clang and sanitizer tests. This report does not self-certify repository closure; CI on the exact Git commit is authoritative.

## What was learned from the real dog experiment
The Virtual Observation Manifold itself is useful, but direct robust reweighting of existing supports did not generalize cleanly across R/G/B and across the three dog scenes. That candidate is retained as falsification evidence and is not silently retuned.

## New architecture
`EXACT_REPARAMETERIZATION` is the default scientific use of the manifold: it is an invertible coordinate change around one source likelihood. `ROBUST_MODEL_SELECTION` is permission-gated and fail-closed. A candidate cannot affect output merely because it looks better; frozen held-out evidence must satisfy the configured scalar-error and topology constraints.

## Real-scene exactness
Source Scene Master: `094423.scene.bin`.
The power-of-two condition/decondition test covered 12 EV nodes from -20 through +20, 37,601,280 RGB values each, for 451,215,360 comparisons. Changed IEEE-754 float32 values: 0. Maximum absolute round-trip error: 0.

## Historical robust candidate
094423 is development/tuning-exposed and therefore excluded from promotion evidence. 094414 and 094416 are frozen-parameter checks. They preserve topology regressions in the evidence record, so the historical robust-asinh triple-EV candidate is not promotable. No retuning was performed to erase this failure.

## Claim boundary
This closes neither physical ISO simulation nor co-sited missing-colour truth. It creates no photons, no new sensor samples, and no independent frames. Physical sensor-forward ISO behavior still requires the separately bound sensor/PTC model described by v0.9.
