#pragma once

#include "multivendor_raw_source_adapter_v0_1.h"
#include "scientific_preview_source_binding_v0_1.h"
#include "tile_native_dng_source_v0_1.h"

#include <memory>

namespace truthraw::android_raw_adapter_bridge::v0_1 {

class RawByteSourceView final : public multivendor_raw_source_adapter::v0_1::IRawByteSource {
public:
    explicit RawByteSourceView(
        std::shared_ptr<tile_dng_v0_1::IRandomAccessByteSource> source) noexcept
        : source_(std::move(source)) {}

    std::uint64_t sizeBytes() const noexcept override { return source_->sizeBytes(); }
    std::size_t residentBytesUpperBound() const noexcept override {
        return source_->residentBytesUpperBound();
    }

    bool readExact(std::uint64_t offset, void* dst, std::size_t count) noexcept override {
        return source_->readExact(offset, dst, count);
    }

private:
    std::shared_ptr<tile_dng_v0_1::IRandomAccessByteSource> source_;
};

struct OpenedDngSource final {
    std::unique_ptr<streaming_v0_1::IRawTileSource> source;
    tile_dng_v0_1::TileNativeDngSource* dngAuditSource = nullptr;
    multivendor_raw_source_adapter::v0_1::RawSourceDescriptor descriptor{};
};

inline multivendor_raw_source_adapter::v0_1::AdapterStatus openDngViaAdapter(
    std::shared_ptr<tile_dng_v0_1::IRandomAccessByteSource> bytes,
    const scientific_preview_binding_v0_1::SourceSeal& sourceSeal,
    const tile_dng_v0_1::OpenOptions& admittedDngOptions,
    OpenedDngSource& out) noexcept {
    namespace adapter = multivendor_raw_source_adapter::v0_1;

    out = {};
    if (!bytes || sourceSeal.sourceEvidenceId.empty() || !admittedDngOptions.color.valid) {
        return adapter::AdapterStatus::error(
            adapter::AdapterStatusCode::InvalidArgument,
            "Android DNG adapter bridge requires admitted source seal and color");
    }

    adapter::RawSourceOpenRequest request;
    request.declaredFormat = adapter::RawFormatFamily::Dng;
    request.sourceSeal.valid = true;
    request.sourceSeal.sha256 = sourceSeal.sha256;
    request.sourceSeal.byteLength = sourceSeal.byteLength;
    request.sourceSeal.sourceEvidenceId = sourceSeal.sourceEvidenceId;
    request.color.valid = admittedDngOptions.color.valid;
    request.color.bindingId = admittedDngOptions.color.bindingId;
    request.color.cameraToXyzD50 = admittedDngOptions.color.cameraToXyzD50;
    request.maxResidentBytes = admittedDngOptions.maxResidentBytes;

    adapter::RawSourceAdapterRegistry registry;
    const auto registered = registry.registerAdapter(adapter::makeDngAdapter());
    if (!registered) return registered;

    auto genericBytes = std::make_shared<RawByteSourceView>(bytes);
    const auto opened = registry.open(
        genericBytes,
        request,
        out.source,
        out.descriptor);
    if (!opened) return opened;

    out.dngAuditSource =
        dynamic_cast<tile_dng_v0_1::TileNativeDngSource*>(out.source.get());
    if (out.dngAuditSource == nullptr) {
        out.source.reset();
        out.descriptor = {};
        return adapter::AdapterStatus::error(
            adapter::AdapterStatusCode::DecodeFailed,
            "DNG adapter did not return TileNativeDngSource implementation");
    }

    return adapter::AdapterStatus::ok();
}

} // namespace truthraw::android_raw_adapter_bridge::v0_1
