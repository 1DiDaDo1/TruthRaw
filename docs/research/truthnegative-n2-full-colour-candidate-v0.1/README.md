# TruthNegative N2 Full-Colour Candidate v0.1

Status: **APPEARANCE-ONLY RECONSTRUCTION CANDIDATE — NO SCIENTIFIC WRITEBACK**

This module takes a bounded Stage-2 CFA tile plus a full-lattice N2 correction
grid and runs the existing measured-preserving Float64 reconstruction twice:

1. baseline from the unmodified Stage-2 CFA tile;
2. candidate from a private copy of Stage-2 with only admitted N2 corrections.

The original Stage-2 source is never modified. The candidate reconstruction
exists only to evaluate whether the already-audited CFA correction remains
natural after the same colour reconstruction used by the Scientific Master.

The module requires a 1:1 full-lattice N2 grid (`samplingPeriod=2`) whose
region and dimensions exactly match the supplied reconstruction tile. Every
source site must map to exactly one measured CFA channel. Protected sites
therefore contribute an exact zero correction.

Permanent invariants:

- sealed Direct CFA is immutable;
- Scientific Master is immutable;
- TruthNegative is immutable;
- candidate reconstruction creates no evidence;
- scientific writeback is forbidden;
- candidate identity is cryptographically bound to the N2 correction grid and
  reconstructed candidate RGB.

This module is a validation bridge. It does not authorize production denoise.
