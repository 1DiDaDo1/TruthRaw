# TruthNegative Authority Pixel-Triplet v0.1

Performance-only successor to merged and real-device-validated SHA-256
direct-block transport v0.2.9.

## Motivation

The v0.2.9 source-authority path still invoked the canonical source-channel
encoder three times per pixel. Each invocation independently resolved the
same CFA phase and repeated source-level validity/classification work.

This candidate performs the source-pixel classification once and emits the
same three canonical channel records, in channel order 0, 1, 2, as one
75-byte pixel triplet.

## Exact scientific contract

The triplet fast path may not change the canonical authority stream.

For every supported source pixel:

- record 0 is exactly the existing channel-0 25-byte canonical record;
- record 1 is exactly the existing channel-1 25-byte canonical record;
- record 2 is exactly the existing channel-2 25-byte canonical record;
- concatenation is therefore exactly 75 bytes;
- CFA measured-channel semantics are unchanged;
- censor classification and source-raw-code bounds are unchanged;
- Float32 value bits are unchanged;
- creation roles, authority classes, support flags, bound flags and
  contribution masks are unchanged;
- record order, tile order, summary counts and final authority SHA-256 are
  unchanged.

The host test covers BGGR, RGGB, GRBG and GBRG, both censored and
non-censored measured samples, and compares each 25-byte slice of the
triplet with the established single-channel encoder.

## Flexible cable / fallback rule

The triplet route is a versioned specialization under the general semantic
contract. It is not a replacement for the general route.

If a future source semantic extension is not supported by the triplet
encoder, the optimized path must fail closed to the established per-channel
`ChannelRecord + validate_record()` path. It may never silently discard or
reinterpret new fields.

Current v0.2.10 device telemetry therefore reports:

- whether pixel-triplet encoding is active;
- canonical bytes per triplet;
- direct triplet count;
- generic fallback pixel count;
- the existing direct-byte record/fallback counts;
- existing SHA direct-block transport counters.

For the present proven source semantics, expected generic fallback pixel
count is zero.

## Isolation

This candidate intentionally does **not** include:

- canonical template/suffix reuse;
- authority hash batch-size changes;
- SIMD;
- center-excluded predictor changes;
- scientific reconstruction/model-selection changes.

Those remain separate candidates so host and real-device gains stay
attributable.

No source value, Scientific Master value, evidence role, calibration,
correction, candidate application or scientific writeback may change.
