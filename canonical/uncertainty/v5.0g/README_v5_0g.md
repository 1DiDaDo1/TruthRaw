# TruthRaw v5.0g — Backend-bound Tele Uncertainty

Status: **BACKEND_BOUND_DEVELOPMENT_PASS_WAITING_PROSPECTIVE_TELE_HOLDOUT**

This branch replaces the historical v4f confidence path for the current reconstruction family. It uses leakage-free hidden-CFA recovery in float Stage-2 space, scene-held-out calibration across four inspected tele vendor DNGs, and a portable ridge + role/SNR quantile runtime.

OOF coverage: p50 0.5049, p95 0.9515.
Targets: 384,435.

It is backend-bound but **not yet prospective-certified**. A fifth genuinely new tele DNG captured after the freeze is required.

Pure Truth boundary: hidden-CFA validation is an uncertainty proxy, not co-sited RGB ground truth.

**Measured where measured. Reconstructed where necessary. Never invented.**


## Native runtime parity
C++ runtime parity: **PASS**.
`max_abs=9.14848e-09 bad=0`

## PTC bridge
The uncertainty blocker remains open until one new prospective tele DNG passes after the freeze:
`2026-09-06T15:12:24.272276+02:00`

No retuning is permitted on that future holdout if it fails.
