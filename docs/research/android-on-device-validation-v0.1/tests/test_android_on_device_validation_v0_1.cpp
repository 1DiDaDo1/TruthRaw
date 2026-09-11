#include "android_on_device_validation_v0_1.h"

#include <cmath>
#include <cstdint>
#include <iostream>
#include <string_view>

namespace av = truthraw::android_validation::v0_1;
namespace br = truthraw::building_runtime::v0_1;
namespace ra = truthraw::room_abi::v0_2;
namespace tb = truthraw::technical_backplane::v0_1;

namespace {

constexpr std::uint64_t MiB = 1024ULL * 1024ULL;
int failures = 0;

void expect(bool condition, std::string_view label) {
    if (!condition) {
        std::cerr << "FAIL: " << label << '\n';
        ++failures;
    }
}

tb::State make_backplane() {
    tb::State state{};
    for (std::size_t i = 0; i < tb::kHashBytes; ++i) {
        state.sourceEvidenceHash[i] = static_cast<std::uint8_t>(i + 1U);
        state.scientificMasterHash[i] = static_cast<std::uint8_t>(i + 33U);
        state.zeroLineHash[i] = static_cast<std::uint8_t>(i + 65U);
        state.sceneScaleHash[i] = static_cast<std::uint8_t>(i + 97U);
    }
    state.physicalFrameCount = 1;
    state.independentEvidenceCount = 1;
    state.roomStatus.fill(tb::RoomStatus::Available);
    state.claimStatus = tb::ClaimStatus::Candidate;
    state.forbiddenFlags = 0;
    return state;
}

ra::AdaptiveAllRoomPlan make_plan(const br::ResourcePolicy& policy,
                                  const tb::State& backplane,
                                  std::uint64_t admittedPeakBytes) {
    ra::AdaptiveAllRoomPlan plan{};
    plan.valid = true;
    plan.resources = policy;
    plan.lineage.backplane = &backplane;
    plan.lineage.valid = true;
    plan.endpoints.valid = true;
    plan.endpoints.sourceResidentUpperBound = 4812;
    plan.endpoints.sinkResidentUpperBound = 65536;
    plan.waveCount = 1;
    plan.waves[0].wave = 0;
    plan.waves[0].totalResidentUpperBound = admittedPeakBytes;
    plan.peakResidentUpperBound = admittedPeakBytes;
    plan.sourceSinkCountedOnce = true;
    plan.oneSharedBackplaneForAllRooms = true;
    plan.sceneIsoAxisPresent = false;
    plan.resourceTierChangesTruthAuthority = false;
    return plan;
}

av::MemorySample memory_sample(std::uint64_t timeNs,
                               std::uint64_t rssMiB,
                               std::uint64_t hwmMiB,
                               br::ThermalState thermal,
                               bool foreground,
                               av::Phase phase) {
    av::MemorySample sample{};
    sample.monotonicTimeNs = timeNs;
    sample.rssKnown = true;
    sample.rssBytes = rssMiB * MiB;
    sample.highWaterKnown = true;
    sample.highWaterBytes = hwmMiB * MiB;
    sample.thermalKnown = true;
    sample.thermalState = thermal;
    sample.foreground = foreground;
    sample.phase = phase;
    return sample;
}

} // namespace

int main() {
    const tb::State backplane = make_backplane();
    expect(tb::validate(backplane) == tb::Status::Ok, "backplane valid");

    br::DeviceEnvelope lowDevice{};
    lowDevice.appMemoryClassMiB = 256;
    lowDevice.cpuThreadBudget = 4;
    lowDevice.lowRamDevice = true;
    lowDevice.foreground = true;
    lowDevice.thermalState = br::ThermalState::Nominal;
    br::ResourcePolicy lowPolicy{};
    expect(br::derive_resource_policy(lowDevice, lowPolicy) == br::Status::Ok, "low policy derives");
    expect(lowPolicy.valid, "low policy valid");
    expect(lowPolicy.totalWorkingSetBudgetBytes == 32ULL * MiB, "low budget 32 MiB");
    expect(lowPolicy.maxConcurrentHeavyRooms == 1U, "low concurrency one");
    expect(lowPolicy.tileSize == 128U, "low tile 128");

    br::DeviceEnvelope highDevice{};
    highDevice.appMemoryClassMiB = 2048;
    highDevice.cpuThreadBudget = 12;
    highDevice.vulkanAvailable = true;
    highDevice.foreground = true;
    highDevice.thermalState = br::ThermalState::Nominal;
    br::ResourcePolicy highPolicy{};
    expect(br::derive_resource_policy(highDevice, highPolicy) == br::Status::Ok, "high policy derives");
    expect(highPolicy.totalWorkingSetBudgetBytes == 256ULL * MiB, "high budget 256 MiB");
    expect(highPolicy.maxConcurrentHeavyRooms == 4U, "high concurrency four");
    expect(highPolicy.tileSize == 512U, "high tile 512");

    const auto lowPlan = make_plan(lowPolicy, backplane, 10ULL * MiB);
    av::SessionContract lowContract{};
    expect(av::bind_session_contract(lowPlan, backplane, lowContract) == av::Status::Ok, "bind low contract");
    expect(lowContract.valid, "low contract valid");

    av::ProcStatusMemory parsed{};
    expect(av::parse_proc_status("Name:\ttruthraw\nVmHWM:\t4096 kB\nVmRSS:\t2048 kB\n", parsed) == av::Status::Ok,
           "parse proc status fixture");
    expect(parsed.rssBytes == 2048ULL * 1024ULL, "proc rss bytes");
    expect(parsed.highWaterBytes == 4096ULL * 1024ULL, "proc hwm bytes");
    expect(av::parse_proc_status("VmRSS: 2 kB\nVmRSS: 3 kB\n", parsed) == av::Status::InvalidSample,
           "duplicate proc rss rejected");
    av::ProcStatusMemory liveProc{};
    expect(av::read_proc_self_status(liveProc) == av::Status::Ok, "live proc self status readable");
    expect(liveProc.rssKnown && liveProc.rssBytes > 0U, "live proc rss nonzero");

    av::SessionRecorder recorder{};
    expect(recorder.begin(lowContract) == av::Status::Ok, "begin normal session");
    auto s0 = memory_sample(1000000000ULL, 100, 110, br::ThermalState::Nominal, true, av::Phase::Baseline);
    s0.nativeHeapKnown = true;
    s0.nativeHeapAllocatedBytes = 20ULL * MiB;
    s0.nativeHeapArenaBytes = 24ULL * MiB;
    expect(recorder.record_memory_sample(s0) == av::Status::Ok, "normal baseline sample");

    auto s1 = memory_sample(2000000000ULL, 106, 112, br::ThermalState::Moderate, true, av::Phase::Pass1);
    s1.nativeHeapKnown = true;
    s1.nativeHeapAllocatedBytes = 22ULL * MiB;
    s1.nativeHeapArenaBytes = 28ULL * MiB;
    expect(recorder.record_memory_sample(s1) == av::Status::Ok, "normal pass1 sample");
    expect(recorder.record_tile_timing({av::Phase::Pass1, 5000000ULL, 1000000ULL}) == av::Status::Ok,
           "pass1 timing");
    expect(recorder.record_tile_timing({av::Phase::Pass2, 8000000ULL, 1000000ULL}) == av::Status::Ok,
           "pass2 timing");
    expect(recorder.record_tile_timing({av::Phase::SinkWrite, 1000000ULL, 1000000ULL}) == av::Status::Ok,
           "sink timing");
    expect(recorder.record_memory_sample(
               memory_sample(3000000000ULL, 108, 114, br::ThermalState::Moderate, true, av::Phase::Finish)) == av::Status::Ok,
           "normal finish sample");

    av::ValidationSummary summary{};
    expect(recorder.finalize(backplane, summary) == av::Status::Ok, "normal session finalizes");
    expect(summary.valid, "summary valid");
    expect(summary.sampleCount == 3U, "three memory samples");
    expect(summary.timingRecordCount == 3U, "three timing records");
    expect(summary.baselineRssBytes == 100ULL * MiB, "baseline rss");
    expect(summary.peakRssBytes == 108ULL * MiB, "peak rss");
    expect(summary.peakRssDeltaBytes == 8ULL * MiB, "rss delta eight MiB");
    expect(summary.maxAllocatorSlackBytes == 6ULL * MiB, "allocator slack proxy");
    expect(summary.thermalTransitions == 1U, "one thermal transition");
    expect(summary.maxThermalState == br::ThermalState::Moderate, "max thermal moderate");
    expect(summary.totalTimedPixels == 3000000ULL, "timed pixels");
    expect(summary.totalTileTimeNs == 14000000ULL, "tile time sum");
    expect(summary.maxTileLatencyNs == 8000000ULL, "max tile latency");
    expect(summary.p50LatencyUpperBoundNs == 8000000ULL, "p50 histogram upper bound");
    expect(summary.p95LatencyUpperBoundNs == 8000000ULL, "p95 histogram upper bound");
    expect(summary.timedPixelsPerSecond > 200000000.0 && summary.timedPixelsPerSecond < 220000000.0,
           "timed throughput range");
    expect(summary.scientificIdentityPreserved, "scientific identity preserved");
    expect(!summary.sceneIsoAxisPresent, "scene ISO axis absent");

    av::SessionRecorder nonMonotonic{};
    expect(nonMonotonic.begin(lowContract) == av::Status::Ok, "begin timestamp negative test");
    expect(nonMonotonic.record_memory_sample(s0) == av::Status::Ok, "timestamp first sample");
    expect(nonMonotonic.record_memory_sample(s0) == av::Status::NonMonotonicTimestamp,
           "non-monotonic timestamp rejected");

    av::SessionRecorder overBudget{};
    expect(overBudget.begin(lowContract) == av::Status::Ok, "begin memory budget negative test");
    expect(overBudget.record_memory_sample(
               memory_sample(1, 100, 100, br::ThermalState::Nominal, true, av::Phase::Baseline)) == av::Status::Ok,
           "budget baseline");
    expect(overBudget.record_memory_sample(
               memory_sample(2, 140, 140, br::ThermalState::Nominal, true, av::Phase::Pass1)) == av::Status::MemoryBudgetExceeded,
           "rss delta budget exceeded");

    const auto highPlan = make_plan(highPolicy, backplane, 22ULL * MiB);
    av::SessionContract highContract{};
    expect(av::bind_session_contract(highPlan, backplane, highContract) == av::Status::Ok, "bind high contract");
    av::SessionRecorder staleThermal{};
    expect(staleThermal.begin(highContract) == av::Status::Ok, "begin stale thermal policy test");
    expect(staleThermal.record_memory_sample(
               memory_sample(1, 100, 100, br::ThermalState::Severe, true, av::Phase::Baseline)) == av::Status::RuntimeReplanRequired,
           "severe thermal requires high-policy replan");

    av::SessionRecorder staleBackground{};
    expect(staleBackground.begin(highContract) == av::Status::Ok, "begin background policy test");
    expect(staleBackground.record_memory_sample(
               memory_sample(1, 100, 100, br::ThermalState::Nominal, false, av::Phase::Baseline)) == av::Status::RuntimeReplanRequired,
           "background requires high-policy replan");

    auto severeDevice = highDevice;
    severeDevice.thermalState = br::ThermalState::Severe;
    br::ResourcePolicy severePolicy{};
    expect(br::derive_resource_policy(severeDevice, severePolicy) == br::Status::Ok, "severe policy derives");
    expect(severePolicy.maxConcurrentHeavyRooms == 1U, "severe concurrency one");
    expect(severePolicy.tileSize == 128U, "severe tile 128");
    expect(severePolicy.preferredBackend == br::ComputeBackend::CpuBaseline, "severe CPU baseline");
    const auto severePlan = make_plan(severePolicy, backplane, 10ULL * MiB);
    av::SessionContract severeContract{};
    expect(av::bind_session_contract(severePlan, backplane, severeContract) == av::Status::Ok, "bind severe contract");
    av::SessionRecorder severeRecorder{};
    expect(severeRecorder.begin(severeContract) == av::Status::Ok, "begin severe safe session");
    expect(severeRecorder.record_memory_sample(
               memory_sample(1, 100, 100, br::ThermalState::Severe, true, av::Phase::Baseline)) == av::Status::Ok,
           "governor severe policy remains admissible");

    av::SessionRecorder identityRecorder{};
    expect(identityRecorder.begin(lowContract) == av::Status::Ok, "begin identity negative test");
    expect(identityRecorder.record_memory_sample(s0) == av::Status::Ok, "identity baseline");
    expect(identityRecorder.record_tile_timing({av::Phase::Pass1, 1000000ULL, 1000ULL}) == av::Status::Ok,
           "identity timing");
    auto changedBackplane = backplane;
    changedBackplane.zeroLineHash[0] ^= 0x01U;
    av::ValidationSummary changedSummary{};
    expect(identityRecorder.finalize(changedBackplane, changedSummary) == av::Status::ScientificIdentityChanged,
           "zero-line identity change rejected");
    expect(!changedSummary.scientificIdentityPreserved, "changed identity marked false");

    auto isoPlan = lowPlan;
    isoPlan.sceneIsoAxisPresent = true;
    av::SessionContract rejectedContract{};
    expect(av::bind_session_contract(isoPlan, backplane, rejectedContract) == av::Status::InvalidPlan,
           "scene ISO axis rejected");

    if (failures != 0) {
        std::cerr << "ANDROID_ON_DEVICE_VALIDATION_V0_1_HOST_FAIL failures=" << failures << '\n';
        return 1;
    }

    std::cout << "ANDROID_ON_DEVICE_VALIDATION_V0_1_HOST_PASS\n";
    std::cout << "low_budget_bytes=" << lowPolicy.totalWorkingSetBudgetBytes << '\n';
    std::cout << "low_concurrency=" << lowPolicy.maxConcurrentHeavyRooms << '\n';
    std::cout << "low_tile=" << lowPolicy.tileSize << '\n';
    std::cout << "high_budget_bytes=" << highPolicy.totalWorkingSetBudgetBytes << '\n';
    std::cout << "high_concurrency=" << highPolicy.maxConcurrentHeavyRooms << '\n';
    std::cout << "high_tile=" << highPolicy.tileSize << '\n';
    std::cout << "normal_peak_rss_delta_bytes=" << summary.peakRssDeltaBytes << '\n';
    std::cout << "normal_allocator_slack_bytes=" << summary.maxAllocatorSlackBytes << '\n';
    std::cout << "normal_timing_records=" << summary.timingRecordCount << '\n';
    std::cout << "normal_thermal_transitions=" << summary.thermalTransitions << '\n';
    std::cout << "memory_budget_gate=PASS\n";
    std::cout << "thermal_replan_gate=PASS\n";
    std::cout << "background_replan_gate=PASS\n";
    std::cout << "scientific_identity_gate=PASS\n";
    std::cout << "physical_frame_count=" << backplane.physicalFrameCount << '\n';
    std::cout << "independent_evidence_count=" << backplane.independentEvidenceCount << '\n';
    std::cout << "scene_iso_axis=ABSENT\n";
    std::cout << "android_device_evidence=PENDING\n";
    return 0;
}
