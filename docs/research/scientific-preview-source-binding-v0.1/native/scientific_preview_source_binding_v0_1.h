#pragma once

#include "technical_backplane_v0_1.h"
#include "tile_native_dng_source_v0_1.h"

#include <array>
#include <cstddef>
#include <cstdint>
#include <string>

namespace truthraw::scientific_preview_binding_v0_1 {

constexpr std::size_t kSha256Bytes = 32;
constexpr std::size_t kDefaultHashChunkBytes = 64u * 1024u;

enum class ColorBindingAuthority : std::uint8_t {
    Unverified = 0,
    PreviewSentinel,
    SourceMetadataBound,
    GatehouseCertifiedMetadata,
    IndependentCalibration,
};

enum class ColorClaimScope : std::uint8_t {
    None = 0,
    SourceBoundPreview,
    IndependentlyCalibratedPreview,
};

enum class BindingStatusCode : std::uint8_t {
    Ok = 0,
    InvalidArgument,
    SourceReadFailed,
    InvalidSourceSeal,
    SourceSealMismatch,
    UnauthorizedColorBinding,
    BindingSourceMismatch,
    InvalidMatrix,
    EvidenceInvariantViolation,
    BackplaneRejected,
    BackplaneSourceMismatch,
};

struct BindingStatus {
    BindingStatusCode code = BindingStatusCode::Ok;
    std::string message;
    explicit operator bool() const noexcept { return code == BindingStatusCode::Ok; }
    static BindingStatus ok() { return {}; }
    static BindingStatus error(BindingStatusCode code, std::string message) {
        BindingStatus out; out.code = code; out.message = std::move(message); return out;
    }
};

struct SourceSeal {
    std::array<std::uint8_t, kSha256Bytes> sha256{};
    std::uint64_t byteLength = 0;
    std::size_t hashWorkspacePeakBytes = 0;
    std::string sourceEvidenceId;
};

struct ScientificColorBindingRecord {
    ColorBindingAuthority authority = ColorBindingAuthority::Unverified;
    std::string sourceEvidenceId;
    std::string bindingId;
    std::array<float, 9> cameraToXyzD50 = {1.f,0.f,0.f, 0.f,1.f,0.f, 0.f,0.f,1.f};
    bool normalized = true;
    bool validated = false;
    std::uint32_t physicalFrameCount = 1;
    std::uint32_t independentEvidenceCount = 1;
};

struct ScientificPreviewAdmission {
    tile_dng_v0_1::OpenOptions tileNativeOptions{};
    ColorClaimScope claimScope = ColorClaimScope::None;
    SourceSeal sourceSeal{};
};

BindingStatus seal_source_sha256(
    tile_dng_v0_1::IRandomAccessByteSource& source,
    SourceSeal& out,
    std::size_t chunkBytes = kDefaultHashChunkBytes);

BindingStatus reverify_source_sha256(
    tile_dng_v0_1::IRandomAccessByteSource& source,
    const SourceSeal& expected,
    std::size_t chunkBytes = kDefaultHashChunkBytes);

BindingStatus admit_scientific_color_preview(
    const SourceSeal& sourceSeal,
    const ScientificColorBindingRecord& color,
    const technical_backplane::v0_1::State& backplane,
    ScientificPreviewAdmission& out);

bool is_canonical_source_evidence_id(const std::string& id) noexcept;
const char* authority_name(ColorBindingAuthority authority) noexcept;
const char* status_name(BindingStatusCode code) noexcept;

} // namespace truthraw::scientific_preview_binding_v0_1
