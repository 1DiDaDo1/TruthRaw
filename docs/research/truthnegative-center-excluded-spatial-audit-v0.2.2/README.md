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

CI executes v0.2.1 and v0.2.2 on the same synthetic RAW source and requires
exact equality of every aggregate and per-tile metric. The expected identity
difference is the versioned v0.2.2 audit/report hash itself.
