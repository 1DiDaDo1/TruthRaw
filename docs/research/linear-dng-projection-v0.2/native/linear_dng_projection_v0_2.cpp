#include "linear_dng_projection_v0_2.h"

#include <algorithm>
#include <array>
#include <cstdint>
#include <cstring>
#include <limits>
#include <string>
#include <vector>

namespace truthraw::linear_dng_projection::v0_2 {
namespace {

constexpr std::uint16_t kClassicTiffMagic = 42u;
constexpr std::uint16_t kTiffAscii = 2u;
constexpr std::uint16_t kTagMake = 271u;
constexpr std::uint16_t kTagModel = 272u;
constexpr std::uint16_t kTagUniqueCameraModel = 50708u;
constexpr std::uint16_t kTagBaselineExposure = 50730u;
constexpr std::uint32_t kMaxIfdEntries = 512u;
constexpr float kRepresentationScale = 0.5f;

std::uint16_t dec16(const std::uint8_t* p, bool little) noexcept {
    if (little) {
        return static_cast<std::uint16_t>(p[0]) |
               static_cast<std::uint16_t>(static_cast<std::uint16_t>(p[1]) << 8u);
    }
    return static_cast<std::uint16_t>(static_cast<std::uint16_t>(p[0]) << 8u) |
           static_cast<std::uint16_t>(p[1]);
}

std::uint32_t dec32(const std::uint8_t* p, bool little) noexcept {
    if (little) {
        return static_cast<std::uint32_t>(p[0]) |
               (static_cast<std::uint32_t>(p[1]) << 8u) |
               (static_cast<std::uint32_t>(p[2]) << 16u) |
               (static_cast<std::uint32_t>(p[3]) << 24u);
    }
    return (static_cast<std::uint32_t>(p[0]) << 24u) |
           (static_cast<std::uint32_t>(p[1]) << 16u) |
           (static_cast<std::uint32_t>(p[2]) << 8u) |
           static_cast<std::uint32_t>(p[3]);
}

void put32(std::uint8_t* p, std::uint32_t value, bool little) noexcept {
    if (little) {
        p[0] = static_cast<std::uint8_t>(value & 0xffu);
        p[1] = static_cast<std::uint8_t>((value >> 8u) & 0xffu);
        p[2] = static_cast<std::uint8_t>((value >> 16u) & 0xffu);
        p[3] = static_cast<std::uint8_t>((value >> 24u) & 0xffu);
    } else {
        p[0] = static_cast<std::uint8_t>((value >> 24u) & 0xffu);
        p[1] = static_cast<std::uint8_t>((value >> 16u) & 0xffu);
        p[2] = static_cast<std::uint8_t>((value >> 8u) & 0xffu);
        p[3] = static_cast<std::uint8_t>(value & 0xffu);
    }
}

bool read_exact(tile_dng_v0_1::IRandomAccessByteSource& source,
                std::uint64_t offset,
                void* dst,
                std::size_t bytes) noexcept {
    if (offset > source.sizeBytes() || bytes > source.sizeBytes() - offset) return false;
    return source.readExact(offset, dst, bytes);
}

struct SourceIdentity final {
    bool little = true;
    std::string make;
    std::string model;
    std::string uniqueCameraModel;
};

bool read_ascii_payload(tile_dng_v0_1::IRandomAccessByteSource& source,
                        const std::array<std::uint8_t,12>& entry,
                        bool little,
                        std::string& out) noexcept {
    const std::uint16_t type = dec16(entry.data() + 2u, little);
    const std::uint32_t count = dec32(entry.data() + 4u, little);
    if (type != kTiffAscii || count == 0u || count > 4096u) return false;

    std::vector<std::uint8_t> payload(count, 0u);
    if (count <= 4u) {
        std::memcpy(payload.data(), entry.data() + 8u, count);
    } else {
        const std::uint32_t offset = dec32(entry.data() + 8u, little);
        if (!read_exact(source, offset, payload.data(), payload.size())) return false;
    }
    const auto zero = std::find(payload.begin(), payload.end(), static_cast<std::uint8_t>(0u));
    out.assign(payload.begin(), zero);
    return !out.empty();
}

Status read_source_identity(tile_dng_v0_1::IRandomAccessByteSource& source,
                            SourceIdentity& out) noexcept {
    out = {};
    std::array<std::uint8_t,8> header{};
    if (!read_exact(source, 0u, header.data(), header.size())) {
        return Status::error(StatusCode::SourceIdentityFailed, "cannot read source TIFF header for camera identity");
    }
    if (header[0] == 'I' && header[1] == 'I') out.little = true;
    else if (header[0] == 'M' && header[1] == 'M') out.little = false;
    else return Status::error(StatusCode::SourceIdentityFailed, "source TIFF byte order is invalid");
    if (dec16(header.data() + 2u, out.little) != kClassicTiffMagic) {
        return Status::error(StatusCode::SourceIdentityFailed, "classic TIFF source required for v0.2 identity restore");
    }

    const std::uint32_t root = dec32(header.data() + 4u, out.little);
    std::array<std::uint8_t,2> countBytes{};
    if (!read_exact(source, root, countBytes.data(), countBytes.size())) {
        return Status::error(StatusCode::SourceIdentityFailed, "cannot read source IFD count");
    }
    const std::uint16_t count = dec16(countBytes.data(), out.little);
    if (count > kMaxIfdEntries) {
        return Status::error(StatusCode::SourceIdentityFailed, "source IFD entry cap exceeded");
    }

    std::array<std::uint8_t,12> entry{};
    for (std::uint16_t i = 0; i < count; ++i) {
        const std::uint64_t offset = static_cast<std::uint64_t>(root) + 2u + 12ull * i;
        if (!read_exact(source, offset, entry.data(), entry.size())) {
            return Status::error(StatusCode::SourceIdentityFailed, "cannot read source identity entry");
        }
        const std::uint16_t tag = dec16(entry.data(), out.little);
        if (tag == kTagMake) {
            if (!read_ascii_payload(source, entry, out.little, out.make)) {
                return Status::error(StatusCode::SourceIdentityFailed, "source Make tag is invalid");
            }
        } else if (tag == kTagModel) {
            if (!read_ascii_payload(source, entry, out.little, out.model)) {
                return Status::error(StatusCode::SourceIdentityFailed, "source Model tag is invalid");
            }
        } else if (tag == kTagUniqueCameraModel) {
            if (!read_ascii_payload(source, entry, out.little, out.uniqueCameraModel)) {
                return Status::error(StatusCode::SourceIdentityFailed, "source UniqueCameraModel tag is invalid");
            }
        }
    }
    if (out.uniqueCameraModel.empty()) {
        return Status::error(StatusCode::SourceIdentityFailed,
                             "source UniqueCameraModel is required to bind copied DNG colour metadata");
    }
    return Status::ok();
}

class ScaleForLinearDngReconstruction final : public IReconstructionBackend {
public:
    explicit ScaleForLinearDngReconstruction(IReconstructionBackend& base) noexcept : base_(base) {}

    ReconstructionQuality quality() const override { return base_.quality(); }
    const char* name() const override { return "linear_dng_v0_2_finite_2x_representation_wrapper"; }
    int requiredHalo() const override { return base_.requiredHalo(); }

    truthraw::Status reconstructTile(
        const float* stage2FullTile, int tileW, int tileH,
        int globalHx0, int globalHy0,
        int coreX0, int coreY0, int coreW, int coreH,
        CfaPattern cfa, float* coreCameraRgb) override {
        auto status = base_.reconstructTile(
            stage2FullTile, tileW, tileH,
            globalHx0, globalHy0,
            coreX0, coreY0, coreW, coreH,
            cfa, coreCameraRgb);
        if (!status) return status;
        const std::size_t count = static_cast<std::size_t>(coreW) *
                                  static_cast<std::size_t>(coreH) * 3u;
        for (std::size_t i = 0; i < count; ++i) coreCameraRgb[i] *= kRepresentationScale;
        return status;
    }
private:
    IReconstructionBackend& base_;
};

struct PatchSlot final {
    std::uint16_t tag = 0u;
    std::uint32_t offset = 0u;
    std::uint32_t allocation = 0u;
    std::string text;
    bool required = false;
    bool written = false;
};

class MetadataPatchSink final : public v0_1::IRandomAccessByteSink {
public:
    MetadataPatchSink(v0_1::IRandomAccessByteSink& base,
                      SourceIdentity identity) noexcept
        : base_(base), identity_(std::move(identity)) {}

    bool resize(std::uint64_t bytes) override {
        return base_.resize(bytes);
    }

    bool writeExact(std::uint64_t offset, const void* src, std::size_t bytes) override {
        if (failed_) return false;
        if (offset == 8u) {
            std::vector<std::uint8_t> patched(bytes, 0u);
            if (bytes != 0u) std::memcpy(patched.data(), src, bytes);
            if (!inspect_and_patch_ifd(patched)) {
                failed_ = true;
                return false;
            }
            return base_.writeExact(offset, patched.data(), patched.size());
        }

        if (offset == baselineOffset_) {
            if (bytes != 8u) {
                failed_ = true;
                return false;
            }
            std::array<std::uint8_t,8> payload{};
            put32(payload.data(), 1u, identity_.little);
            put32(payload.data() + 4u, 1u, identity_.little);
            baselineWritten_ = base_.writeExact(offset, payload.data(), payload.size());
            return baselineWritten_;
        }

        for (auto& slot : slots_) {
            if (offset != slot.offset) continue;
            if (bytes != slot.allocation || slot.text.size() + 1u > slot.allocation) {
                failed_ = true;
                return false;
            }
            std::vector<std::uint8_t> payload(slot.allocation, 0u);
            std::copy(slot.text.begin(), slot.text.end(), payload.begin());
            slot.written = base_.writeExact(offset, payload.data(), payload.size());
            return slot.written;
        }
        return base_.writeExact(offset, src, bytes);
    }

    std::size_t residentBytesUpperBound() const override {
        std::size_t extra = sizeof(*this) + identity_.make.capacity() +
                            identity_.model.capacity() + identity_.uniqueCameraModel.capacity();
        for (const auto& slot : slots_) extra += slot.text.capacity();
        const std::size_t base = base_.residentBytesUpperBound();
        if (base > std::numeric_limits<std::size_t>::max() - extra) {
            return std::numeric_limits<std::size_t>::max();
        }
        return base + extra;
    }

    bool complete() const noexcept {
        if (failed_ || !ifdSeen_ || !baselineWritten_) return false;
        bool uniqueWritten = false;
        for (const auto& slot : slots_) {
            if (slot.required && !slot.written) return false;
            if (slot.tag == kTagUniqueCameraModel && slot.written) uniqueWritten = true;
        }
        return uniqueWritten;
    }

    bool failed() const noexcept { return failed_; }

private:
    bool add_identity_slot(std::vector<std::uint8_t>& ifd,
                           std::size_t entryOffset,
                           std::uint16_t tag,
                           const std::string& text,
                           bool required) {
        if (text.empty()) return !required;
        const std::uint16_t type = dec16(ifd.data() + entryOffset + 2u, identity_.little);
        const std::uint32_t count = dec32(ifd.data() + entryOffset + 4u, identity_.little);
        const std::uint32_t payloadOffset = dec32(ifd.data() + entryOffset + 8u, identity_.little);
        if (type != kTiffAscii || count <= 4u || payloadOffset == 0u || text.size() + 1u > count) return false;
        put32(ifd.data() + entryOffset + 4u,
              static_cast<std::uint32_t>(text.size() + 1u), identity_.little);
        slots_.push_back(PatchSlot{tag, payloadOffset, count, text, required, false});
        return true;
    }

    bool inspect_and_patch_ifd(std::vector<std::uint8_t>& ifd) {
        if (ifd.size() < 6u) return false;
        const std::uint16_t count = dec16(ifd.data(), identity_.little);
        if (count > kMaxIfdEntries || 2u + 12ull * count + 4u > ifd.size()) return false;

        bool baselineSeen = false;
        bool uniqueSeen = false;
        for (std::uint16_t i = 0; i < count; ++i) {
            const std::size_t e = 2u + 12u * i;
            const std::uint16_t tag = dec16(ifd.data() + e, identity_.little);
            if (tag == kTagBaselineExposure) {
                const std::uint32_t itemCount = dec32(ifd.data() + e + 4u, identity_.little);
                if (itemCount != 1u) return false;
                baselineOffset_ = dec32(ifd.data() + e + 8u, identity_.little);
                baselineSeen = baselineOffset_ != 0u;
            } else if (tag == kTagMake) {
                if (!add_identity_slot(ifd, e, tag, identity_.make, false)) return false;
            } else if (tag == kTagModel) {
                if (!add_identity_slot(ifd, e, tag, identity_.model, false)) return false;
            } else if (tag == kTagUniqueCameraModel) {
                if (!add_identity_slot(ifd, e, tag, identity_.uniqueCameraModel, true)) return false;
                uniqueSeen = true;
            }
        }
        ifdSeen_ = baselineSeen && uniqueSeen;
        return ifdSeen_;
    }

    v0_1::IRandomAccessByteSink& base_;
    SourceIdentity identity_;
    std::vector<PatchSlot> slots_;
    std::uint32_t baselineOffset_ = std::numeric_limits<std::uint32_t>::max();
    bool ifdSeen_ = false;
    bool baselineWritten_ = false;
    bool failed_ = false;
};

} // namespace

Status write_finalized_linear_dng(
    const finalized_scientific_preview_release::v0_2::ReleaseResult& finalized,
    tile_dng_v0_1::IRandomAccessByteSource& sealedSourceBytes,
    streaming_v0_1::IRawTileSource& source,
    IReconstructionBackend& reconstruction,
    v0_1::IRandomAccessByteSink& sink,
    const Options& options,
    Result& out) noexcept {
    out = {};
    SourceIdentity identity;
    auto identityStatus = read_source_identity(sealedSourceBytes, identity);
    if (!identityStatus) return identityStatus;

    ScaleForLinearDngReconstruction scaled(reconstruction);
    MetadataPatchSink patchedSink(sink, std::move(identity));
    v0_1::Options baseOptions;
    baseOptions.memoryBudgetBytes = options.memoryBudgetBytes;
    v0_1::Result baseResult;
    const auto baseStatus = v0_1::write_finalized_linear_dng(
        finalized,
        sealedSourceBytes,
        source,
        scaled,
        patchedSink,
        baseOptions,
        baseResult);
    if (!baseStatus) {
        (void)sink.resize(0u);
        if (patchedSink.failed()) {
            return Status::error(StatusCode::MetadataPatchFailed,
                                 "v0.2 could not restore finite-window/camera-identity metadata");
        }
        return Status::error(StatusCode::UnderlyingProjectionFailed,
                             std::string("v0.1 bounded writer failed: ") + baseStatus.message);
    }
    if (!patchedSink.complete()) {
        (void)sink.resize(0u);
        return Status::error(StatusCode::MetadataPatchFailed,
                             "v0.2 DNG metadata restoration did not complete");
    }

    if (baseResult.samplesClippedHigh != 0u) {
        (void)sink.resize(0u);
        out.base = baseResult;
        out.overWindowRejected = true;
        return Status::error(StatusCode::SceneExceedsValidatedWindow,
                             "reconstructed RGB exceeds validated 2x LinearRaw compatibility window");
    }

    out.base = baseResult;
    out.compatibilityWindow = 2.0;
    out.baselineExposureEv = 1.0;
    out.sourceCameraIdentityPreserved = true;
    out.overWindowRejected = false;
    return Status::ok();
}

const char* status_name(StatusCode code) noexcept {
    switch (code) {
        case StatusCode::Ok: return "OK";
        case StatusCode::InvalidArgument: return "INVALID_ARGUMENT";
        case StatusCode::SourceIdentityFailed: return "SOURCE_IDENTITY_FAILED";
        case StatusCode::UnderlyingProjectionFailed: return "UNDERLYING_PROJECTION_FAILED";
        case StatusCode::SceneExceedsValidatedWindow: return "SCENE_EXCEEDS_VALIDATED_WINDOW";
        case StatusCode::MetadataPatchFailed: return "METADATA_PATCH_FAILED";
    }
    return "UNKNOWN";
}

} // namespace truthraw::linear_dng_projection::v0_2
