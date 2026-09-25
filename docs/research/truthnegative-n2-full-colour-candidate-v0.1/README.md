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


## Host validation

GitHub Actions run `36196540781` completed successfully:

- GCC reference: PASS;
- Clang reference: PASS;
- ASan/UBSan: PASS;
- CTest: 100% passed.

The unit test verifies that the input Stage-2 buffer remains byte-for-byte
unchanged while an admitted local CFA correction propagates into multiple
full-colour reconstructed RGB channels.


## Reconstruction-support protection closure

Real-device Risk/Quality validation showed that a source CFA site can remain
explicitly Structure-protected while its reconstructed RGB output pixel still
changes because admitted neighboring CFA corrections participate in the
measured-preserving reconstruction support.

The Android diagnostic therefore now enables a conservative support-closure
gate.

For every protected core output pixel, the gate suppresses otherwise-admitted
N2 Stage-2 corrections within the reconstruction backend's exact
`requiredHalo()` Chebyshev radius. All N2 preserve reasons other than
`None` are treated as protected for this closure.

After full-colour reconstruction the candidate RGB of every protected core
pixel must be Float32 bit-identical to its baseline RGB. Any changed protected
RGB channel fails the diagnostic closed.

The report exposes:

- support-guard suppressed Stage-2 sites;
- protected core pixel count;
- protected core changed RGB channels (must be 0);
- reconstruction influence radius.

This remains diagnostic-only. It does not authorize production denoise and
does not modify sealed CFA, Scientific Master or TruthNegative.
