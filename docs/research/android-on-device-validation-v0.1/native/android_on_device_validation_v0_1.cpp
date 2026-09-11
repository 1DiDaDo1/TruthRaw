#include "android_on_device_validation_v0_1.h"

#include <algorithm>
#include <fstream>
#include <limits>
#include <sstream>
#include <string>

namespace truthraw::android_validation::v0_1 {
namespace {

constexpr std::array<std::uint64_t, kLatencyBucketCount> kLatencyUpperBoundsNs{
    250000ULL,
    500000ULL,
    1000000ULL,
    2000000ULL,
    4000000ULL,
    8000000ULL,
    16000000ULL,
    32000000ULL,
    64000000ULL,
    128000000ULL,
    256000000ULL,
    512000000ULL,
    1024000000ULL,
    2048000000ULL,
    4096000000ULL,
    8192000000ULL,
    16384000000ULL,
    32768000000ULL,
    65536000000ULL,
    std::numeric_limits<std::uint64_t>::max()
};

bool valid_phase(Phase phase) noexcept {
    return static_cast<std::uint8_t>(phase) <= static_cast<std::uint8_t>(Phase::Finish);
}

bool valid_thermal(ThermalState state) noexcept {
    return static_cast<std::uint8_t>(state) <= static_cast<std::uint8_t>(ThermalState::Critical);
}

bool safe_add(std::uint64_t a, std::uint64_t b, std::uint64_t& out) noexcept {
    if (a > std::numeric_limits<std::uint64_t>::max() - b) return false;
    out = a + b;
    return true;
}

Status parse_kb_value(std::string_view valueText, std::uint64_t& outBytes) noexcept {
    std::size_t i = 0;
    while (i < valueText.size() && (valueText[i] == ' ' || valueText[i] == '\t')) ++i;
    if (i == valueText.size() || valueText[i] < '0' || valueText[i] > '9') return Status::InvalidSample;

    std::uint64_t kb = 0;
    while (i < valueText.size() && valueText[i] >= '0' && valueText[i] <= '9') {
        const auto digit = static_cast<std::uint64_t>(valueText[i] - '0');
        if (kb > (std::numeric_limits<std::uint64_t>::max() - digit) / 10ULL) return Status::Overflow;
        kb = kb * 10ULL + digit;
        ++i;
    }
    while (i < valueText.size() && (valueText[i] == ' ' || valueText[i] == '\t')) ++i;
    if (i + 2U > valueText.size() || valueText.substr(i, 2) != "kB") return Status::InvalidSample;
    i += 2U;
    while (i < valueText.size() && (valueText[i] == ' ' || valueText[i] == '\t' || valueText[i] == '\r')) ++i;
    if (i != valueText.size()) return Status::InvalidSample;
    if (kb > std::numeric_limits<std::uint64_t>::max() / 1024ULL) return Status::Overflow;
    outBytes = kb * 1024ULL;
    return Status::Ok;
}

bool severe_or_worse(ThermalState state) noexcept {
    return static_cast<std::uint8_t>(state) >= static_cast<std::uint8_t>(ThermalState::Severe);
}

bool policy_safe_for_severe_thermal(const ResourcePolicy& policy) noexcept {
    return policy.maxConcurrentHeavyRooms == 1U &&
           policy.tileSize == 128U &&
           !policy.speculativePrefetch &&
           !policy.retainRebuildableCaches &&
           policy.preferredBackend == building_runtime::v0_1::ComputeBackend::CpuBaseline;
}

bool policy_safe_for_background(const ResourcePolicy& policy) noexcept {
    return policy.maxConcurrentHeavyRooms == 1U &&
           !policy.speculativePrefetch &&
           !policy.retainRebuildableCaches;
}

std::uint64_t latency_quantile_upper_bound(const std::array<std::uint64_t, kLatencyBucketCount>& histogram,
                                           std::uint64_t count,
                                           std::uint64_t numerator,
                                           std::uint64_t denominator) noexcept {
    if (count == 0 || denominator == 0) return 0;
    const std::uint64_t target = (count * numerator + denominator - 1ULL) / denominator;
    std::uint64_t cumulative = 0;
    for (std::size_t i = 0; i < histogram.size(); ++i) {
        cumulative += histogram[i];
        if (cumulative >= target) return kLatencyUpperBoundsNs[i];
    }
    return kLatencyUpperBoundsNs.back();
}

} // namespace

Status bind_session_contract(const AdaptiveAllRoomPlan& plan,
                             const State& backplane,
                             SessionContract& out) noexcept {
    out = {};
    if (!plan.valid || !plan.resources.valid || plan.waveCount == 0U) return Status::InvalidPlan;
    if (!plan.lineage.valid || plan.lineage.backplane != &backplane) return Status::InvalidPlan;
    if (!plan.endpoints.valid || !plan.sourceSinkCountedOnce || !plan.oneSharedBackplaneForAllRooms) {
        return Status::InvalidPlan;
    }
    if (plan.sceneIsoAxisPresent || plan.resourceTierChangesTruthAuthority) return Status::InvalidPlan;
    if (plan.resources.totalWorkingSetBudgetBytes == 0U || plan.resources.tileSize == 0U ||
        plan.resources.maxConcurrentHeavyRooms == 0U) {
        return Status::InvalidPlan;
    }
    if (plan.peakResidentUpperBound == 0U ||
        plan.peakResidentUpperBound > plan.resources.totalWorkingSetBudgetBytes) {
        return Status::InvalidPlan;
    }

    if (technical_backplane::v0_1::validate(backplane) != technical_backplane::v0_1::Status::Ok) {
        return Status::InvalidBackplane;
    }
    SerializedBackplane serialized{};
    if (technical_backplane::v0_1::serialize(backplane, serialized) != technical_backplane::v0_1::Status::Ok) {
        return Status::InvalidBackplane;
    }

    out.resources = plan.resources;
    out.admittedPeakResidentUpperBound = plan.peakResidentUpperBound;
    out.baselineBackplane = serialized;
    out.valid = true;
    return Status::Ok;
}

Status parse_proc_status(std::string_view text, ProcStatusMemory& out) noexcept {
    out = {};
    bool seenRss = false;
    bool seenHwm = false;
    std::size_t pos = 0;
    while (pos <= text.size()) {
        const std::size_t end = text.find('\n', pos);
        const std::size_t length = end == std::string_view::npos ? text.size() - pos : end - pos;
        const std::string_view line = text.substr(pos, length);
        if (line.starts_with("VmRSS:")) {
            if (seenRss) return Status::InvalidSample;
            seenRss = true;
            const Status s = parse_kb_value(line.substr(6), out.rssBytes);
            if (s != Status::Ok) return s;
            out.rssKnown = true;
        } else if (line.starts_with("VmHWM:")) {
            if (seenHwm) return Status::InvalidSample;
            seenHwm = true;
            const Status s = parse_kb_value(line.substr(6), out.highWaterBytes);
            if (s != Status::Ok) return s;
            out.highWaterKnown = true;
        }
        if (end == std::string_view::npos) break;
        pos = end + 1U;
    }
    if (!out.rssKnown) return Status::IncompleteMeasurement;
    if (out.highWaterKnown && out.highWaterBytes < out.rssBytes) return Status::InvalidSample;
    return Status::Ok;
}

Status read_proc_self_status(ProcStatusMemory& out) noexcept {
    out = {};
    std::ifstream input("/proc/self/status", std::ios::in | std::ios::binary);
    if (!input) return Status::IoFailure;
    std::ostringstream buffer;
    buffer << input.rdbuf();
    if (!input.good() && !input.eof()) return Status::IoFailure;
    return parse_proc_status(buffer.str(), out);
}

Status capture_proc_memory_sample(std::uint64_t monotonicTimeNs,
                                  ThermalState thermalState,
                                  bool thermalKnown,
                                  bool foreground,
                                  Phase phase,
                                  MemorySample& out) noexcept {
    out = {};
    if (monotonicTimeNs == 0U || !valid_phase(phase) || (thermalKnown && !valid_thermal(thermalState))) {
        return Status::InvalidInput;
    }
    ProcStatusMemory proc{};
    const Status status = read_proc_self_status(proc);
    if (status != Status::Ok) return status;
    out.monotonicTimeNs = monotonicTimeNs;
    out.rssKnown = proc.rssKnown;
    out.rssBytes = proc.rssBytes;
    out.highWaterKnown = proc.highWaterKnown;
    out.highWaterBytes = proc.highWaterBytes;
    out.thermalKnown = thermalKnown;
    out.thermalState = thermalState;
    out.foreground = foreground;
    out.phase = phase;
    return Status::Ok;
}

Status SessionRecorder::begin(const SessionContract& contract) noexcept {
    if (!contract.valid || !contract.resources.valid || contract.resources.totalWorkingSetBudgetBytes == 0U ||
        contract.admittedPeakResidentUpperBound == 0U ||
        contract.admittedPeakResidentUpperBound > contract.resources.totalWorkingSetBudgetBytes) {
        return Status::InvalidInput;
    }
    *this = SessionRecorder{};
    contract_ = contract;
    begun_ = true;
    return Status::Ok;
}

Status SessionRecorder::record_memory_sample(const MemorySample& sample) noexcept {
    if (!begun_ || finalized_ || sample.monotonicTimeNs == 0U || !valid_phase(sample.phase) ||
        (sample.thermalKnown && !valid_thermal(sample.thermalState))) {
        return Status::InvalidSample;
    }
    if (haveTimestamp_ && sample.monotonicTimeNs <= lastTimestampNs_) return Status::NonMonotonicTimestamp;
    if (sample.rssKnown && sample.rssBytes == 0U) return Status::InvalidSample;
    if (sample.highWaterKnown && sample.highWaterBytes == 0U) return Status::InvalidSample;
    if (sample.rssKnown && sample.highWaterKnown && sample.highWaterBytes < sample.rssBytes) {
        return Status::InvalidSample;
    }
    if (sample.nativeHeapKnown && sample.nativeHeapArenaBytes < sample.nativeHeapAllocatedBytes) {
        return Status::InvalidSample;
    }
    if (sampleCount_ == std::numeric_limits<std::uint64_t>::max()) return Status::Overflow;

    haveTimestamp_ = true;
    lastTimestampNs_ = sample.monotonicTimeNs;
    ++sampleCount_;

    if (sample.rssKnown) {
        if (!haveBaselineRss_) {
            haveBaselineRss_ = true;
            baselineRssBytes_ = sample.rssBytes;
        }
        peakRssBytes_ = std::max(peakRssBytes_, sample.rssBytes);
        const std::uint64_t delta = peakRssBytes_ > baselineRssBytes_ ? peakRssBytes_ - baselineRssBytes_ : 0U;
        if (delta > contract_.resources.totalWorkingSetBudgetBytes) memoryBudgetExceeded_ = true;
    }
    if (sample.highWaterKnown) peakHighWaterBytes_ = std::max(peakHighWaterBytes_, sample.highWaterBytes);
    if (sample.nativeHeapKnown) {
        nativeHeapCoverage_ = true;
        peakNativeHeapAllocatedBytes_ = std::max(peakNativeHeapAllocatedBytes_, sample.nativeHeapAllocatedBytes);
        peakNativeHeapArenaBytes_ = std::max(peakNativeHeapArenaBytes_, sample.nativeHeapArenaBytes);
        maxAllocatorSlackBytes_ = std::max(maxAllocatorSlackBytes_,
                                           sample.nativeHeapArenaBytes - sample.nativeHeapAllocatedBytes);
    }
    if (sample.thermalKnown) {
        if (haveThermal_ && sample.thermalState != lastThermalState_) {
            if (thermalTransitions_ == std::numeric_limits<std::uint64_t>::max()) return Status::Overflow;
            ++thermalTransitions_;
        }
        haveThermal_ = true;
        lastThermalState_ = sample.thermalState;
        if (static_cast<std::uint8_t>(sample.thermalState) > static_cast<std::uint8_t>(maxThermalState_)) {
            maxThermalState_ = sample.thermalState;
        }
        if (severe_or_worse(sample.thermalState) && !policy_safe_for_severe_thermal(contract_.resources)) {
            runtimeReplanRequired_ = true;
        }
    }
    if (!sample.foreground && !policy_safe_for_background(contract_.resources)) {
        runtimeReplanRequired_ = true;
    }

    if (memoryBudgetExceeded_) return Status::MemoryBudgetExceeded;
    if (runtimeReplanRequired_) return Status::RuntimeReplanRequired;
    return Status::Ok;
}

Status SessionRecorder::record_tile_timing(const TileTimingSample& sample) noexcept {
    if (!begun_ || finalized_ || sample.durationNs == 0U || sample.processedPixels == 0U) {
        return Status::InvalidSample;
    }
    if (sample.phase != Phase::Pass1 && sample.phase != Phase::Pass2 && sample.phase != Phase::SinkWrite) {
        return Status::InvalidSample;
    }
    if (timingRecordCount_ == std::numeric_limits<std::uint64_t>::max()) return Status::Overflow;
    std::uint64_t newPixels = 0;
    std::uint64_t newTime = 0;
    if (!safe_add(totalTimedPixels_, sample.processedPixels, newPixels) ||
        !safe_add(totalTileTimeNs_, sample.durationNs, newTime)) {
        return Status::Overflow;
    }
    totalTimedPixels_ = newPixels;
    totalTileTimeNs_ = newTime;
    maxTileLatencyNs_ = std::max(maxTileLatencyNs_, sample.durationNs);
    ++timingRecordCount_;

    std::size_t bucket = 0;
    while (bucket + 1U < kLatencyUpperBoundsNs.size() && sample.durationNs > kLatencyUpperBoundsNs[bucket]) {
        ++bucket;
    }
    if (latencyHistogram_[bucket] == std::numeric_limits<std::uint64_t>::max()) return Status::Overflow;
    ++latencyHistogram_[bucket];
    return Status::Ok;
}

Status SessionRecorder::finalize(const State& currentBackplane, ValidationSummary& out) noexcept {
    out = {};
    if (!begun_ || finalized_) return Status::InvalidInput;
    finalized_ = true;

    SerializedBackplane currentSerialized{};
    const auto bpStatus = technical_backplane::v0_1::serialize(currentBackplane, currentSerialized);
    const bool identityPreserved = bpStatus == technical_backplane::v0_1::Status::Ok &&
                                   currentSerialized == contract_.baselineBackplane;

    out.valid = true;
    out.sampleCount = sampleCount_;
    out.timingRecordCount = timingRecordCount_;
    out.baselineRssBytes = baselineRssBytes_;
    out.peakRssBytes = peakRssBytes_;
    out.peakRssDeltaBytes = peakRssBytes_ > baselineRssBytes_ ? peakRssBytes_ - baselineRssBytes_ : 0U;
    out.peakReportedHighWaterBytes = peakHighWaterBytes_;
    out.peakNativeHeapAllocatedBytes = peakNativeHeapAllocatedBytes_;
    out.peakNativeHeapArenaBytes = peakNativeHeapArenaBytes_;
    out.maxAllocatorSlackBytes = maxAllocatorSlackBytes_;
    out.totalTimedPixels = totalTimedPixels_;
    out.totalTileTimeNs = totalTileTimeNs_;
    out.maxTileLatencyNs = maxTileLatencyNs_;
    out.p50LatencyUpperBoundNs = latency_quantile_upper_bound(latencyHistogram_, timingRecordCount_, 50U, 100U);
    out.p95LatencyUpperBoundNs = latency_quantile_upper_bound(latencyHistogram_, timingRecordCount_, 95U, 100U);
    out.timedPixelsPerSecond = totalTileTimeNs_ == 0U
                                   ? 0.0
                                   : static_cast<double>(totalTimedPixels_) * 1000000000.0 /
                                         static_cast<double>(totalTileTimeNs_);
    out.maxThermalState = maxThermalState_;
    out.thermalTransitions = thermalTransitions_;
    out.rssCoverage = haveBaselineRss_;
    out.thermalCoverage = haveThermal_;
    out.nativeHeapCoverage = nativeHeapCoverage_;
    out.memoryBudgetExceeded = memoryBudgetExceeded_;
    out.runtimeReplanRequired = runtimeReplanRequired_;
    out.scientificIdentityPreserved = identityPreserved;
    out.sceneIsoAxisPresent = false;

    if (!identityPreserved) return Status::ScientificIdentityChanged;
    if (!haveBaselineRss_ || timingRecordCount_ == 0U) return Status::IncompleteMeasurement;
    if (memoryBudgetExceeded_) return Status::MemoryBudgetExceeded;
    if (runtimeReplanRequired_) return Status::RuntimeReplanRequired;
    return Status::Ok;
}

const char* status_name(Status status) noexcept {
    switch (status) {
        case Status::Ok: return "OK";
        case Status::InvalidInput: return "INVALID_INPUT";
        case Status::InvalidPlan: return "INVALID_PLAN";
        case Status::InvalidBackplane: return "INVALID_BACKPLANE";
        case Status::InvalidSample: return "INVALID_SAMPLE";
        case Status::NonMonotonicTimestamp: return "NON_MONOTONIC_TIMESTAMP";
        case Status::Overflow: return "OVERFLOW";
        case Status::IoFailure: return "IO_FAILURE";
        case Status::IncompleteMeasurement: return "INCOMPLETE_MEASUREMENT";
        case Status::MemoryBudgetExceeded: return "MEMORY_BUDGET_EXCEEDED";
        case Status::RuntimeReplanRequired: return "RUNTIME_REPLAN_REQUIRED";
        case Status::ScientificIdentityChanged: return "SCIENTIFIC_IDENTITY_CHANGED";
    }
    return "UNKNOWN";
}

const char* phase_name(Phase phase) noexcept {
    switch (phase) {
        case Phase::Baseline: return "BASELINE";
        case Phase::Pass1: return "PASS1";
        case Phase::Pass2: return "PASS2";
        case Phase::SinkWrite: return "SINK_WRITE";
        case Phase::Finish: return "FINISH";
    }
    return "UNKNOWN";
}

} // namespace truthraw::android_validation::v0_1
