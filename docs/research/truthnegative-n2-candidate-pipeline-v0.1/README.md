# TruthNegative N2 Candidate Pipeline v0.1

Status: EXECUTABLE SIDE-CAR VALIDATION PIPELINE — NOT YET CONNECTED TO PRO OUTPUT.

This module composes the existing N2 research primitives in one fail-closed path:

Noise/variance contract -> Structure Preservation Gate -> Authority-Aware Neighborhood -> Bounded Residual Estimator -> Audit.

It requires the structure gate sigma and neighborhood center variance to describe the same local uncertainty (variance = sigma^2). A mismatch fails closed.

The pipeline records, per candidate pixel:
- input and candidate values;
- exact correction;
- original/removed/retained residual;
- suppression fraction;
- neighborhood contributor count;
- explicit preserve reason.

The audit accumulates:
- total / eligible / corrected / preserved counts;
- protection counts for censoring, censor boundaries, unknown noise, non-measured support, weak registration/visibility, structure, missing compatible neighborhoods and residual outliers;
- total residual energy and removed residual energy;
- maximum absolute correction.

No result is allowed to overwrite Direct CFA, Scientific Master or TruthNegative. This is a side-car candidate path for A/B validation only.

Promotion gates remain external: real-RAW residual maps, flat fields, edge MTF/SFR, fine texture, low-light colour, CFA colour edges, censor boundaries, moving/occluded regions and natural scenes must show no systematic detail loss or authority violation before any PRO-output integration.
