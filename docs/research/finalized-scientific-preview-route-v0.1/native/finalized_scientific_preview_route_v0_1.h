#pragma once

#include "scientific_master_streaming_binding_v0_1.h"
#include "technical_backplane_phase2_v0_1.h"
#include "tile_native_dng_source_v0_1.h"

#include <array>
#include <cstddef>
#include <cstdint>
#include <memory>
#include <string>

namespace truthraw::finalized_scientific_preview_route::v0_1 {

enum class StatusCode : std::uint8_t {
    Ok = 0,
    InvalidPreparedSource,
    SourceReverificationFailed,
    TileSourceOpenFailed,
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
    tile_dng_v0_1::SourceAudit tileSourceAudit{};

    // True only after exact source-byte reverification and the complete
    // source -> TileNative -> master -> zero-line/scene-scale -> Backplane ->
    // Scientific Preview admission chain.
    bool scientificPreviewReleaseAllowed = false;
    // Stronger color-truth claim remains independently calibration-gated.
    bool scientificClaimAllowed = false;
};

// Direct-native finalized Scientific Preview route.
//
// The same random-access byte source is first reverified against the prepared
// SHA-256 seal and then used to open TileNativeDngSource with the exact prepared
// OpenOptions. Therefore a caller cannot substitute a second CFA tile source or
// supply master/zero-line/scene-scale hashes.
//
// Gatehouse/decoded-measurement sources require a separate certified-handoff
// overload; they are deliberately not accepted by this direct-native v0.1 API.
Status finalize_direct_native(
    const scientific_preview_binding_v0_2::PreparedScientificPreviewSource& prepared,
    std::shared_ptr<tile_dng_v0_1::IRandomAccessByteSource> sealedSourceBytes,
    IReconstructionBackend& reconstruction,
    const Options& options,
    Result& out) noexcept;

const char* status_name(StatusCode code) noexcept;

}  // namespace truthraw::finalized_scientific_preview_route::v0_1
