#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <string_view>

#include "building_runtime_v0_1.h"
#include "room_abi_v0_2.h"
#include "technical_backplane_v0_1.h"

namespace truthraw::android_validation::v0_1 {

using building_runtime::v0_1::ResourcePolicy;
using building_runtime::v0_1::ThermalState;
using room_abi::v0_2::AdaptiveAllRoomPlan;
using technical_backplane::v0_1::SerializedBackplane;
using technical_backplane::v0_1::State;

constexpr std::size_t kLatencyBucketCount = 20;

enum class Status : std::uint8_t {
    Ok = 0,
    InvalidInput,
    InvalidPlan,
    InvalidBackplane,
    InvalidSample,
    NonMonotonicTimestamp,
    Overflow,
    IoFailure,
    IncompleteMeasurement,
    MemoryBudgetExceeded,
    RuntimeReplanRequired,
    ScientificIdentityChanged
};

enum class Phase : std::uint8_t {
    Baseline = 0,
    Pass1,
    Pass2,
    SinkWrite,
    Finish
};

struct ProcStatusMemory {
    bool rssKnown = false;
    std::uint64_t rssBytes = 0;
    bool highWaterKnown = false;
    std::uint64_t highWaterBytes = 0;
};

struct MemorySample {
    std::uint64_t monotonicTimeNs = 0;
    bool rssKnown = false;
    std::uint64_t rssBytes = 0;
    bool highWaterKnown = false;
    std::uint64_t highWaterBytes = 0;
    bool nativeHeapKnown = false;
    std::uint64_t nativeHeapAllocatedBytes = 0;
    std::uint64_t nativeHeapArenaBytes = 0;
    bool thermalKnown = false;
    ThermalState thermalState = ThermalState::Nominal;
    bool foreground = true;
    Phase phase = Phase::Baseline;
};

struct TileTimingSample {
    Phase phase = Phase::Pass1;
    std::uint64_t durationNs = 0;
    std::uint64_t processedPixels = 0;
};

// Immutable validation input bound to an already-admitted Room ABI v0.2 plan.
// It contains execution/resource facts and a serialized Technical Backplane
// identity only; it has no scene ISO field and no pixel payload.
struct SessionContract {
    ResourcePolicy resources{};
    std::uint64_t admittedPeakResidentUpperBound = 0;
    SerializedBackplane baselineBackplane{};
    bool valid = false;
};

struct ValidationSummary {
    bool valid = false;
    std::uint64_t sampleCount = 0;
    std::uint64_t timingRecordCount = 0;
    std::uint64_t baselineRssBytes = 0;
    std::uint64_t peakRssBytes = 0;
    std::uint64_t peakRssDeltaBytes = 0;
    std::uint64_t peakReportedHighWaterBytes = 0;
    std::uint64_t peakNativeHeapAllocatedBytes = 0;
    std::uint64_t peakNativeHeapArenaBytes = 0;
    std::uint64_t maxAllocatorSlackBytes = 0;
    std::uint64_t totalTimedPixels = 0;
    std::uint64_t totalTileTimeNs = 0;
    std::uint64_t maxTileLatencyNs = 0;
    std::uint64_t p50LatencyUpperBoundNs = 0;
    std::uint64_t p95LatencyUpperBoundNs = 0;
    double timedPixelsPerSecond = 0.0;
    ThermalState maxThermalState = ThermalState::Nominal;
    std::uint64_t thermalTransitions = 0;
    bool rssCoverage = false;
    bool thermalCoverage = false;
    bool nativeHeapCoverage = false;
    bool memoryBudgetExceeded = false;
    bool runtimeReplanRequired = false;
    bool scientificIdentityPreserved = false;
    bool sceneIsoAxisPresent = false;
};

Status bind_session_contract(const AdaptiveAllRoomPlan& plan,
                             const State& backplane,
                             SessionContract& out) noexcept;

Status parse_proc_status(std::string_view text, ProcStatusMemory& out) noexcept;
Status read_proc_self_status(ProcStatusMemory& out) noexcept;
Status capture_proc_memory_sample(std::uint64_t monotonicTimeNs,
                                  ThermalState thermalState,
                                  bool thermalKnown,
                                  bool foreground,
                                  Phase phase,
                                  MemorySample& out) noexcept;

class SessionRecorder {
public:
    Status begin(const SessionContract& contract) noexcept;
    Status record_memory_sample(const MemorySample& sample) noexcept;
    Status record_tile_timing(const TileTimingSample& sample) noexcept;
    Status finalize(const State& currentBackplane, ValidationSummary& out) noexcept;

private:
    SessionContract contract_{};
    bool begun_ = false;
    bool finalized_ = false;
    bool haveTimestamp_ = false;
    std::uint64_t lastTimestampNs_ = 0;
    bool haveBaselineRss_ = false;
    std::uint64_t baselineRssBytes_ = 0;
    std::uint64_t peakRssBytes_ = 0;
    std::uint64_t peakHighWaterBytes_ = 0;
    std::uint64_t peakNativeHeapAllocatedBytes_ = 0;
    std::uint64_t peakNativeHeapArenaBytes_ = 0;
    std::uint64_t maxAllocatorSlackBytes_ = 0;
    bool haveThermal_ = false;
    ThermalState lastThermalState_ = ThermalState::Nominal;
    ThermalState maxThermalState_ = ThermalState::Nominal;
    std::uint64_t thermalTransitions_ = 0;
    std::uint64_t sampleCount_ = 0;
    std::uint64_t timingRecordCount_ = 0;
    std::uint64_t totalTimedPixels_ = 0;
    std::uint64_t totalTileTimeNs_ = 0;
    std::uint64_t maxTileLatencyNs_ = 0;
    std::array<std::uint64_t, kLatencyBucketCount> latencyHistogram_{};
    bool nativeHeapCoverage_ = false;
    bool memoryBudgetExceeded_ = false;
    bool runtimeReplanRequired_ = false;
};

const char* status_name(Status status) noexcept;
const char* phase_name(Phase phase) noexcept;

} // namespace truthraw::android_validation::v0_1
