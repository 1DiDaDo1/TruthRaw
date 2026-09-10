#include "building_runtime_v0_1.h"

#include <cmath>

namespace truthraw::building_runtime::v0_1 {
namespace {
constexpr std::size_t room_count() noexcept {
    return static_cast<std::size_t>(RoomId::Count);
}
constexpr std::size_t idx(RoomId id) noexcept {
    return static_cast<std::size_t>(id);
}
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
    if (d >= out.decisionCount) return false;
    return out.decisions[d].execute;
}

Status append_event(RuntimeResult& out, const Decision& d) noexcept {
    if (out.ledgerCount >= kMaxEvents) return Status::CapacityExceeded;
    out.ledger[out.ledgerCount++] = DecisionEvent{d.room, d.status, d.reason};
    return Status::Ok;
}
} // namespace

std::array<RoomDescriptor, room_count()> default_room_graph() noexcept {
    std::array<RoomDescriptor, room_count()> g{};
    g[0] = {RoomId::ManifoldConditioning, Domain::Scientific, RoomStatus::Available,
            {}, 0, true, false, false, false, false};
    g[1] = {RoomId::CounterfactualIllumination, Domain::Scientific, RoomStatus::ResearchOnly,
            {RoomId::ManifoldConditioning}, 1, true, false, false, false, false};
    g[2] = {RoomId::RoomCapsule, Domain::Appearance, RoomStatus::ResearchOnly,
            {RoomId::CounterfactualIllumination}, 1, false, false, true, false, false};
    g[3] = {RoomId::Uncertainty, Domain::Scientific, RoomStatus::ResearchOnly,
            {RoomId::ManifoldConditioning}, 1, true, false, false, false, false};
    g[4] = {RoomId::Appearance, Domain::Appearance, RoomStatus::Available,
            {RoomId::Uncertainty}, 1, false, false, true, false, false};
    g[5] = {RoomId::OutputProjection, Domain::Projection, RoomStatus::Available,
            {RoomId::Appearance}, 1, false, true, false, false, false};
    return g;
}

Status validate_graph(const std::array<RoomDescriptor, room_count()>& graph) noexcept {
    std::array<std::uint8_t, room_count()> color{};
    for (std::size_t i = 0; i < room_count(); ++i) {
        if (idx(graph[i].id) != i) return Status::InvalidGraph;
        if (graph[i].dependencyCount > kMaxDependencies) return Status::InvalidGraph;
        if (graph[i].mayModifyScientificMaster || graph[i].mayIncreaseIndependentEvidence) return Status::InvalidGraph;
        if (graph[i].domain == Domain::Scientific && (graph[i].mayReadAppearance || graph[i].mayWriteAppearance)) return Status::InvalidGraph;
        for (std::size_t d = 0; d < graph[i].dependencyCount; ++d) {
            if (!valid_room(graph[i].dependencies[d])) return Status::InvalidGraph;
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
            d.execute = false;
        } else if (desc.declaredStatus == RoomStatus::Rejected) {
            d.reason = Reason::ExplicitlyRejected;
            d.execute = false;
        } else if (desc.declaredStatus == RoomStatus::BlockedMissingEvidence) {
            d.reason = Reason::CriticalScientificStateInvalid;
            d.execute = false;
        } else if (desc.declaredStatus == RoomStatus::IdentityFallback) {
            d.reason = Reason::IdentityByContract;
            d.execute = false;
        } else if (desc.requiresKnownSigma && !scientific.sigmaKnown) {
            d.status = RoomStatus::BlockedMissingEvidence;
            d.reason = Reason::RequiredSigmaUnknown;
            d.execute = false;
        } else {
            bool depsOk = true;
            for (std::size_t j = 0; j < desc.dependencyCount; ++j) {
                if (!dependency_executes(out, desc.dependencies[j])) { depsOk = false; break; }
            }
            if (!depsOk) {
                d.reason = Reason::DependencyNotSatisfied;
                d.execute = false;
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

    // Runtime is a corridor/scheduler only. These invariants are hard-coded at exit.
    if (out.physicalFrameCount != 1 || out.independentEvidenceCount != 1 || out.scientificMasterModified) {
        out.status = Status::EvidenceInvariantViolation;
        return out.status;
    }
    // Critically: appearance EV is copied only from appearance input. Never derive it from conditioning EV.
    if (!appearance.appearanceEvKnown) {
        out.appearanceEvKnown = false;
        out.appearanceEv = 0.0;
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

} // namespace truthraw::building_runtime::v0_1
