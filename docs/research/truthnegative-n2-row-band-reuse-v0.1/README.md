# N2 Row-Band Reuse v0.1

Performance-only, transport-only candidate for the N2 local spatial path.

The established N2 v0.1 CFA audit requests many neighboring 64x64 tile
rectangles from the same immutable RAW source. On Android's tile-native DNG
adapter those requests can repeat raw/gain reads for the same vertical source
band. This candidate inserts a bounded, read-only row-band cache in front of
the existing IRawTileSource.

Scientific behavior is intentionally unchanged:

- the v0.1 audit code, tile boundaries and sample order are unchanged;
- raw and gain values returned for every requested halo coordinate must be
  byte-identical to the direct source;
- v0.1 candidate/audit/spatial/appearance hashes must be identical;
- sparse corrected-sample coordinates and per-tile metrics must be identical;
- v0.2.2 center-excluded sparse-reference output must be identical;
- cache overflow falls back to the established direct tile-read path;
- the cache is runtime-only, bounded, never evidence and never persisted;
- source samples, Scientific Master, TruthNegative and D.RAWnegative are never
  modified and no promotion/writeback is authorized.

The Android candidate uses the same wrapper for the v0.1 pass and the
v0.2.2 sparse-reference pass. Runtime telemetry reports band fills, served
requests, cache-hit requests, fallback requests and peak cache bytes so a
real-device timing run can determine whether reduced source reads translate
into lower N2_LOCAL_SPATIAL latency.
