#pragma once

#include "scientific_preview_source_binding_v0_2.h"
#include "technical_backplane_v0_1.h"
#include "truthrange_latent_v0_2.h"

#include <array>
#include <cstdint>
#include <string>

namespace truthraw::technical_backplane_phase2::v0_1 {

using Hash256 = technical_backplane::v0_1::Hash256;

constexpr std::uint16_t kVersion = 1;

enum class StatusCode : std::uint8_t {
    Ok = 0,
    InvalidPreparedSource,
    InvalidScientificMasterDigest,
    InvalidZeroLine,
    InvalidSceneScale,
    InvalidRoomState,
    BackplaneRejected,
    BackplaneSerializationFailed,
    PreviewFinalizationRejected,
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

struct Phase2Input final {
    scientific_preview_binding_v0_2::PreparedScientificPreviewSource prepared{};
    Hash256 scientificMasterHash{};
    TruthRangeGaugeV02 zeroLineGauge{};
    LatentSceneBindingV02 sceneBinding{};
    std::array<technical_backplane::v0_1::RoomStatus,
               technical_backplane::v0_1::kRoomCount> roomStatus{};
    technical_backplane::v0_1::ClaimStatus claimStatus =
        technical_backplane::v0_1::ClaimStatus::Candidate;
};

struct Phase2Result final {
    Hash256 zeroLineHash{};
    Hash256 sceneScaleHash{};
    technical_backplane::v0_1::State backplane{};
    technical_backplane::v0_1::SerializedBackplane serializedBackplane{};
    scientific_preview_binding_v0_1::ScientificPreviewAdmission admission{};
};

// Canonically hashes the existing TruthRange zero-line/gauge state. This does
// not choose a new gauge and does not alter L0.
Status hash_zero_line_identity(const TruthRangeGaugeV02& gauge, Hash256& out) noexcept;

// Canonically hashes the existing scene-scale identity and its normalization
// flags. ISO is intentionally absent: capture ISO remains provenance and is not
// a Scientific Scene/TruthRange coordinate.
Status hash_scene_scale_identity(const LatentSceneBindingV02& binding, Hash256& out) noexcept;

// Completes Technical Backplane phase 2 from a prepared source, a real
// Scientific Master digest, and real zero-line/scene-scale identities, then
// invokes the already-existing Scientific Preview v0.2 finalization gate.
// No placeholder/dummy hash path exists in this function.
Status finalize_phase2(const Phase2Input& input, Phase2Result& out) noexcept;

const char* status_name(StatusCode code) noexcept;

}  // namespace truthraw::technical_backplane_phase2::v0_1
