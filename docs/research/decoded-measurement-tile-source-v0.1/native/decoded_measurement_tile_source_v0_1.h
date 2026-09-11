#pragma once

#include "decoded_measurement_handoff_v0_1.h"
#include "full_frame_streaming_v0_1.h"

#include <cstddef>

namespace truthraw::decoded_measurement_tile_source::v0_1 {

namespace handoff = truthraw::decoded_measurement_handoff::v0_1;
namespace ingress = truthraw::professional_raw_ingress::v0_1;
namespace streaming = truthraw::streaming_v0_1;

struct BindingAuthority {
    bool metadataBoundToOriginalSource = false;
    bool metadataSemanticsVerified = false;
    bool decodedStoreBoundToOriginalSource = false;
    bool gatehouseDetached = false;
};

enum class BindStatus : std::uint8_t {
    Ok = 0,
    InvalidArgument,
    IntegrityNotVerified,
    AuthorityMissing,
    MetadataMismatch,
    UnsupportedMetadata,
};

class DecodedMeasurementTileSource final : public streaming::IRawTileSource {
public:
    DecodedMeasurementTileSource() = default;
    DecodedMeasurementTileSource(const DecodedMeasurementTileSource&) = delete;
    DecodedMeasurementTileSource& operator=(const DecodedMeasurementTileSource&) = delete;

    [[nodiscard]] BindStatus bind(
        handoff::Reader& reader,
        const DngMetadata& certifiedMetadata,
        const BindingAuthority& authority) noexcept;

    [[nodiscard]] bool bound() const noexcept { return reader_ != nullptr; }
    [[nodiscard]] const BindingAuthority& authority() const noexcept { return authority_; }

    const DngMetadata& metadata() const override { return metadata_; }
    std::size_t residentBytesUpperBound() const override;

    streaming::StreamStatus readRawTile(
        const TileRect& rect,
        std::uint16_t* rawOut,
        std::size_t rawCount,
        float* gainOut,
        std::size_t gainCount) override;

    streaming::StreamStatus readRowBias(
        int y0,
        int y1,
        float* out,
        std::size_t count) override;

    streaming::StreamStatus readColBias(
        int x0,
        int x1,
        float* out,
        std::size_t count) override;

private:
    handoff::Reader* reader_ = nullptr; // borrowed; Gatehouse store remains externally owned.
    DngMetadata metadata_{};           // small certified metadata copy, never pixel payload.
    BindingAuthority authority_{};
};

[[nodiscard]] const char* bind_status_name(BindStatus status) noexcept;

} // namespace truthraw::decoded_measurement_tile_source::v0_1
