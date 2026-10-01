# TruthNegative N2 Center-Excluded Spatial Audit v0.2.2

Performance-only candidate derived from v0.2.1.

v0.2.1 re-runs the complete N2 v0.1 calculation once per reporting tile in
order to rediscover the exact coordinates where v0.1 applied a private
candidate correction. v0.2.2 records those coordinates sparsely during the
original v0.1 pass and consumes that in-memory list directly.

The v0.1 scientific hashes are intentionally unchanged: the sparse coordinate
list and per-tile offsets/counts are not included in candidate, audit or spatial
hashes. They are derived runtime indices only.

The v0.2.2 audit still reads Stage-2/source support for every actual candidate,
runs the same center-excluded predictor, preserves the same protection rules,
and remains audit-only. It does not alter source values, Scientific Master,
TruthNegative, calibration, correction authority or scientific writeback.

CI executes the legacy v0.2.1 implementation and the v0.2.2 sparse execution
path on the same synthetic RAW source and requires exact equality of every
aggregate and per-tile metric, the v0.2.1 audit SHA-256, the encoded JSON bytes,
and the encoded JSON SHA-256. v0.2.2 therefore introduces no new scientific
output schema; it is an implementation optimization for the existing v0.2.1
contract.


## Bounded runtime index

The optional sparse coordinate index is capped at **1,048,576** coordinates
(about 8 MiB of coordinate payload at two uint32 values per entry). If that
bound is exceeded, v0.1 remains scientifically valid and unchanged, the sparse
index is marked incomplete, and Android explicitly falls back to the existing
v0.2.1 rerun implementation. An incomplete index can never be consumed by the
v0.2.2 sparse executor. This bounds the optimization on very large or unusually
candidate-dense RAW sources without turning performance metadata into an
admission requirement.
