#pragma once

#include "scientific_master_streaming_binding_v0_1.h"
#include "technical_backplane_phase2_v0_1.h"

#include <array>
#include <cstddef>
#include <cstdint>
#include <string>

namespace truthraw::finalized_scientific_preview_route::v0_1 {

enum class StatusCode : std::uint8_t {
    Ok = 0,
    InvalidPreparedSource,
    SourceLineageMismatch,
    ScientificMasterBindingFailed,
    Phase2FinalizationFailed,
    FinalAdmissionMismatch,
};

struct Status final {
    StatusCode code = StatusCode::Ok;
    std::string message;
    explicit operator bool() const noexcept { return code == StatusCode::Ok; }
    static Status ok() { return {}; }
    static Status error(StatusCode code, std::string message) {
        Status out;
        out.code = code;
        out.message = std::move(message);
        return out;
    }
};

struct Options final {
    scientific_master_streaming_binding::v0_1::Options scientificBinding{};
    std::array<technical_backplane::v0_1::RoomStatus,
               technical_backplane::v0_1::kRoomCount> roomStatus{};
    technical_backplane::v0_1::ClaimStatus claimStatus =
        technical_backplane::v0_1::ClaimStatus::Candidate;
};

struct Result final {
    scientific_master_streaming_binding::v0_1::Result scientific{};
    technical_backplane_phase2::v0_1::Phase2Result phase2{};

    // True only after the complete source -> master -> zero-line/scene-scale ->
    // Backplane -> Scientific Preview admission chain succeeded.
    bool scientificPreviewReleaseAllowed = false;
    bool scientificClaimAllowed = false;
};

// Closes the pre-master/post-master authority gap in one fail-closed call.
// The caller cannot supply a Scientific Master hash, zero-line hash or
// scene-scale hash. They are derived from the actual IRawTileSource and the
// reconstruction backend before Technical Backplane phase 2 is finalized.
Status finalize_from_streaming_source(
    const scientific_preview_binding_v0_2::PreparedScientificPreviewSource& prepared,
    streaming_v0_1::IRawTileSource& source,
    IReconstructionBackend& reconstruction,
    const Options& options,
    Result& out) noexcept;

const char* status_name(StatusCode code) noexcept;

}  // namespace truthraw::finalized_scientific_preview_route::v0_1
