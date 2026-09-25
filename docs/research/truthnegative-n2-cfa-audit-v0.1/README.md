# TruthNegative N2 CFA Audit v0.1

Status: **ANDROID-COMPILED REAL-RAW SIDE-CAR AUDIT — AUDIT-ONLY, NO IMAGE WRITEBACK**

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


## Android runtime integration validation

The audit is now compiled into the PRO TruthNegative Continuous Android bridge on branch `integration/pro-truthnegative-continuous-primary-route-v075`.

The bridge executes the N2 CFA audit after the raster-independent TruthNegative state is finalized and before visible Free-World/Appearance output is produced. Candidate values are exported only as diagnostics; `candidateAppliedToAppearance=false` is fail-closed in the Kotlin packet contract.

Host algorithm validation:
- GitHub Actions run `36158104992`: GCC PASS, Clang PASS, ASan/UBSan PASS.

Signed Android ARM64 integration validation:
- GitHub Actions run `36159730145`: SUCCESS.
- validated head: `1cbaa75f3d521d5d73afcde470d9eec76341b886`
- APK bytes: `6521987`
- APK SHA-256: `21f2c8a1b4acb580f86dfb0ff35ffebe4f55e95d51669171e482400d9469fa8d`
- stable development signing certificate SHA-256: `a6288a4b7e9d18e908eeba37540cd962b687f2290f7e45d7c7ad3390ad61fd44`
- workflow artifact ID: `10874234464`

This validates compilation/integration and synthetic invariants. It does **not** yet validate denoise quality on a physical-device RAW. Real-device N2 audit metrics remain the next evidence gate.
