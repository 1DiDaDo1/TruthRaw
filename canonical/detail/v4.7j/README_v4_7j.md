# TruthRaw v4.7j — Detail Truth / Adaptive Detail

## Decision
**DETAIL_TRUTH_NATIVE_PASS_WAITING_VULKAN_AND_DEVICE_VALIDATION**

The user's observation was correct: the legacy Detailed/Crisp logic was too weak for fine dark texture, while simultaneously capable of producing excessive hard-edge acutance. v4.7j replaces the fixed detailed look with a deterministic, luminance-only, NoiseProfile-aware multiband detail compensator.

## What v4.7j does
- separates micro (3×3), fine (5×5) and texture (11×11) bands;
- scales strength from the DNG NoiseProfile-derived sigma at 2% signal;
- uses local activity so isolated noise is not treated as detail;
- uses a non-semantic hard-edge detector so steps/contours are protected without skin detection;
- uses a local support limiter to keep halos bounded;
- scales RGB together, preserving chromaticity;
- leaves the scientific Stage-2/latent/XYZ masters unchanged.

## Key validation
- native unit/integration: **29/29 PASS**
- Python↔C++ adaptive parity: **PASS**
- four real RAW reconstruction regressions: **PASS**
- four real scene detail regressions: **PASS**
- people/skin detail guard: **PASS**
- synthetic MTF / halo / noise gates: **PASS**

### Dark fine-detail response
- horizontal: legacy 0.894 → v4.7j **0.944**
- diagonal: legacy 0.833 → v4.7j **0.878**

### Real scene micro-detail p95 gain vs Neutral
- house: **1.167×**
- people: **1.130×**
- red car: **1.170×**
- ISO478 forest: **1.069×**

The high-noise forest is intentionally much more conservative than the clean red-car/house scenes.

### Artifact controls
- hard-edge halo: **2.11%** of the test contrast step
- maximum flat-noise amplification: **1.060×**
- real-scene chromaticity error: numerical float noise only (~1e-7)
- people median skin-detail ratio: **1.088×**; p95 **1.112×**

## Performance
12.53 MP host CPU reference (not Magic8 Pro):
- Adaptive Detail 1 thread: **2880.6 ms**
- Adaptive Detail 8 threads: **935.1 ms**
- 8-thread appearance pass alone: **346.5 ms**

This confirms Adaptive Detail is a high-priority Vulkan target. These are container-host measurements, not phone claims.

## Important boundary
v4.7j improves rendering of detail already supported by the reconstructed scene. It does **not** claim to recover optical frequencies that the lens/sensor never captured. Output acutance after final downscale is intentionally kept separate and is not yet promoted here.
