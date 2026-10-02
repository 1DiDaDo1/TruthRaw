# TruthRaw SHA-256 Direct-Block Transport v0.1

Performance-only transport optimization for the existing
`truthraw::sha256_v0_69::Hasher`.

## Problem

The historical `Hasher::update()` copied every input byte into its internal
64-byte staging buffer before invoking the SHA-256 compression transform.
For the v0.2.8 authority stream, each RAW contributes approximately
940,032,000 canonical authority bytes to SHA-256, so complete blocks incurred
an unnecessary extra memory copy.

## Candidate

When `update()` starts with no pending partial block:

- every complete 64-byte block is passed directly to the **unchanged**
  `transform()`;
- only the final partial tail is copied into the internal staging buffer.

When a previous update left a partial block:

- that block is completed through the historical buffered path;
- only after it is transformed may subsequent complete blocks use the direct
  path.

Final padding, message bit length, compression constants, compression rounds,
digest byte order and `finalize()` semantics are unchanged.

## Scientific / architectural authority

This is generic byte transport only.

It does not know about RAW, CFA, TruthNegative, Scientific Master, authority
roles, reconstruction or calibration. It cannot create evidence or change
scientific authority.

The generic buffered path remains necessary for arbitrary chunk boundaries.
Future callers therefore remain free to submit any valid chunking pattern.

## Required parity

The gate validates:

- standard SHA-256 empty / `abc` / long standard vector;
- one million `a` vector;
- identical digest for one-shot, 1, 7, 63, 64, 65, 2400 and mixed chunk sizes;
- direct-block transport is actually exercised for contiguous blocks;
- byte-by-byte input still exercises the buffered path;
- both transport patterns end at exactly the same digest.

Device promotion still requires full repository CI and a real-device
two-RAW timing export.
