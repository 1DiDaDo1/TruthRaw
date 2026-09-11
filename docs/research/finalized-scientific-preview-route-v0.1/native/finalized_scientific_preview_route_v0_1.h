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
    SourceReverificationFailed,
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
    std::size_t sourceReverifyChunkBytes =
        scientific_preview_binding_v0_1::kDefaultHashChunkBytes;
    std::array<technical_backplane::v0_1::RoomStatus,
               technical_backplane::v0_1::kRoomCount> roomStatus{};
    technical_backplane::v0_1::ClaimStatus claimStatus =
        technical_backplane::v0_1::ClaimStatus::Candidate;
};

struct Result final {
    scientific_master_streaming_binding::v0_1::Result scientific{};
    technical_backplane_phase2::v0_1::Phase2Result phase2{};

    // True only after exact source-byte reverification and the complete
    // source -> master -> zero-line/scene-scale -> Backplane -> admission chain.
    bool scientificPreviewReleaseAllowed = false;
    // Stronger color-truth claim remains independently calibration-gated.
    bool scientificClaimAllowed = false;
};

// Closes the pre-master/post-master authority gap in one fail-closed call.
// The original random-access source bytes are reverified against the prepared
// SHA-256 seal before the tile source may produce a Scientific Master. The
// caller cannot supply master/zero-line/scene-scale hashes.
Status finalize_from_streaming_source(
    const scientific_preview_binding_v0_2::PreparedScientificPreviewSource& prepared,
    tile_dng_v0_1::IRandomAccessByteSource& sealedSourceBytes,
    streaming_v0_1::IRawTileSource& source,
    IReconstructionBackend& reconstruction,
    const Options& options,
    Result& out) noexcept;

const char* status_name(StatusCode code) noexcept;

}  // namespace truthraw::finalized_scientific_preview_route::v0_1
