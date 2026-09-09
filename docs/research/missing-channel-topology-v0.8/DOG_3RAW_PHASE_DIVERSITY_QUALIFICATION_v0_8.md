# Three-dog-RAW phase-diversity qualification v0.8

**Decision: FAIL — not suitable for co-sited missing-channel topology certification.**

Registration used only directly measured green CFA evidence: the two physical green samples in each 2×2 Bayer cell were averaged to form the registration image. No reconstructed RGB channel was used as registration ground truth.

Against 094414, the 094416 frame reaches measured-green ECC ≈0.788 but requires a large affine deformation (singular values ≈1.255 and 0.890). After that global fit, 40.2% of the evaluated field still has residual flow above 0.2 raw pixel and the p95 residual is ~77 raw pixels. This is far outside the frozen phase-diversity qualification target.

094423 is a different pose/geometry and fails even more clearly: ECC ≈0.541, median residual ~2.31 raw pixels and p95 ~98 raw pixels.

Therefore these three captures remain useful **real-life single-frame TruthRaw evidence**, but are rejected for validation-only co-sited phase diversity. No missing RGB component receives `topologyCertified=true` from this sequence.
