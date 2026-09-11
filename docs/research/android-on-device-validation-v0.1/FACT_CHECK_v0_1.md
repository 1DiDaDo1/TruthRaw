# TruthRaw Android On-Device Validation v0.1 — Fact Check

## Proven on host CI

Validated implementation: `b32b7a784f01a4792550e4c47cff70c55c850cd5`.

GitHub Actions run `34566632602` completed successfully with:

- Building Runtime v0.1 integrity PASS;
- Technical Backplane v0.1 integrity PASS;
- GCC Release PASS;
- Clang Release PASS;
- ASan+UBSan PASS.

The host tests prove the C++ contracts below.

### Resource authority remains upstream

The validator consumes `ResourcePolicy` from Building Runtime and an admitted `AdaptiveAllRoomPlan` from Room ABI v0.2. It does not derive a competing scheduler policy or modify the plan.

### Truth identity is protected

The exact 180-byte Technical Backplane before-state is serialized into the session contract. Finalization requires the complete serialized after-state to match. A changed zero-line hash is rejected with `SCIENTIFIC_IDENTITY_CHANGED`.

The host fixture retains:

- physical frame count = 1;
- independent evidence count = 1;
- forbidden flags = 0;
- scene ISO axis absent.

### RSS budget gate is session-relative

The validator uses peak `VmRSS` growth relative to the first valid session RSS sample. It does not compare absolute process RSS against the TruthRaw job budget.

The deterministic fixture has:

- baseline RSS = 100 MiB;
- peak RSS = 108 MiB;
- peak session delta = 8 MiB;
- low Runtime budget = 32 MiB.

A 40 MiB session delta against the same 32 MiB budget is rejected with `MEMORY_BUDGET_EXCEEDED`.

### Thermal/background replan is fail-closed

A nominal high-tier policy (four heavy rooms / 512 px tile) presented with severe thermal state is rejected with `RUNTIME_REPLAN_REQUIRED`.

The real Building Runtime severe-thermal policy is accepted: one heavy room, 128 px tile and CPU baseline.

A high-tier foreground policy observed after moving to background is also marked for replan when its concurrency/cache behavior is no longer conservative.

### Telemetry is bounded

The recorder stores aggregates and a fixed 20-bucket latency histogram. It does not retain full frame buffers, tile payloads or an unbounded telemetry vector.

## Not proven

The successful host run does **not** prove:

- execution on Android;
- Android `/proc/self/status` availability/semantics on the target device;
- real target-device RSS or RSS peak;
- Android allocator statistics or allocator fragmentation;
- Android thermal callback integration;
- real target-device throughput;
- thermal throttling under sustained processing;
- Vulkan performance/correctness on Android;
- 200 MP target workload behavior;
- production media encoder performance.

`read_proc_self_status()` was live-tested on the Ubuntu 24.04 GitHub runner only. The shared Linux procfs mechanism makes it a plausible Android probe, but that is an implementation hypothesis until device execution records it.

## Scientific conclusion

This v0.1 result promotes only the **measurement harness contract** to host-CI-pass status. It does not promote Android-device performance, reconstruction quality, FULL_PHYSICAL calibration, or any new scene evidence.
