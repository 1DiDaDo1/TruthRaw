# TruthRaw Android On-Device Validation v0.1

Status: **RESEARCH_HARNESS_HOST_CI_PASS — ANDROID_DEVICE_EVIDENCE_PENDING**

Validated host implementation: `b32b7a784f01a4792550e4c47cff70c55c850cd5`.

Validation: GitHub Actions run `34566632602` — **SUCCESS** under GCC Release, Clang Release and ASan+UBSan after Building Runtime v0.1 and Technical Backplane v0.1 integrity passed.

Stacked base: Room ABI v0.2 branch state `497bbea144f2e0b30c203aa7c5b3f40c8f47a16e`.

## Purpose

This module measures whether an already-admitted TruthRaw execution actually behaves within its mobile runtime envelope. It is an **observer/validator**, not a second scheduler and not a scientific reconstruction component.

Building Runtime v0.1 remains the authority for resource policy. Room ABI v0.2 remains the authority for whole-house admission. Android On-Device Validation consumes those decisions and checks runtime behavior against them.

It may never:

- alter Direct-CFA evidence;
- mutate the scientific master;
- move or redefine the global TruthRange zero-line;
- create an ISO scene axis;
- change room truth authority;
- turn runtime telemetry into scene evidence;
- change canonical reconstruction v4.7i.

## Bound session contract

`bind_session_contract()` accepts an already valid `AdaptiveAllRoomPlan` and its exact shared Technical Backplane state.

It fails closed unless:

- the Room ABI plan and ResourcePolicy are valid;
- the same Backplane object is the plan lineage binding;
- source/sink resident memory was counted once;
- one shared Backplane is used for all rooms;
- no scene ISO axis is present;
- resource tier does not change truth authority;
- the admitted peak is nonzero and within the Runtime working-set budget;
- Technical Backplane validation succeeds.

The complete serialized 180-byte Backplane is retained as the immutable before-state. `finalize()` serializes the after-state and requires byte-for-byte identity. This protects source evidence identity, scientific-master identity, zero-line identity, scene-scale identity, frame/evidence counts, room status, claim status and forbidden flags together.

## RSS measurement

The native probe parses `/proc/self/status` for `VmRSS` and `VmHWM` and includes a `read_proc_self_status()` implementation.

The memory gate intentionally uses **session RSS growth from the first valid sample**, not absolute process RSS:

`peak session RSS delta = max(VmRSS during session) - baseline VmRSS`

That delta is compared with the Building Runtime `totalWorkingSetBudgetBytes`.

This avoids treating unrelated process/JVM/UI baseline memory as if it were TruthRaw workspace. `VmHWM` is recorded as observational evidence only; because it can predate the validation session, it is not used as the session budget gate.

Host CI proves the parser and live `/proc/self/status` access on Ubuntu 24.04. It does **not** prove Android procfs behavior on the target phone; that remains device evidence pending.

## Allocator observation

`MemorySample` can carry native heap allocated and arena bytes when an Android adapter has a trustworthy allocator source. The recorder tracks the bounded proxy:

`allocator slack = arena bytes - allocated bytes`

No allocator API is guessed inside this v0.1 core. If the Android layer does not supply these values, `nativeHeapCoverage` remains false. Missing allocator telemetry may not be silently reported as zero fragmentation.

## Thermal/background observation

The validator reuses Building Runtime's `ThermalState`; it does not invent a second thermal scale.

If severe/critical thermal state is observed while the bound policy has not already collapsed to the governor's conservative form — one heavy room, 128 px tiles, no speculative prefetch, no rebuildable-cache retention and CPU baseline — the recorder returns `RUNTIME_REPLAN_REQUIRED`.

If execution moves to background while a policy still allows more than one heavy room, speculative prefetch or rebuildable-cache retention, the same replan status is raised.

Thermal state is supplied by the Android adapter. This host harness does not claim that it has already wired or validated Android SDK thermal callbacks.

## Timing and bounded telemetry

Per-operation timing records carry only phase, duration and processed-pixel count. v0.1 aggregates:

- timing-record count;
- total timed pixels;
- total tile time;
- max tile latency;
- fixed-histogram p50/p95 upper bounds;
- timed pixels/second.

The recorder does **not** retain an unbounded sample log or tile payload. Latency uses a fixed 20-bucket histogram; memory telemetry is aggregated online. This keeps the validation layer itself bounded and prevents measurement from becoming a new full-frame or unbounded-memory problem.

## Host validation result

The deterministic host fixture validates:

- low Runtime policy: 32 MiB budget, one heavy room, 128 px tile;
- high Runtime policy: 256 MiB budget, four heavy rooms, 512 px tile;
- normal session RSS delta: 8 MiB;
- optional allocator-slack fixture: 6 MiB;
- RSS-delta budget rejection;
- non-monotonic timestamp rejection;
- stale high-tier policy rejection under severe thermal state;
- stale high-tier policy rejection after moving to background;
- acceptance of the actual Building Runtime severe-thermal policy;
- byte-for-byte Backplane identity protection including zero-line identity;
- explicit rejection of a Room ABI plan with a scene ISO axis;
- physical frame count remains 1;
- independent evidence count remains 1.

## Proof boundary

The current status proves a **host-side measurement and fail-closed contract**, not Android performance.

Still open:

- running this harness inside the Android app/process;
- target-device RSS traces;
- Android allocator telemetry source and fragmentation observations;
- Android thermal callback/source binding;
- actual tile/whole-frame throughput on device;
- sustained thermal/throttling behavior;
- Vulkan behavior;
- real 200 MP / target-camera workload measurements;
- production DNG/JPEG/HEIF/AVIF output performance.

Until those measurements exist, no Android phone may be described as validated by this module.
