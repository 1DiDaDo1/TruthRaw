# TruthRaw v0.80 — canonical v4.7j Adaptive Detail adapter

Status: **integration candidate**

Purpose: replace the legacy Advanced Detailed/Crisp appearance with canonical v4.7j Adaptive Detail while keeping the scientific v4.7i reconstruction translation unit exactly frozen.

## Architecture

Android continues to compile:

`canonical/reconstruction/v4.7i/native/src/core.cpp`

for Scientific Master reconstruction.

Android does **not** replace that file with the v4.7j combined core.

Instead:

`adaptive_detail_v47j_adapter.cpp`

implements the old v4.7i `IAppearanceBackend` ABI and ports only canonical v4.7j `AdaptiveDetailedCrispAppearance`.

NoiseProfile-derived sigma at 2% signal is captured from source metadata before appearance processing and stored read-only in the adapter.

## Scientific boundary

v4.7j is appearance/detail compensation.

It:
- does not alter Scientific Master;
- does not alter v0.78 channel authority;
- does not alter v0.79 uncertainty admission;
- does not create sensor/optical evidence;
- does not turn UNKNOWN into RECONSTRUCTED;
- does not claim frequencies not supported by lens/sensor evidence.

The adapter reports `ExternalProfile` through the old v4.7i ABI rather than mislabelling itself as the legacy SkinSafe profile.

## Promotion gates

1. v4.7i and v4.7j native core must be byte-identical through the start of the appearance section.
2. Adapter output must be float-bit identical to the real canonical v4.7j C++ appearance implementation.
3. Multiple NoiseProfile sigma regimes and edge/core tile placements must pass.
4. v0.79 admission must remain blocked on current camera/import routes.
5. PURE v0.63, Zero-Line, Backplane and Open Scene identities must remain unaffected.
