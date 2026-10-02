# TruthNegative Authority Direct Stream v0.1

Performance-only successor to the proven authority-field fused v0.2.6 route.

v0.2.6 removed the second full authority replay, but real-device telemetry
showed most authority work moving into Scientific Master pass 1 because each
canonical tile still materialized a `std::vector<ChannelRecord>` before the
same records were counted and hashed.

This candidate removes that temporary record vector from the fused hot path.

Scientific semantics are intentionally unchanged:

- `build_source_channel_record()` is now the single canonical source-record
  constructor;
- the established `build_source_tile_records()` materialized route calls that
  same constructor for every pixel/channel;
- the direct authority accumulator calls that same constructor one record at a
  time and immediately feeds the existing canonical count/hash routine;
- record validation, Float32 bits, CFA phase selection, censor classification,
  support, contribution mask, tile order and authority SHA-256 are unchanged;
- the historical materialized/replay route remains available as an exact
  parity oracle;
- the direct accumulator reports zero resident record-vector scratch;
- no source value, Scientific Master value, evidence authority, calibration,
  correction or scientific writeback is changed or enabled.

The device candidate exposes diagnostic-only telemetry for direct-stream
activation, temporary-vector use, authority stream time, processed tile/record
counts and accumulator resident-byte upper bound.
