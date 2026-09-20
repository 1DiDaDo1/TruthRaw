# TruthRaw Android Acceleration v0.1

Status: RESEARCH / PERFORMANCE ONLY

This work must not redefine Source Evidence, Scientific Master identity,
authority, reconstruction semantics, or provenance. A faster backend is never
allowed to become more authoritative merely because it is faster.

## Baseline audit

The current Android integration is arm64-only and uses C++20. The suite target
does not currently contain Vulkan, OpenMP, explicit Neon vector kernels, or an
Android native worker pool outside algorithms that already own threading.

Important current hotspots:

1. Full-frame streaming v0.1 is sealed and performs a two-pass bounded-memory
   pipeline. Pass 2 reconstructs tiles again. The sealed implementation must not
   be edited for acceleration.
2. v4.7i reconstruction is branch-heavy and numerically sensitive. The
   non-streaming canonical processor already contains CPU threading; the sealed
   streaming path remains intentionally bounded and sequential at tile
   orchestration level.
3. v4.7j Adaptive Detail contains several full-tile luminance/gradient/integral
   passes and is a strong data-parallel candidate.
4. v4.7k Output Acutance contains separable blur, Sobel-like gradient and final
   luminance scaling passes and is a strong data-parallel candidate.
5. photo_export_bridge performs additional full-resolution Light/HDR/
   Restoration/NV21 work.
6. output-channel authority v0.84 scans source support and hashes three
   authority records per output pixel. Its integer classification can be
   parallelized, but canonical hash ordering must remain deterministic.
7. truthraw_sha256_v0_69 is currently a portable scalar SHA-256 implementation.
   ARMv8 SHA2 instructions can accelerate this without changing one digest bit.

## Platform findings

### Generic GPU: Vulkan first

Android recommends Vulkan compute for workloads that benefit from direct GPU
compute control. Vulkan FP32 arithmetic is mandatory. Vulkan must be detected at
runtime and a CPU/software Vulkan implementation must never be selected for a
performance path.

Android's Vulkan migration documentation also documents
VK_ANDROID_external_memory_android_hardware_buffer and states that the
extension is available on Vulkan 1.1 Android devices. This is important for a
future zero-copy or reduced-copy pixel transport.

Sources:
- https://developer.android.com/guide/topics/renderscript/migrate/migrate-vulkan
- https://developer.android.com/ndk/guides/graphics/design-notes
- https://developer.android.com/ndk/guides/graphics/android-vulkan-profile
- https://developer.android.com/ndk/reference/group/a-hardware-buffer

### CPU SIMD

All arm64 Android devices support Advanced SIMD/Neon and the NDK enables Neon
for Arm ABIs. TruthRaw should establish an optimized CPU baseline before
judging GPU speedups.

Optional CPU features such as SHA2 must be discovered with getauxval(AT_HWCAP)
or an equivalent robust feature library.

Sources:
- https://developer.android.com/ndk/guides/cpu-arm-neon
- https://developer.android.com/ndk/guides/cpu-features

### Qualcomm

FastCV is an official mobile-optimized CV library and Qualcomm documents
hardware acceleration/tuning on Snapdragon. It is suitable only where a FastCV
primitive has the same declared numerical role as a TruthRaw operation.

Qualcomm AI Engine Direct / QNN can target Kryo CPU, Adreno GPU and Hexagon NPU,
but it is an AI graph/runtime stack. It is not the default route for arbitrary
scientific Float32 raster math.

Sources:
- https://www.qualcomm.com/developer/software/qualcomm-fastcv-sdk
- https://www.qualcomm.com/developer/software/qualcomm-ai-engine-direct-sdk

### MediaTek

NeuroPilot is primarily an on-device AI/NPU ecosystem. It can become an
optional backend for explicitly admitted ML/tensor tasks, but it must not be
treated as a general replacement for deterministic RAW reconstruction.

Source:
- https://neuropilot.mediatek.com/

### APIs explicitly not selected

- RenderScript: deprecated since Android 12; manufacturers have stopped
  providing hardware acceleration.
- NNAPI: deprecated in Android 15; Android recommends newer ML runtimes.
- Direct vendor OpenCL: not the portable TruthRaw base. It may exist underneath
  vendor stacks, but TruthRaw should not depend on undocumented OpenCL presence.

Sources:
- https://developer.android.com/guide/topics/renderscript/migrate
- https://developer.android.com/ndk/guides/neuralnetworks

## Compute authority classes

Every accelerated kernel is assigned one of these classes.

### EXACT_SCIENTIFIC

Required output is bit-exact with the canonical CPU reference for every admitted
test vector. Examples:
- SHA-256
- integer/authority classification if operation ordering is canonical
- future exact reconstruction backend only after exhaustive equivalence proof

A mismatch disables the backend.

### BOUNDED_SCIENTIFIC

Reserved. Not enabled by v0.1. A numerical tolerance is not sufficient on its
own to redefine Scientific Master identity.

### APPEARANCE

Numerically bounded output is acceptable when the operation is already
appearance-only and its policy defines the tolerance. Examples:
- Natural HDR presentation
- Light appearance
- Adaptive Detail
- Output Acutance

### PRESENTATION

Display/export-only operations, such as final format conversion, preview
resampling and display transfer.

## Runtime router contract

The router never selects by marketing name alone.

1. Discover capabilities.
2. Reject software/emulated GPU devices.
3. Record API version, vendor ID, device ID, driver version, queue features and
   required extensions.
4. Run a fixed correctness self-test per kernel/backend.
5. Benchmark only backends that pass correctness.
6. Cache the result by app/kernel version + Android build fingerprint + GPU
   vendor/device/driver identity.
7. Re-test after app update, OS update or GPU driver change.
8. At runtime, consider thermal headroom and memory pressure.
9. On any backend error, fail over to the CPU reference without changing
   scientific authority.

## Initial backend order

1. CPU_REFERENCE
2. CPU_OPTIMIZED_ARM64
   - compiler optimized release baseline
   - Neon/portable vectors where bit/ULP policy permits
   - optional ARMv8 SHA2 transform for exact SHA-256
3. VULKAN_GENERIC
   - first for APPEARANCE/PRESENTATION kernels
4. QUALCOMM_FASTCV_OPTIONAL
   - only exact semantic primitive matches
5. QUALCOMM_QNN_OPTIONAL
   - AI/tensor workloads only
6. MEDIATEK_NEUROPILOT_OPTIONAL
   - AI/tensor workloads only

## First benchmark targets

Measure on release-like native optimization, not only the debug APK.

- source sealing SHA-256
- Scientific Master digest
- v0.84 authority-content hashing
- v4.7i reconstruction tile
- camera RGB -> XYZ and XYZ -> linear sRGB
- v4.7j Adaptive Detail
- Light presentation
- HDR presentation/rebase
- Restoration reintegration
- v4.7k Output Acutance
- RGB -> NV21
- Float32 DNG serialization

Record:
- wall time
- CPU time
- peak resident memory
- bytes read/written
- thermal headroom
- backend identity
- correctness result
- output digest where applicable

## Profiling

Use native ATrace sections around bridge-level stages so Perfetto can show
TruthRaw work beside CPU scheduling, I/O and thermal behavior. For Vulkan,
Android GPU Inspector / Android Performance Analyzer can validate GPU occupancy,
memory traffic and synchronization.

Sources:
- https://developer.android.com/tools/perfetto
- https://developer.android.com/topic/performance/tracing/custom-events-native
- https://developer.android.com/agi/start

## Thermal behavior

A backend that wins a 500 ms microbenchmark can lose a 20-minute 200 MP
workflow after throttling. The router therefore needs sustained tests and may
use Android thermal headroom to reduce concurrency or change backend without
changing image semantics.

Source:
- https://developer.android.com/reference/android/os/PowerManager#getThermalHeadroom(int)

## Non-negotiable TruthRaw rules

- Sealed source bytes are immutable.
- Full-frame-streaming-v0.1 stays byte-frozen.
- Canonical v4.7i remains the CPU reference.
- Accelerator availability is not evidence.
- Accelerator choice is provenance/performance metadata only.
- Appearance acceleration cannot write back into Scientific Master.
- A vendor backend that fails validation is disabled, not repaired by silently
  weakening the scientific contract.


## Implementation state — 2026-09-21

Implemented on `integration/truthraw-suite-v0-84-2-adaptive-compute-router`:

- strict-FP `-O2` for the Android debug native library;
  - `-fno-fast-math`;
  - `-ffp-contract=off`;
  - canonical v4.7i O0/O2 fixture signature is required to match before APK build;
  - current proven signature: `5dba058e5742346a`.
- bounded ordered multicore executor:
  - tile compute may run concurrently;
  - commit/write/hash stays canonical and ordered;
  - host test proves concurrency, ordering and fail-fast behavior.
- Full-resolution Restoration:
  - dynamic CPU worker count from memory/power/thermal/resource-headroom policy;
  - one read-only DNG source + reconstruction workspace per worker;
  - canonical commit keeps scientific/master/provenance ordering stable;
  - worker count is runtime telemetry only and is not embedded into the `.trr` identity.
- ADPF CPU worker hints:
  - optional one-thread hint session per Restoration worker;
  - actual tile duration is reported after successful tile compute;
  - unsupported/error state fails open to normal Android scheduling;
  - ADPF never changes image authority.
- Android 16+/17 resource headroom:
  - runtime-dynamic `ASystemHealth_getCpuHeadroom` and
    `ASystemHealth_getGpuHeadroom` lookup through `libandroid.so`;
  - no compileSdk-36 dependency is introduced;
  - values are cached according to the device's reported minimum poll interval;
  - CPU headroom can reduce worker count;
  - low GPU headroom can temporarily suppress future Vulkan candidacy.
- Vulkan:
  - hardware loader/device/compute-queue/AHardwareBuffer extension probe exists;
  - software/CPU Vulkan devices are rejected as performance candidates;
  - no Vulkan pixel kernel is promoted yet.
- Qualcomm APV / Android APV:
  - read-only `video/apv` MediaCodec encoder/decoder probe;
  - hardware/vendor/software flags, profile-levels, color formats,
    max resolution/bitrate and performance points are reported in PRO;
  - APV is a professional-video/intermediate candidate only;
  - Scientific Master replacement is explicitly false.
- PRO diagnostics:
  - CPU/NEON/SHA2 capability;
  - Vulkan hardware identity/capabilities;
  - CPU thermal headroom;
  - Android CPU/GPU resource headroom;
  - current dynamic worker plan;
  - APV codec diagnostics.

Still research/not selected:

- Vulkan pixel kernels;
- ARMv8 SHA2 implementation in TruthRaw's SHA-256 transform;
- Qualcomm FastCV kernels;
- Qualcomm QNN/Hexagon tensor routes;
- MediaTek NeuroPilot/Neuron routes;
- parallel Scientific Master v0.3 replacement;
- APV encode/decode production route;
- OpenAPV bundling;
- general multicore replacement of byte-frozen `full-frame-streaming-v0.1`.

The byte-frozen `full-frame-streaming-v0.1` module remains unchanged. A parallel
Scientific Master plan exists in `PARALLEL_SCIENTIFIC_MASTER_PLAN.md`; it is
not production-selected until bit-exact reference gates are passed.
