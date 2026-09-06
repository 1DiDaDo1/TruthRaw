# TruthRaw v4.7i — Production Backend Port

v4.7i replaces the v4.7h end-to-end **fallback-only** execution with pluggable research/appearance backends while preserving the same orchestration, provenance, Unified Exposure Truth, and fail-closed rules.

## What is new
- deterministic research edge-aware reconstruction backend;
- exact measured-channel reinjection;
- local-support limiter for reconstructed channels to suppress unsupported ringing;
- `IAppearanceBackend` interface;
- Skin-Safe Detailed/Crisp / Color Fidelity Guard port without skin segmentation;
- exposure plan remains scene-based and shared across looks;
- independent Python<->C++ parity tests;
- four real MotionCam RAW reconstruction regressions;
- real-people Skin-Safe validation on the existing 8 fixed skin ROIs;
- semantic Vulkan shader references for the next GPU-parity step.

## Key validation results
- native core tests: 29/29 PASS;
- synthetic Python/C++ reconstruction parity: max error 0;
- synthetic Python/C++ appearance parity: max error 2.38e-6;
- four real 12.5 MP RAWs: exact measured component, all reconstructed components finite;
- support limiter reduces real-RAW reconstructed maximums from previous unconstrained ~1.43 to <= ~1.10 on the tested unity-GainMap captures;
- four real 128x128 crops: Python/C++ reconstruction max error 0;
- real people Skin-Safe validation: median deltaE00 0.216 vs Neutral, p95 0.249, p95 hue shift 0.079 degrees, median chroma +2.64%, non-skin p95 detail +5.7%.

These skin metrics are **appearance preservation relative to metadata-derived Neutral Reference**, not absolute calibrated skin truth.

## Host performance
Container CPU only, not the Magic8 Pro:
- 12.5 MP research + Neutral, 8 threads: ~0.80 s best final run;
- 12.5 MP research + Skin-Safe, 8 threads: ~1.21 s;
- 8-thread speedup vs 1-thread: ~2.69x Neutral, ~2.92x Skin-Safe.

This is intentionally not a mobile performance claim. The results show that the research/appearance kernels are the correct targets for Vulkan/Adreno acceleration.

## Status
`RESEARCH_AND_APPEARANCE_BACKEND_PORT_PASS_WAITING_VULKAN_SPIRV_AND_DEVICE_PARITY`
