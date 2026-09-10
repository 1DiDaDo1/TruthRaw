#include "building_runtime_v0_1.h"

#include <algorithm>
#include <cmath>
#include <limits>

namespace truthraw::building_runtime::v0_1 {
namespace {
constexpr std::size_t room_count() noexcept { return static_cast<std::size_t>(RoomId::Count); }
constexpr std::size_t idx(RoomId id) noexcept { return static_cast<std::size_t>(id); }
bool valid_room(RoomId id) noexcept { return idx(id) < room_count(); }

bool critical_state_valid(const ScientificState& s, const CaptureProvenance& p) noexcept {
    if (s.physicalFrameCount != 1 || s.independentEvidenceCount != 1) return false;
    if (p.physicalFrameCount != 1 || p.independentEvidenceCount != 1) return false;
    if (s.physicalFrameCount != p.physicalFrameCount || s.independentEvidenceCount != p.independentEvidenceCount) return false;
    if (!std::isfinite(s.physicalCaptureEv)) return false;
    if (s.bestConditioningEvKnown && !std::isfinite(s.bestConditioningEv)) return false;
    if (s.sigmaKnown && (!std::isfinite(s.sigma) || !(s.sigma > 0.0))) return false;
    return true;
}

bool dependency_executes(const RuntimeResult& out, RoomId dep) noexcept {
    const std::size_t d = idx(dep);
    return d < out.decisionCount && out.decisions[d].execute;
}

Status append_event(RuntimeResult& out, const Decision& d) noexcept {
    if (out.ledgerCount >= kMaxEvents) return Status::CapacityExceeded;
    out.ledger[out.ledgerCount++] = DecisionEvent{d.room, d.status, d.reason};
    return Status::Ok;
}

std::uint64_t safe_mul_mib(std::uint64_t mib) noexcept {
    if (mib > std::numeric_limits<std::uint64_t>::max() / kMiB) return 0;
    return mib * kMiB;
}

bool corridor_floor_allows(TruthFloor from, TruthFloor to) noexcept {
    return static_cast<std::uint8_t>(to) >= static_cast<std::uint8_t>(from);
}
} // namespace

std::array<RoomDescriptor, room_count()> default_room_graph() noexcept {
    std::array<RoomDescriptor, room_count()> g{};
    g[0] = {RoomId::Archivist, Domain::Scientific, TruthFloor::Foundation, RoomStatus::Available,
            {}, 0, false, false, false, false, false, false, false, false};
    g[1] = {RoomId::MeasurementLab, Domain::Scientific, TruthFloor::Measurement, RoomStatus::Available,
            {RoomId::Archivist}, 1, false, false, false, false, false, false, false, false};
    g[2] = {RoomId::Architect, Domain::Scientific, TruthFloor::Reconstruction, RoomStatus::Available,
            {RoomId::MeasurementLab}, 1, true, false, false, false, false, false, false, false};
    g[3] = {RoomId::Restorer, Domain::Scientific, TruthFloor::Reconstruction, RoomStatus::ResearchOnly,
            {RoomId::Architect}, 1, true, true, false, false, false, false, false, false};
    g[4] = {RoomId::SceneRegistry, Domain::Scientific, TruthFloor::Scene, RoomStatus::Available,
            {RoomId::Architect}, 1, false, false, false, false, false, false, false, false};
    g[5] = {RoomId::Surveyor, Domain::Scientific, TruthFloor::Scene, RoomStatus::ResearchOnly,
            {RoomId::SceneRegistry}, 1, true, true, false, false, false, false, false, false};
    g[6] = {RoomId::ManifoldConditioning, Domain::Scientific, TruthFloor::Scene, RoomStatus::Available,
            {RoomId::Surveyor}, 1, true, true, false, false, false, false, false, false};
    g[7] = {RoomId::LightingStudioCicm, Domain::Counterfactual, TruthFloor::Counterfactual, RoomStatus::ResearchOnly,
            {RoomId::SceneRegistry, RoomId::Surveyor}, 2, true, true, false, false, false, true, false, false};
    g[8] = {RoomId::RoomCapsule, Domain::Counterfactual, TruthFloor::Counterfactual, RoomStatus::ResearchOnly,
            {RoomId::LightingStudioCicm}, 1, true, false, false, false, true, true, false, false};
    g[9] = {RoomId::Colorist, Domain::Appearance, TruthFloor::Appearance, RoomStatus::Available,
            {RoomId::SceneRegistry, RoomId::Surveyor}, 2, true, false, false, true, true, false, false, false};
    g[10] = {RoomId::Finisher, Domain::Appearance, TruthFloor::Appearance, RoomStatus::Available,
             {RoomId::Colorist}, 1, true, false, true, true, true, false, false, false};
    g[11] = {RoomId::Exporter, Domain::Projection, TruthFloor::Projection, RoomStatus::Available,
             {RoomId::Finisher}, 1, true, false, true, false, true, false, false, false};
    return g;
}

Status validate_graph(const std::array<RoomDescriptor, room_count()>& graph) noexcept {
    std::array<std::uint8_t, room_count()> color{};
    for (std::size_t i = 0; i < room_count(); ++i) {
        const auto& r = graph[i];
        if (idx(r.id) != i) return Status::InvalidGraph;
        if (r.dependencyCount > kMaxDependencies) return Status::InvalidGraph;
        if (r.mayMutateInputScientificMaster || r.mayIncreaseIndependentEvidence) return Status::InvalidGraph;
        if (r.domain == Domain::Scientific && (r.mayReadAppearance || r.mayWriteAppearance || r.mayReadCounterfactual || r.mayWriteCounterfactual)) {
            return Status::InvalidGraph;
        }
        if (r.domain == Domain::Counterfactual && r.floor != TruthFloor::Counterfactual) return Status::InvalidGraph;
        if (r.domain == Domain::Appearance && r.floor != TruthFloor::Appearance) return Status::InvalidGraph;
        if (r.domain == Domain::Projection && r.floor != TruthFloor::Projection) return Status::InvalidGraph;
        for (std::size_t d = 0; d < r.dependencyCount; ++d) {
            if (!valid_room(r.dependencies[d])) return Status::InvalidGraph;
            const auto& dep = graph[idx(r.dependencies[d])];
            if (!corridor_floor_allows(dep.floor, r.floor)) return Status::InvalidGraph;
        }
    }

    for (std::size_t root = 0; root < room_count(); ++root) {
        if (color[root] != 0) continue;
        struct Frame { std::size_t node; std::size_t nextDep; };
        std::array<Frame, room_count()> stack{};
        std::size_t sp = 0;
        stack[sp++] = {root, 0};
        while (sp > 0) {
            Frame& f = stack[sp - 1];
            if (color[f.node] == 0) color[f.node] = 1;
            const auto& desc = graph[f.node];
            if (f.nextDep >= desc.dependencyCount) {
                color[f.node] = 2;
                --sp;
                continue;
            }
            const std::size_t dep = idx(desc.dependencies[f.nextDep++]);
            if (color[dep] == 1) return Status::InvalidGraph;
            if (color[dep] == 0) {
                if (sp >= stack.size()) return Status::InvalidGraph;
                stack[sp++] = {dep, 0};
            }
        }
    }
    return Status::Ok;
}

Status evaluate(const ScientificState& scientific,
                const AppearanceState& appearance,
                const CaptureProvenance& provenance,
                const ActivationRequest& request,
                const std::array<RoomDescriptor, room_count()>& graph,
                RuntimeResult& out) noexcept {
    out = {};
    out.status = Status::InvalidInput;

    const Status graphStatus = validate_graph(graph);
    if (graphStatus != Status::Ok) { out.status = graphStatus; return graphStatus; }
    if (scientific.scientificMasterModified) {
        out.status = Status::ScientificMasterAlreadyModified;
        return out.status;
    }
    if (!critical_state_valid(scientific, provenance)) {
        out.status = Status::EvidenceInvariantViolation;
        return out.status;
    }

    out.physicalFrameCount = scientific.physicalFrameCount;
    out.independentEvidenceCount = scientific.independentEvidenceCount;
    out.scientificMasterModified = false;
    out.physicalCaptureEv = scientific.physicalCaptureEv;
    out.bestConditioningEvKnown = scientific.bestConditioningEvKnown;
    out.bestConditioningEv = scientific.bestConditioningEv;
    out.appearanceEvKnown = appearance.appearanceEvKnown;
    out.appearanceEv = appearance.appearanceEv;
    out.claimStatus = scientific.claimStatus;
    out.provenanceFingerprint = provenance.immutableEvidenceFingerprint;
    out.decisionCount = room_count();

    for (std::size_t i = 0; i < room_count(); ++i) {
        const RoomDescriptor& desc = graph[i];
        Decision d{};
        d.room = desc.id;
        d.status = desc.declaredStatus;

        if (!request.requested[i]) {
            d.reason = Reason::NotRequested;
        } else if (desc.declaredStatus == RoomStatus::Rejected) {
            d.reason = Reason::ExplicitlyRejected;
        } else if (desc.declaredStatus == RoomStatus::BlockedMissingEvidence) {
            d.reason = Reason::CriticalScientificStateInvalid;
        } else if (desc.declaredStatus == RoomStatus::IdentityFallback) {
            d.reason = Reason::IdentityByContract;
        } else if (desc.requiresKnownSigma && !scientific.sigmaKnown) {
            d.status = RoomStatus::BlockedMissingEvidence;
            d.reason = Reason::RequiredSigmaUnknown;
        } else {
            bool depsOk = true;
            for (std::size_t j = 0; j < desc.dependencyCount; ++j) {
                if (!dependency_executes(out, desc.dependencies[j])) { depsOk = false; break; }
            }
            if (!depsOk) {
                d.reason = Reason::DependencyNotSatisfied;
            } else {
                d.reason = (desc.declaredStatus == RoomStatus::ResearchOnly)
                    ? Reason::ResearchOnlyByContract : Reason::RequestedAndAdmissible;
                d.execute = true;
            }
        }

        out.decisions[i] = d;
        const Status ev = append_event(out, d);
        if (ev != Status::Ok) { out.status = ev; return ev; }
    }

    if (out.physicalFrameCount != 1 || out.independentEvidenceCount != 1 || out.scientificMasterModified) {
        out.status = Status::EvidenceInvariantViolation;
        return out.status;
    }
    if (!appearance.appearanceEvKnown) {
        out.appearanceEvKnown = false;
        out.appearanceEv = 0.0;
    }
    out.status = Status::Ok;
    return out.status;
}

Status validate_corridor_transition(const RoomDescriptor& from,
                                    const RoomDescriptor& to,
                                    const CorridorToken& token) noexcept {
    if (!valid_room(from.id) || !valid_room(to.id)) return Status::InvalidCorridor;
    if (token.artifactHandle == 0 || token.provenanceFingerprint == 0) return Status::InvalidCorridor;
    if (token.physicalFrameCount != 1 || token.independentEvidenceCount != 1 || token.scientificMasterModified) {
        return Status::EvidenceInvariantViolation;
    }
    if (token.floor != from.floor) return Status::InvalidCorridor;
    if (!corridor_floor_allows(from.floor, to.floor)) return Status::InvalidCorridor;
    if (to.domain == Domain::Scientific && from.domain != Domain::Scientific) return Status::InvalidCorridor;
    if (to.domain == Domain::Scientific && (to.mayReadAppearance || to.mayReadCounterfactual)) return Status::InvalidCorridor;
    return Status::Ok;
}

Status derive_resource_policy(const DeviceEnvelope& device, ResourcePolicy& out) noexcept {
    out = {};
    out.correctnessBaseline = ComputeBackend::CpuBaseline;
    out.preferredBackend = ComputeBackend::CpuBaseline;

    if (device.cpuThreadBudget == 0 || (device.appMemoryClassMiB == 0 && device.explicitMaxWorkingSetBytes == 0)) {
        return Status::InvalidInput;
    }
    const std::uint64_t classBytes = safe_mul_mib(device.appMemoryClassMiB);
    if (device.appMemoryClassMiB != 0 && classBytes == 0) return Status::InvalidInput;

    std::uint64_t budget = device.explicitMaxWorkingSetBytes;
    if (classBytes != 0) {
        const std::uint64_t defaultBudget = classBytes / 8U;
        budget = (budget == 0) ? defaultBudget : std::min(budget, defaultBudget);
    }
    if (device.currentlyAvailableBytes != 0) budget = std::min(budget, device.currentlyAvailableBytes / 2U);
    if (budget < 4U * kMiB) return Status::BudgetTooSmall;

    const bool low = device.lowRamDevice || device.appMemoryClassMiB <= 256 || device.cpuThreadBudget <= 4;
    const bool high = !low && device.appMemoryClassMiB >= 1536 && device.cpuThreadBudget >= 8;

    out.tier = low ? ResourceTier::Low : (high ? ResourceTier::High : ResourceTier::Mid);
    out.maxConcurrentHeavyRooms = low ? 1U : (high ? 4U : 2U);
    out.tileSize = low ? 128U : (high ? 512U : 256U);
    out.speculativePrefetch = !low && device.foreground;
    out.retainRebuildableCaches = high && device.foreground;

    if (!device.foreground) {
        out.maxConcurrentHeavyRooms = 1;
        out.speculativePrefetch = false;
        out.retainRebuildableCaches = false;
    }
    if (device.thermalState == ThermalState::Severe || device.thermalState == ThermalState::Critical) {
        out.maxConcurrentHeavyRooms = 1;
        out.tileSize = 128;
        out.speculativePrefetch = false;
        out.retainRebuildableCaches = false;
    } else if (device.thermalState == ThermalState::Moderate && out.maxConcurrentHeavyRooms > 2) {
        out.maxConcurrentHeavyRooms = 2;
    }

    out.totalWorkingSetBudgetBytes = budget;
    out.perHeavyRoomBudgetBytes = budget / out.maxConcurrentHeavyRooms;
    out.cpuThreadsPerHeavyRoom = std::max(1U, device.cpuThreadBudget / out.maxConcurrentHeavyRooms);
    if (device.vulkanAvailable && device.foreground &&
        device.thermalState != ThermalState::Severe && device.thermalState != ThermalState::Critical) {
        out.preferredBackend = ComputeBackend::OptionalVulkan;
    }
    out.valid = true;
    return Status::Ok;
}

Status plan_execution(const RuntimeResult& runtime,
                      const std::array<RoomDescriptor, room_count()>& graph,
                      const ResourcePolicy& resources,
                      ExecutionPlan& out) noexcept {
    out = {};
    if (runtime.status != Status::Ok || !resources.valid) return Status::InvalidInput;
    const Status graphStatus = validate_graph(graph);
    if (graphStatus != Status::Ok) return graphStatus;

    out.resources = resources;
    std::array<int, room_count()> waveFor{};
    waveFor.fill(-1);
    std::array<std::uint8_t, room_count() + 1> heavyInWave{};
    std::array<std::uint8_t, room_count() + 1> lanesInWave{};

    std::size_t assigned = 0;
    for (std::size_t pass = 0; pass < room_count() && assigned < room_count(); ++pass) {
        bool progress = false;
        for (std::size_t i = 0; i < room_count(); ++i) {
            if (waveFor[i] >= 0) continue;
            if (!runtime.decisions[i].execute) {
                waveFor[i] = 0;
                ++assigned;
                progress = true;
                continue;
            }
            const auto& room = graph[i];
            int earliest = 0;
            bool ready = true;
            for (std::size_t d = 0; d < room.dependencyCount; ++d) {
                const std::size_t dep = idx(room.dependencies[d]);
                if (runtime.decisions[dep].execute && waveFor[dep] < 0) { ready = false; break; }
                if (runtime.decisions[dep].execute) earliest = std::max(earliest, waveFor[dep] + 1);
            }
            if (!ready) continue;

            int wave = earliest;
            if (room.heavy) {
                while (wave <= static_cast<int>(room_count()) &&
                       heavyInWave[static_cast<std::size_t>(wave)] >= resources.maxConcurrentHeavyRooms) {
                    ++wave;
                }
            }
            if (wave > static_cast<int>(room_count())) return Status::CapacityExceeded;
            waveFor[i] = wave;
            if (room.heavy) ++heavyInWave[static_cast<std::size_t>(wave)];
            ++assigned;
            progress = true;
        }
        if (!progress) return Status::InvalidGraph;
    }

    for (std::size_t i = 0; i < room_count(); ++i) {
        if (!runtime.decisions[i].execute) continue;
        if (out.placementCount >= out.placements.size()) return Status::CapacityExceeded;
        const std::size_t w = static_cast<std::size_t>(waveFor[i]);
        ExecutionPlacement p{};
        p.room = graph[i].id;
        p.wave = static_cast<std::uint8_t>(w);
        p.lane = lanesInWave[w]++;
        p.heavyLease = graph[i].heavy;
        p.backend = (graph[i].heavy ? resources.preferredBackend : ComputeBackend::CpuBaseline);
        p.memoryLeaseBytes = graph[i].heavy ? resources.perHeavyRoomBudgetBytes : 0;
        out.placements[out.placementCount++] = p;
        out.waveCount = std::max(out.waveCount, static_cast<std::uint8_t>(p.wave + 1));
    }
    out.status = Status::Ok;
    return out.status;
}

const char* status_name(Status s) noexcept {
    switch (s) {
        case Status::Ok: return "OK";
        case Status::InvalidInput: return "INVALID_INPUT";
        case Status::InvalidGraph: return "INVALID_GRAPH";
        case Status::DependencyBlocked: return "DEPENDENCY_BLOCKED";
        case Status::MissingScientificState: return "MISSING_SCIENTIFIC_STATE";
        case Status::EvidenceInvariantViolation: return "EVIDENCE_INVARIANT_VIOLATION";
        case Status::ScientificMasterAlreadyModified: return "SCIENTIFIC_MASTER_ALREADY_MODIFIED";
        case Status::CapacityExceeded: return "CAPACITY_EXCEEDED";
        case Status::InvalidCorridor: return "INVALID_CORRIDOR";
        case Status::BudgetTooSmall: return "BUDGET_TOO_SMALL";
    }
    return "UNKNOWN";
}
const char* room_status_name(RoomStatus s) noexcept {
    switch (s) {
        case RoomStatus::Available: return "AVAILABLE";
        case RoomStatus::ResearchOnly: return "RESEARCH_ONLY";
        case RoomStatus::BlockedMissingEvidence: return "BLOCKED_MISSING_EVIDENCE";
        case RoomStatus::IdentityFallback: return "IDENTITY_FALLBACK";
        case RoomStatus::Rejected: return "REJECTED";
    }
    return "UNKNOWN";
}
const char* reason_name(Reason r) noexcept {
    switch (r) {
        case Reason::None: return "NONE";
        case Reason::NotRequested: return "NOT_REQUESTED";
        case Reason::DependencyNotSatisfied: return "DEPENDENCY_NOT_SATISFIED";
        case Reason::RequiredSigmaUnknown: return "REQUIRED_SIGMA_UNKNOWN";
        case Reason::CriticalScientificStateInvalid: return "CRITICAL_SCIENTIFIC_STATE_INVALID";
        case Reason::ResearchOnlyByContract: return "RESEARCH_ONLY_BY_CONTRACT";
        case Reason::ExplicitlyRejected: return "EXPLICITLY_REJECTED";
        case Reason::IdentityByContract: return "IDENTITY_BY_CONTRACT";
        case Reason::RequestedAndAdmissible: return "REQUESTED_AND_ADMISSIBLE";
    }
    return "UNKNOWN";
}
const char* resource_tier_name(ResourceTier t) noexcept {
    switch (t) {
        case ResourceTier::Low: return "LOW";
        case ResourceTier::Mid: return "MID";
        case ResourceTier::High: return "HIGH";
    }
    return "UNKNOWN";
}

} // namespace truthraw::building_runtime::v0_1
