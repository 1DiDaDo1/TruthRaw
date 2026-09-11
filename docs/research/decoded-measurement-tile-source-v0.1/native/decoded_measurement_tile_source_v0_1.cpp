#include "decoded_measurement_tile_source_v0_1.h"

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <limits>
#include <span>

namespace truthraw::decoded_measurement_tile_source::v0_1 {
namespace {

[[nodiscard]] bool finite_metadata(const DngMetadata& m) noexcept {
    if (m.width <= 0 || m.height <= 0 ||
        !std::isfinite(m.whiteLevel) || m.whiteLevel <= 0.0f ||
        m.sourceId.empty()) {
        return false;
    }
    for (const float black : m.blackPhase) {
        if (!std::isfinite(black) || black < 0.0f || black >= m.whiteLevel) return false;
    }
    if (m.hasNoiseProfile) {
        for (const float v : m.noiseProfile) if (!std::isfinite(v)) return false;
    }
    for (const float v : m.cameraToXyzD50) if (!std::isfinite(v)) return false;
    switch (m.orientation) {
        case Orientation::Normal:
        case Orientation::Rotate180:
        case Orientation::Rotate90CW:
        case Orientation::Rotate90CCW:
            break;
        default:
            return false;
    }
    return true;
}

[[nodiscard]] bool descriptor_cfa(
    const std::array<handoff::CfaColor, 4>& cfa,
    CfaPattern& out) noexcept {
    using C = handoff::CfaColor;
    if (cfa == std::array<C,4>{C::Red, C::Green, C::Green, C::Blue}) {
        out = CfaPattern::RGGB;
        return true;
    }
    if (cfa == std::array<C,4>{C::Blue, C::Green, C::Green, C::Red}) {
        out = CfaPattern::BGGR;
        return true;
    }
    if (cfa == std::array<C,4>{C::Green, C::Red, C::Blue, C::Green}) {
        out = CfaPattern::GRBG;
        return true;
    }
    if (cfa == std::array<C,4>{C::Green, C::Blue, C::Red, C::Green}) {
        out = CfaPattern::GBRG;
        return true;
    }
    return false;
}

[[nodiscard]] streaming::StreamStatus source_error(const char* message) {
    return streaming::StreamStatus::error(
        streaming::StreamStatusCode::SourceFailed, message);
}

} // namespace

BindStatus DecodedMeasurementTileSource::bind(
    handoff::Reader& reader,
    const DngMetadata& certifiedMetadata,
    const BindingAuthority& authority) noexcept {
    if (reader_ != nullptr || !reader.open_ok()) return BindStatus::InvalidArgument;
    if (!reader.info().payloadIntegrityVerified) return BindStatus::IntegrityNotVerified;
    if (!authority.metadataBoundToOriginalSource ||
        !authority.metadataSemanticsVerified ||
        !authority.decodedStoreBoundToOriginalSource ||
        !authority.gatehouseDetached) {
        return BindStatus::AuthorityMissing;
    }
    if (!finite_metadata(certifiedMetadata)) return BindStatus::InvalidArgument;

    const auto& d = reader.info().descriptor;
    if (d.topology != ingress::MeasurementTopology::Bayer2x2 ||
        d.evidenceClass != ingress::EvidenceClass::LosslessDecodedCertified ||
        d.physicalFrameCount != 1u || d.independentEvidenceCount != 1u) {
        return BindStatus::MetadataMismatch;
    }

    CfaPattern expectedCfa = CfaPattern::RGGB;
    if (!descriptor_cfa(d.cfa2x2, expectedCfa) ||
        certifiedMetadata.width != static_cast<int>(d.width) ||
        certifiedMetadata.height != static_cast<int>(d.height) ||
        certifiedMetadata.cfa != expectedCfa) {
        return BindStatus::MetadataMismatch;
    }

    // v0.1 does not silently discard GainMap or residual-black semantics.
    // Such sources remain blocked until their data are explicitly carried by
    // the detached handoff rather than borrowed from decoder state.
    if (certifiedMetadata.hasGainField || certifiedMetadata.hasResidualBlack) {
        return BindStatus::UnsupportedMetadata;
    }

    reader_ = &reader;
    metadata_ = certifiedMetadata;
    authority_ = authority;
    return BindStatus::Ok;
}

std::size_t DecodedMeasurementTileSource::residentBytesUpperBound() const {
    std::size_t total = sizeof(DecodedMeasurementTileSource);
    if (reader_ != nullptr) {
        const std::size_t readerBytes = reader_->resident_bytes_upper_bound();
        if (readerBytes > std::numeric_limits<std::size_t>::max() - total) {
            return std::numeric_limits<std::size_t>::max();
        }
        total += readerBytes;
    }
    const std::size_t sourceCapacity = metadata_.sourceId.capacity();
    if (sourceCapacity > std::numeric_limits<std::size_t>::max() - total) {
        return std::numeric_limits<std::size_t>::max();
    }
    return total + sourceCapacity;
}

streaming::StreamStatus DecodedMeasurementTileSource::readRawTile(
    const TileRect& rect,
    std::uint16_t* rawOut,
    std::size_t rawCount,
    float* gainOut,
    std::size_t gainCount) {
    if (reader_ == nullptr) return source_error("decoded handoff source is not bound");
    if (rawOut == nullptr || rect.hx0 < 0 || rect.hy0 < 0 ||
        rect.hx1 > metadata_.width || rect.hy1 > metadata_.height ||
        rect.hx1 <= rect.hx0 || rect.hy1 <= rect.hy0) {
        return streaming::StreamStatus::error(
            streaming::StreamStatusCode::InvalidArgument,
            "invalid decoded handoff tile rectangle");
    }

    const std::size_t width = static_cast<std::size_t>(rect.hx1 - rect.hx0);
    const std::size_t height = static_cast<std::size_t>(rect.hy1 - rect.hy0);
    if (height != 0 && width > std::numeric_limits<std::size_t>::max() / height) {
        return streaming::StreamStatus::error(
            streaming::StreamStatusCode::InvalidArgument,
            "decoded handoff tile size overflow");
    }
    const std::size_t sampleCount = width * height;
    if (rawCount < sampleCount) {
        return streaming::StreamStatus::error(
            streaming::StreamStatusCode::InvalidArgument,
            "decoded handoff raw output too small");
    }
    if (gainOut != nullptr && gainCount < sampleCount) {
        return streaming::StreamStatus::error(
            streaming::StreamStatusCode::InvalidArgument,
            "decoded handoff gain output count inconsistent");
    }

    const auto status = reader_->read_rect(
        static_cast<std::uint32_t>(rect.hx0),
        static_cast<std::uint32_t>(rect.hy0),
        static_cast<std::uint32_t>(rect.hx1 - rect.hx0),
        static_cast<std::uint32_t>(rect.hy1 - rect.hy0),
        std::span<std::uint16_t>(rawOut, sampleCount));
    if (status != handoff::Status::Ok) {
        return source_error(handoff::status_name(status));
    }
    return streaming::StreamStatus::ok();
}

streaming::StreamStatus DecodedMeasurementTileSource::readRowBias(
    int y0,
    int y1,
    float* out,
    std::size_t count) {
    if (reader_ == nullptr) return source_error("decoded handoff source is not bound");
    if (y0 < 0 || y1 < y0 || y1 > metadata_.height ||
        count < static_cast<std::size_t>(y1 - y0) ||
        (y1 > y0 && out == nullptr)) {
        return streaming::StreamStatus::error(
            streaming::StreamStatusCode::InvalidArgument,
            "invalid decoded handoff row-bias request");
    }
    if (y1 > y0) std::fill(out, out + (y1 - y0), 0.0f);
    return streaming::StreamStatus::ok();
}

streaming::StreamStatus DecodedMeasurementTileSource::readColBias(
    int x0,
    int x1,
    float* out,
    std::size_t count) {
    if (reader_ == nullptr) return source_error("decoded handoff source is not bound");
    if (x0 < 0 || x1 < x0 || x1 > metadata_.width ||
        count < static_cast<std::size_t>(x1 - x0) ||
        (x1 > x0 && out == nullptr)) {
        return streaming::StreamStatus::error(
            streaming::StreamStatusCode::InvalidArgument,
            "invalid decoded handoff col-bias request");
    }
    if (x1 > x0) std::fill(out, out + (x1 - x0), 0.0f);
    return streaming::StreamStatus::ok();
}

const char* bind_status_name(BindStatus status) noexcept {
    switch (status) {
        case BindStatus::Ok: return "OK";
        case BindStatus::InvalidArgument: return "INVALID_ARGUMENT";
        case BindStatus::IntegrityNotVerified: return "INTEGRITY_NOT_VERIFIED";
        case BindStatus::AuthorityMissing: return "AUTHORITY_MISSING";
        case BindStatus::MetadataMismatch: return "METADATA_MISMATCH";
        case BindStatus::UnsupportedMetadata: return "UNSUPPORTED_METADATA";
    }
    return "UNKNOWN";
}

} // namespace truthraw::decoded_measurement_tile_source::v0_1
