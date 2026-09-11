# TruthRaw Android On-Device Validation v0.1 — Report

Status: **RESEARCH_HARNESS_HOST_CI_PASS — ANDROID_DEVICE_EVIDENCE_PENDING**

Branch: `research/android-on-device-validation-v0.1-2026-09-11`

Stacked base: Room ABI v0.2 state `497bbea144f2e0b30c203aa7c5b3f40c8f47a16e`.

Validated host implementation SHA: `b32b7a784f01a4792550e4c47cff70c55c850cd5`.

Host integrity run: `34566632602` — **SUCCESS**.

## What was added

A separate native validation layer was added above Building Runtime v0.1 and Room ABI v0.2. It measures runtime behavior without becoming a scheduler, scene model or reconstruction stage.

The module contains:

- an immutable session contract bound to `AdaptiveAllRoomPlan` plus the exact Technical Backplane;
- `/proc/self/status` parsing for `VmRSS` and `VmHWM`;
- a live self-proc reader;
- online RSS-delta accounting against the Runtime working-set budget;
- optional allocator allocated/arena accounting;
- thermal/background policy-staleness detection;
- fixed-memory tile/phase timing aggregation;
- byte-for-byte Backplane identity verification at session end.

## Host matrix

The exact implementation passed:

- upstream Building Runtime v0.1 integrity;
- upstream Technical Backplane v0.1 integrity;
- GCC 13.3.0 Release with `-Wall -Wextra -Werror`;
- Clang 18.1.3 Release with `-Wall -Wextra -Werror`;
- Clang 18.1.3 ASan+UBSan.

All three executions emitted `ANDROID_ON_DEVICE_VALIDATION_V0_1_HOST_PASS` and explicitly emitted `android_device_evidence=PENDING`.

## Deterministic validation metrics

Building Runtime policy fixtures:

- low budget: `33,554,432` bytes;
- low heavy concurrency: `1`;
- low tile: `128`;
- high budget: `268,435,456` bytes;
- high heavy concurrency: `4`;
- high tile: `512`.

Normal validation fixture:

- baseline RSS: `100 MiB`;
- peak RSS: `108 MiB`;
- peak RSS delta: `8,388,608` bytes;
- optional allocator slack proxy peak: `6,291,456` bytes;
- timing records: `3`;
- thermal transitions: `1`;
- max fixture thermal state: `Moderate`;
- timed pixels: `3,000,000`;
- summed timed duration: `14,000,000 ns`;
- max tile latency: `8,000,000 ns`;
- p50 histogram upper bound: `8,000,000 ns`;
- p95 histogram upper bound: `8,000,000 ns`.

The timing values are synthetic host fixtures used to validate aggregation logic; they are not Android performance measurements.

## Negative gates

The test suite rejects or marks for replan:

- duplicate/malformed proc RSS entries;
- non-monotonic sample timestamps;
- native heap arena smaller than allocated bytes;
- session RSS delta above Runtime working-set budget;
- high-tier policy observed under severe thermal state without conservative collapse;
- high-tier policy observed after background transition without conservative collapse;
- altered Technical Backplane identity, including altered zero-line hash;
- a Room ABI plan that exposes a scene ISO axis.

The test also confirms that the existing Building Runtime severe-thermal policy is accepted by the validator, avoiding a second or contradictory thermal policy.

## Why RSS uses a baseline delta

`totalWorkingSetBudgetBytes` is a TruthRaw execution budget, not a declaration that the complete Android process/JVM/UI must fit inside it. Therefore absolute process RSS is not a scientifically valid direct comparison.

v0.1 records the first valid session RSS as baseline and gates only the subsequent peak growth. `VmHWM` is retained as context but not used as the session gate because it may include memory peaks that happened before the session began.

This is still an approximation for real Android job memory: allocator reuse, GC/JNI behavior, file-backed pages and concurrent app activity can affect process RSS. Device validation must therefore record the actual environment and repeated runs rather than treating one RSS delta as an exact allocation ledger.

## Android bridge boundary

No Android SDK/JNI thermal implementation is claimed in v0.1. The adapter is expected to feed the same `ThermalState` already used by Building Runtime. Likewise native heap allocated/arena values are optional inputs until a trustworthy target-device allocator source is bound.

The pure C++ core has no Android SDK dependency and can therefore be tested on host CI, while the device adapter remains independently auditable.

## Scientific invariants

The validation layer contains no pixel payload and no scene ISO coordinate. It does not modify:

- Direct-CFA evidence;
- scientific master;
- source evidence hash;
- zero-line hash;
- scene-scale hash;
- physical frame count;
- independent evidence count;
- canonical reconstruction v4.7i.

Resource/thermal observations are execution evidence only and may never be promoted to scene evidence.

## Current proof boundary

Host implementation and fail-closed measurement logic are proven.

Still pending before any claim of Android validation:

1. compile/integrate the adapter in an Android app/NDK target;
2. capture real baseline/peak RSS on target hardware;
3. bind and record Android thermal state over sustained runs;
4. decide and bind a trustworthy allocator telemetry source or explicitly leave allocator coverage unavailable;
5. record real tile/pass/sink timing and throughput;
6. repeat under low-memory/background/thermal pressure;
7. verify the same Backplane before/after each session;
8. preserve physical frame count = 1, independent evidence count = 1 and scene ISO axis absent.

No FULL_PHYSICAL promotion or Android-device-performance claim is made by this report.
