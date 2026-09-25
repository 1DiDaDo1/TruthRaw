# TruthNegative N2 Risk / Quality Audit v0.1

Status: **DIAGNOSTIC QUALITY MEASUREMENT — NO SCIENTIFIC WRITEBACK**

This audit measures the visual footprint of an already-isolated N2 full-colour
appearance candidate. It does not decide that a candidate is scientifically
correct and it does not authorize production denoise.

Inputs are the A and B display-encoded RGB crops plus the full-lattice N2
preserve-reason mask. The candidate identity SHA-256 is mandatory.

Per crop it reports:

- pixel max-channel |B-A| mean, p50, p95, p99 and maximum;
- the same distribution independently for R, G and B;
- display-luma |B-A| distribution;
- chroma-only delta distribution after removing display-luma change;
- exact source x/y of the largest pixel delta;
- distance from that maximum to the nearest Structure-protected sample;
- distance from that maximum to the nearest CENSORED or censor-boundary sample;
- structure-mask and censor-mask pixel counts;
- A/B display-luma gradient energy;
- B/A gradient-energy ratio;
- mean absolute gradient-magnitude change.

The edge metric is an encoded-display diagnostic, not an MTF or optical
measurement. It is intended to reveal candidate-induced edge suppression or
amplification before any production appearance denoise is enabled.

The quality report is cryptographically bound to the candidate identity,
A/B encoded samples, preserve-reason mask, geometry and calculated metrics.

Permanent invariants:

- no Direct CFA mutation;
- no Scientific Master mutation;
- no TruthNegative mutation;
- `createsNewEvidence=false`;
- `scientificWritebackAllowed=false`.
