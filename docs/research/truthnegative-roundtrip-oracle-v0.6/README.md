# TruthNegative Round-Trip / Explainability Oracle v0.6

Status: **EXECUTABLE RESEARCH ORACLE**

This oracle deliberately does **not** claim arbitrary resampling is invertible.

Instead it tests the invariants that a raster-independent TruthNegative scientific negative must preserve when it is queried onto finite lattices.

## What it verifies

For one immutable TruthNegative Continuous v0.5 state, the oracle evaluates multiple target rasters and requires:

- the TruthNegative state SHA-256 never changes;
- every target pixel has zero measured-target claims;
- every source footprint is positive and normalized;
- one physical frame and one independent evidence item remain one;
- no scientific writeback appears;
- every raster's global scene-linear area average matches the canonical 1x1 whole-frame area integral within a declared Float64 tolerance;
- query chains are deterministic and hashed.

The global-area conservation test is important because it proves the finite target grids are different partitions of the same continuous scene field rather than independent image worlds.

## What it does not prove

It does not prove:

- perfect inversion of a resize;
- recovery of frequencies not supported by the source;
- optical super-resolution;
- hidden scene recovery;
- new measured pixels.

The oracle is about **identity, conservation, authority and explainability**.

## Next gate

The next safe optics step is to bind measured/calibrated PSF/MTF to the scientific-negative support model. Inferred optics may be explored, but must not be admitted as scientific calibration.
