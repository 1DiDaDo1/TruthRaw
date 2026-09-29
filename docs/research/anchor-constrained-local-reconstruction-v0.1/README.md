# D.RAW Anchor-Constrained Local Reconstruction Audit v0.1

Date: 2026-09-29

Status: research-only, private/audit-only successor above the device-validated Raster-Independent Sample Lattice v0.1.

## Why this exists

The first real-device raster-independent lattice case proved that exact sampled
support geometry can distinguish local candidate conditions that coarse
tile-presence could not.

For tele capture `1790673150696`:

- candidate 0 nearest sampled Structure support: ~2.69 source px;
- candidate 0 r8 structure: 5/12;
- candidate 1 nearest sampled Structure support: ~8.51 source px;
- candidate 1 r8 structure: 0/12;
- candidate 1 r16 structure: 7/44.

That is enough to justify a reconstruction **audit** but not enough to enable a
correction.

## Scientific question

Can a local raster-independent reconstruction predict a temporarily hidden
real CFA measurement from surrounding measured anchors without reading the
hidden target value?

The audit answers that question by deterministic hold-out validation.

## Hold-out contract

For each visible Dark-Chroma candidate region:

1. select a deterministic subset of real source CFA anchors using the existing
   period-8 parity-preserving sample grid;
2. keep the source bytes and source sample immutable;
3. hide the target value from the private solver;
4. use only other measured anchors of the exact same CFA phase;
5. predict the hidden target;
6. reveal the true source-bound Stage-2 value only after prediction;
7. score prediction error and uncertainty coverage.

The target remains `MEASURED`. The private estimate is
`RECONSTRUCTED_PRIVATE_AUDIT_ONLY`.

## New solver

`INVERSE_VARIANCE_LOCAL_AFFINE_PLANE_V0_1`

Support:

- same exact CFA phase only;
- ±8 source-pixel local window;
- target center excluded;
- censored anchors excluded;
- paths crossing censored samples excluded;
- NoiseProfile/GainField variance used where available;
- weighted affine plane fitted in local source/lattice coordinates;
- minimum 12 admitted measured support anchors.

The target value is never inserted into the normal equations.

## Existing reference predictor

The audit also runs:

`CENTER_EXCLUDED_MULTISCALE_V0_2`

This is the already-established center-excluded symmetric H/V/diagonal
multiscale predictor. It is a reference predictor, not a winner definition.

Per holdout, the sidecar records:

- actual measured source-bound Stage-2 value;
- target variance;
- lattice solver estimate;
- solver prediction variance;
- variance inflation;
- solver absolute error;
- solver combined z diagnostic;
- reference estimate and error;
- exact source/lattice coordinate.

## Why lower hold-out error is not denoise proof

A held-out target is itself one noisy measurement.

Therefore:

- lower error means better prediction of that observed sample;
- it does not directly prove a cleaner latent scene estimate;
- it does not prove noise independence;
- it does not prove that replacing the measured anchor is allowed.

The audit records this explicitly.

## Uncertainty

The lattice solver returns a diagnostic variance derived from:

- inverse-variance weighted affine normal equations;
- local weighted residual mismatch;
- residual inflation floor >= 1.

This uncertainty is diagnostic only.

Hard state:

- `uncertainty_diagnostic_only=true`;
- `noise_independence_admitted=false`.

## Immutable-anchor law

Hard invariants:

- target source sample is never modified;
- source bytes are never modified;
- source measured anchors are never relocated;
- solver output remains reconstructed authority;
- no unanchored value is promoted to measured;
- solver is not applied to Scientific Master;
- no private correction candidate is applied;
- no new evidence is created;
- no Scientific-Master/D.RAWnegative writeback.

## Machine-readable export

The APK exposes:

`Export Anchor-Constrained Reconstruction v0.1 · JSON`

The sidecar contains all deterministic holdout records and a
`holdout_stream_sha256`.

This permits independent recomputation of MAE/RMSE, phase coverage and
per-candidate behavior.

## Next gate

The next step depends on device evidence.

No promotion rule is predeclared.

A later successor may study private values at genuinely unanchored lattice
coordinates only if the holdout audit demonstrates useful local predictive
behavior and the uncertainty diagnostics remain inspectable.

Even then:

`MEASURED != RECONSTRUCTED != APPEARANCE`
