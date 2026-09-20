# TruthRaw v0.81 — canonical v4.7k Output Acutance bridge

Status: **research/integration candidate**

v0.81 places canonical v4.7k at the output boundary, not in reconstruction.

## Required order

`final resized linear SDR base -> canonical v4.7k output acutance -> HDR gain rebase against the adjusted base -> shoulder/OETF/encoding`

The module calls the canonical C++:
- `choose_output_acutance_plan()`
- `apply_output_acutance()`

No copy/reimplementation of the v4.7k filter math is used.

## HDR rebase

The existing Advanced HDR transport represented a bounded display gain derived before output acutance.

For each non-censored preview pixel, v0.81:
1. computes the old effective display target from the pre-acutance final SDR base and existing bounded HDR gain;
2. applies canonical v4.7k to the final resized SDR base;
3. derives a new direct display gain relative to that acutance-adjusted base;
4. re-expresses gain only where a positive upstream HDR relation already existed;
5. keeps an upstream unity/no-HDR pixel exactly at gain 1;
6. caps the re-expressed gain at the existing maximum display gain;
7. forces censored support to gain 1.

This does not create a new HDR target or new evidence. It only changes the coordinate/base against which the existing downstream display relationship is expressed.

## Authority boundary

v4.7k:
- never touches RAW/CFA;
- never touches Scientific Master;
- never changes Zero-Line/scene-scale/Backplane;
- never upgrades v0.78 authority;
- never changes v0.79 uncertainty admission;
- never claims optical-frequency recovery;
- is presentation/output acutance only.

## Test gates

The standalone tests require:
- exact canonical plan float-bit parity;
- exact canonical acutance output float-bit parity;
- HDR target preservation in the unclamped domain;
- gain >= 1 and <= the existing display maximum;
- upstream zero-HDR pixels remain exactly gain 1;
- censored pixels always gain 1;
- invalid shapes/noise/resize contracts fail closed.
