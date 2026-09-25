# TruthNegative N2 CFA Audit v0.1

Status: **EXECUTABLE REAL-RAW SIDE-CAR AUDIT — NO IMAGE WRITEBACK**

This is the first N2 path wired to the same streaming RAW source used by the Android Scientific Master route.

It intentionally audits the **measured CFA lattice**, not reconstructed RGB. For every sampled CFA site it reuses the exact Stage-2 source semantics from full-frame streaming, keeps the CFA phase fixed, and uses same-phase neighbours at ±2 source pixels.

Noise variance follows the already established DNG NoiseProfile Stage-2 equation:

`Var(stage2) = g * S * max(stage2, 0) + g^2 * O`

where `g` is the exact GainMap value used by Stage-2. CENSORED samples never receive a Gaussian point-noise interpretation.

The audit composes:

`measured Stage-2 CFA -> NoiseProfile variance -> spacing-aware structure gate -> uncertainty-aware same-phase neighbourhood -> bounded residual candidate -> audit`

The candidate is never fed back to Direct CFA, Scientific Master, TruthNegative, Deep Scene or Appearance. Instead the module produces deterministic candidate/report SHA-256 identities plus counts and residual-energy statistics.

Default Android sampling period is 8: one sample from each CFA parity per 8x8 source block (balanced 1/16 sampling). Host tests can use period 2 for full-lattice coverage.

Object identity remains unknown in this CFA-plane audit. The neighbourhood module therefore does not claim that neighbouring samples belong to the same physical object; it relies on the independent structure and uncertainty-compatibility gates. A future Deep Scene adapter may provide known object identity as an additional rejection gate.

Promotion to visible denoising remains forbidden until real-image validation demonstrates no systematic loss in edge MTF/SFR, fine texture, low-light colour, CFA colour edges, censor boundaries and natural detail.
