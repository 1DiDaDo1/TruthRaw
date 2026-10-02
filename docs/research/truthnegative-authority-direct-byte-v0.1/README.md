# TruthNegative Authority Direct Byte v0.1

Performance-only successor to the merged and real-device-validated authority
direct-stream v0.2.7 route.

## Purpose

v0.2.7 removed the tile-wide `std::vector<ChannelRecord>`, but device
telemetry showed that the dominant remaining authority cost was still
per-record semantic construction, generic validation, count accounting and
many small canonical SHA updates across 37,601,280 channel records per RAW.

This candidate keeps the scientific meaning unchanged while shortening only
the transport path:

1. the existing source semantics are classified once by the shared
   `classify_source_channel()` primitive;
2. a versioned `encode_source_channel_record_canonical_v1()` emits the exact
   canonical 25-byte record representation directly;
3. canonical bytes are fed to SHA in a bounded 96-record / 2400-byte batch;
4. the same role/authority/known-count accounting is updated without
   constructing a full `ChannelRecord` on the fast path.

## Flexible internal cable

The fast path is **not** the scientific contract.

The general materialized `ChannelRecord + validate_record()` path remains
implemented and authoritative as the compatibility/parity route.

The versioned encoder returns one of:

- `Encoded`
- `UnsupportedSemanticExtension`
- `Invalid`

A future semantic extension that is valid in the general contract but not yet
implemented by v1 must use `UnsupportedSemanticExtension`. The accumulator
then flushes all pending canonical bytes before falling back, for that record,
to `build_source_channel_record() + appendRecord()`. This preserves canonical
record order and prevents a fast implementation from silently discarding new
scientific fields.

`Invalid` remains fail-closed and does not fall back as though invalid source
data were merely an unsupported feature.

For the current proven source-record semantics, host and device telemetry must
show zero generic fallback records.

## Exact invariants

The candidate must preserve:

- the same canonical 64x64 tile order;
- the same CFA phase classification;
- the same measured/reconstructed/censored source semantics;
- the same bound domain and bound Float32 bits;
- the same support-known flag and support Float32 bits;
- the same p95/uncertainty semantics;
- the same contribution masks;
- the same camera-native value Float32 bits;
- the same 25 canonical bytes per source channel record;
- the same role/authority/support/bound counts;
- the same complete authority-field SHA-256;
- the same Scientific Master identity and self-gauge;
- zero creation of new evidence;
- zero calibration/correction promotion;
- zero Scientific Master writeback.

The materialized route stays available as a parity oracle and as the
compatibility route for future semantics.

## Performance telemetry

The Android candidate reports, diagnostically only:

- direct-byte encoding active;
- generic validation bypassed for the fully supported current fast path;
- canonical record byte width;
- hash batch record capacity and byte capacity;
- direct-byte record count;
- generic fallback record count;
- direct authority stream elapsed time;
- existing fused/replay/vector/scratch telemetry.

Timing and route counters are not scientific evidence and cannot modify
authority.

## Promotion rule

Do not merge/promote from host tests alone.

Required sequence:

1. exact source-record byte parity;
2. exact authority SHA/count parity against the materialized replay oracle;
3. Scientific Master parity;
4. full repository/Android CI green;
5. one real-device two-RAW export;
6. current-device fast-path count equals authority record count;
7. current-device generic fallback count is zero;
8. scientific firewall remains unchanged.
