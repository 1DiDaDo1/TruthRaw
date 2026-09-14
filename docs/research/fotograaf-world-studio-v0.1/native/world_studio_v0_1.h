#pragma once

#include <array>
#include <cstdint>

namespace truthraw::fotograaf::world_studio_v0_1 {

enum class WorldAuthority : std::uint8_t {
    WS0_RELATIVE_APPEARANCE = 0,
    WS1_GEOMETRY_AWARE_RELATIVE = 1,
    WS2_CALIBRATED_NEUTRAL_PHOTOMETRIC = 2,
    WS3_SPECTRAL_MATERIAL_PHYSICAL_RESERVED = 3,
};

enum class EvidenceClass : std::uint8_t {
    MEASURED = 0,
    RECONSTRUCTED = 1,
    COUNTERFACTUAL = 2,
    APPEARANCE_ONLY = 3,
    UNKNOWN = 4,
};

enum class EmitterKind : std::uint8_t {
    DIRECTIONAL = 0,
    POINT = 1,
    AREA = 2,
    ENVIRONMENT = 3,
    EMISSIVE_SURFACE = 4,
};

// Descriptor only. No image-sized ownership belongs here.
struct SceneDomain {
    std::uint64_t domain_id = 0;
    std::uint64_t boundary_handle = 0;
    std::uint64_t geometry_hypothesis_id = 0;
    EvidenceClass geometry_class = EvidenceClass::UNKNOWN;
    float support = 0.0F;
};

struct LightEmitter {
    std::uint64_t emitter_id = 0;
    EmitterKind kind = EmitterKind::DIRECTIONAL;
    EvidenceClass authority = EvidenceClass::COUNTERFACTUAL;

    std::array<float, 3> position_or_direction{};
    float relative_ev = 0.0F;
    float angular_extent_rad = 0.0F;

    // Zero means no independently bound spectral authority.
    std::uint64_t spectral_binding_id = 0;
    float spectral_support = 0.0F;

    // Descriptive only until a calibrated polarization model is bound.
    float polarization_fraction = 0.0F;
    float polarization_angle_rad = 0.0F;
};

struct MaterialHypothesis {
    std::uint64_t hypothesis_id = 0;
    EvidenceClass authority = EvidenceClass::UNKNOWN;
    float diffuse_weight = 0.0F;
    float specular_weight = 0.0F;
    float roughness = 0.5F;
    float index_of_refraction = 1.0F;
    float transmission = 0.0F;
    float absorption = 0.0F;
    float anisotropy = 0.0F;
    float subsurface_weight = 0.0F;
    float support = 0.0F;
};

struct AtmosphereHypothesis {
    std::uint64_t hypothesis_id = 0;
    EvidenceClass authority = EvidenceClass::UNKNOWN;
    float extinction = 0.0F;
    float scattering = 0.0F;
    float airlight = 0.0F;
    float support = 0.0F;
};

struct VirtualCapture {
    float relative_exposure_ev = 0.0F;
    float focus_distance_diopters = 0.0F;
    float aperture_f_number = 0.0F;
    std::uint64_t calibrated_sensor_mode_binding_id = 0;
    bool physical_snr_claim_allowed = false;
};

struct WorldState {
    std::array<std::uint8_t, 32> scientific_master_sha256{};
    std::array<std::uint8_t, 32> zero_line_binding_sha256{};
    std::array<std::uint8_t, 32> scene_scale_binding_sha256{};

    std::uint64_t world_id = 0;
    WorldAuthority authority = WorldAuthority::WS0_RELATIVE_APPEARANCE;
    std::uint32_t physical_frame_count = 1;
    std::uint32_t independent_evidence_count = 1;

    // Handles point to external descriptor stores; never full-frame pixel copies.
    std::uint64_t scene_domain_set_handle = 0;
    std::uint64_t emitter_set_handle = 0;
    std::uint64_t material_hypothesis_set_handle = 0;
    std::uint64_t atmosphere_hypothesis_handle = 0;
    std::uint64_t virtual_capture_handle = 0;
    std::uint64_t hypothesis_ledger_handle = 0;
};

constexpr bool authority_may_claim_physical_snr(WorldAuthority a) noexcept {
    return a == WorldAuthority::WS2_CALIBRATED_NEUTRAL_PHOTOMETRIC;
}

constexpr bool valid_evidence_invariants(const WorldState& s) noexcept {
    return s.physical_frame_count == 1 && s.independent_evidence_count == 1;
}

}  // namespace truthraw::fotograaf::world_studio_v0_1
