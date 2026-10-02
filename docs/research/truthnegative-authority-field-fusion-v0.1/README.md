# TruthNegative Authority-Field Fused Streaming v0.1

Performance-only candidate built on the proven v0.2.5 diagnostic baseline.

The historical Android shared preparation performed:

1. Scientific Master Streaming Binding pass 1 and pass 2;
2. construction of a replay Scientific Master tile source;
3. a second full canonical source traversal to rebuild the same camera-native
   RGB tiles plus exact core RAW values for
   `TruthNegativeContinuousScientificNegative/0.5` authority hashing.

The v0.2.5 real-device telemetry showed that second authority traversal
dominating shared preparation.

This candidate does **not** define a new authority field or digest. It factors
the existing v0.5 canonical hash/count implementation into one
`AuthorityFieldAccumulator` and feeds that same accumulator from an optional
read-only observer during Scientific Master pass 1, where the exact canonical
core RAW and camera-native RGB are already present.

Hard invariants:

- same canonical 64x64 tile order;
- same `build_source_tile_records()` implementation;
- same per-record validation;
- same Float32 value bits and classification fields;
- same authority-field SHA-256 and all summary counts;
- same Scientific Master SHA-256 and TruthRange self-gauge;
- observer cannot modify source/reconstruction values;
- no new evidence, no authority promotion, no Scientific Master writeback;
- the normal field source remains available for later queries/audits;
- the historical replay path remains implemented and is used as the parity
  oracle in host validation.

The host gate requires the fused and replay authority summaries to be exactly
equal and verifies that the fused Scientific Master path performs no additional
RAW tile reads beyond the already-required two self-gauge passes.
