# TruthRaw v0.9 — real 3-RAW virtual manifold validation

Status: **REAL_SCENE_NUMERICAL_INVARIANTS_PASS_PHYSICAL_ISO_NOISE_MODEL_OPEN**

Three previously reconstructed real BnCam tele Scene Masters were used only as numerical scene inputs. No extra capture/evidence was introduced.

| Scene | min | max | negative RGB components | >1 components | max EV roundtrip error |
|---|---:|---:|---:|---:|---:|
| 094414 | -0.005658744 | 1.013005137 | 300,814 | 2 | 0 |
| 094416 | -0.005020548 | 1.730840802 | 266,402 | 6,140 | 0 |
| 094423 | -0.003741123 | 1.075762868 | 22,307 | 8,465 | 0 |

EV nodes per scene: `[-6, -4, -2, 0, 2, 4, 6, 8, 10]`
Virtual ISO/gain nodes per scene: `[100, 200, 400, 800, 1600, 3200, 6400, 12800]` (GAIN_ENCODING_ONLY).

Aggregate virtual EV views: **27**; virtual ISO encoding views: **24**. Physical source scenes remain **3**, independent evidence roots remain **3**.

Maximum normalized-weight sum error: `3.33e-16`.

Physical ISO-dependent sensor-noise simulation is intentionally rejected for these real scenes until a source-bound sensor forward model / PTC calibration is supplied.
