#include "multivendor_raw_source_adapter_v0_1.h"

#include "tile_native_dng_source_v0_1.h"

#include <algorithm>
#include <array>
#include <cstring>
#include <limits>
#include <memory>
#include <new>
#include <utility>

namespace truthraw::multivendor_raw_source_adapter::v0_1 {
namespace {

using streaming_v0_1::IRawTileSource;
using streaming_v0_1::StreamStatus;
using streaming_v0_1::StreamStatusCode;
using tile_dng_v0_1::ColorBinding;
using tile_dng_v0_1::DngSourceCode;
using tile_dng_v0_1::OpenOptions;
using tile_dng_v0_1::TileNativeDngSource;

AdapterStatus validateCommon(
    const std::shared_ptr<IRawByteSource>& bytes,
    const RawSourceOpenRequest& request) noexcept {
    if (!bytes) {
        return AdapterStatus::error(AdapterStatusCode::InvalidArgument, "null RAW byte source");
    }
    if (!request.sourceSeal.valid || request.sourceSeal.sourceEvidenceId.empty()) {
        return AdapterStatus::error(AdapterStatusCode::InvalidArgument, "valid sealed source identity required");
    }
    if (request.sourceSeal.byteLength != bytes->sizeBytes()) {
        return AdapterStatus::error(
            AdapterStatusCode::SourceSealMismatch,
            "sealed byte length does not match RAW byte source length");
    }
    if (request.maxResidentBytes == 0u) {
        return AdapterStatus::error(AdapterStatusCode::InvalidArgument, "resident-memory budget must be non-zero");
    }
    return AdapterStatus::ok();
}

class DngByteSourceView final : public tile_dng_v0_1::IRandomAccessByteSource {
public:
    explicit DngByteSourceView(std::shared_ptr<IRawByteSource> source) : source_(std::move(source)) {}

    std::uint64_t sizeBytes() const override { return source_->sizeBytes(); }
    std::size_t residentBytesUpperBound() const override { return source_->residentBytesUpperBound(); }

    bool readExact(std::uint64_t offset, void* dst, std::size_t count) override {
        return source_->readExact(offset, dst, count);
    }

private:
    std::shared_ptr<IRawByteSource> source_;
};

class DngAdapter final : public IRawSourceAdapter {
public:
    RawFormatFamily formatFamily() const noexcept override { return RawFormatFamily::Dng; }

    AdapterStatus open(
        std::shared_ptr<IRawByteSource> bytes,
        const RawSourceOpenRequest& request,
        std::unique_ptr<IRawTileSource>& outSource,
        RawSourceDescriptor& outDescriptor) noexcept override {
        outSource.reset();
        outDescriptor = {};

        const auto valid = validateCommon(bytes, request);
        if (!valid) return valid;
        if (request.declaredFormat != RawFormatFamily::Dng) {
            return AdapterStatus::error(AdapterStatusCode::InvalidArgument, "DNG adapter received non-DNG format");
        }
        if (!request.color.valid || request.color.bindingId.empty()) {
            return AdapterStatus::error(
                AdapterStatusCode::InvalidArgument,
                "DNG adapter requires admitted source-bound color binding");
        }

        auto view = std::make_shared<DngByteSourceView>(std::move(bytes));
        OpenOptions options;
        options.sourceEvidenceId = request.sourceSeal.sourceEvidenceId;
        options.color = ColorBinding{};
        options.color.valid = true;
        options.color.bindingId = request.color.bindingId;
        options.color.cameraToXyzD50 = request.color.cameraToXyzD50;
        options.maxResidentBytes = request.maxResidentBytes;

        std::unique_ptr<TileNativeDngSource> dng;
        const auto opened = TileNativeDngSource::open(view, options, dng);
        if (!opened) {
            AdapterStatusCode code = AdapterStatusCode::DecodeFailed;
            if (opened.code == DngSourceCode::BudgetExceeded) {
                code = AdapterStatusCode::BudgetExceeded;
            } else if (
                opened.code == DngSourceCode::UnsupportedBigTiff ||
                opened.code == DngSourceCode::UnsupportedCompression ||
                opened.code == DngSourceCode::UnsupportedBitsPerSample ||
                opened.code == DngSourceCode::UnsupportedPhotometric ||
                opened.code == DngSourceCode::UnsupportedTopology) {
                code = AdapterStatusCode::UnsupportedContainerFeature;
            } else if (
                opened.code == DngSourceCode::InvalidTiff ||
                opened.code == DngSourceCode::MissingRequiredTag ||
                opened.code == DngSourceCode::InvalidTag ||
                opened.code == DngSourceCode::InvalidStorage) {
                code = AdapterStatusCode::InvalidContainer;
            }
            return AdapterStatus::error(code, "TileNativeDngSource rejected DNG: " + opened.message);
        }

        outDescriptor.format = RawFormatFamily::Dng;
        outDescriptor.storageRepresentation = StorageRepresentation::CfaMosaic;
        outDescriptor.sampleTopology = SampleTopologyFamily::Bayer2x2;
        outDescriptor.processingLineage = ProcessingLineageClass::DirectCfaStorageUncertified;
        outDescriptor.decoderId = "truthraw.tile-native-dng-source.v0.1";
        outDescriptor.sourceEvidenceId = request.sourceSeal.sourceEvidenceId;
        outDescriptor.sourceSealAcceptedAtBoundary = true;
        outDescriptor.exactCfaSamplesAvailable = true;
        outDescriptor.scientificColorBindingProvided = true;
        outDescriptor.measurementAdmissionReady = true;
        outDescriptor.scientificAdmissionReady = true;
        outDescriptor.syntheticConformanceOnly = false;
        outDescriptor.physicalExposureCountKnown = false;
        outDescriptor.physicalFrameCount = 0u;
        outDescriptor.singleExposureCertified = false;
        outDescriptor.storedSampleSenselSemanticsCertified = false;
        outDescriptor.requiresTopologySpecificSolver = false;
        outDescriptor.directSensorAdcClaimAllowed = false;
        outDescriptor.fullRawFrameMaterialized = dng->audit().fullRawMaterialized;

        outSource = std::move(dng);
        return AdapterStatus::ok();
    }
};

constexpr std::array<std::uint8_t, 8> kSyntheticMagic = {
    'T','R','A','W','V','0','0','1'
};
constexpr std::size_t kSyntheticHeaderBytes = 28u;

std::uint16_t readLe16(const std::uint8_t* p) noexcept {
    return static_cast<std::uint16_t>(p[0]) |
        static_cast<std::uint16_t>(static_cast<std::uint16_t>(p[1]) << 8u);
}

std::uint32_t readLe32(const std::uint8_t* p) noexcept {
    return static_cast<std::uint32_t>(p[0]) |
        (static_cast<std::uint32_t>(p[1]) << 8u) |
        (static_cast<std::uint32_t>(p[2]) << 16u) |
        (static_cast<std::uint32_t>(p[3]) << 24u);
}

class SyntheticFixtureSource final : public IRawTileSource {
public:
    SyntheticFixtureSource(
        std::shared_ptr<IRawByteSource> bytes,
        DngMetadata metadata) noexcept
        : bytes_(std::move(bytes)), metadata_(std::move(metadata)) {}

    const DngMetadata& metadata() const override { return metadata_; }

    std::size_t residentBytesUpperBound() const override {
        return sizeof(*this) + bytes_->residentBytesUpperBound();
    }

    StreamStatus readRawTile(
        const TileRect& rect,
        std::uint16_t* rawOut,
        std::size_t rawCount,
        float* gainOut,
        std::size_t gainCount) override {
        if (rawOut == nullptr || gainOut != nullptr || gainCount != 0u ||
            rect.hx0 < 0 || rect.hy0 < 0 ||
            rect.hx1 > metadata_.width || rect.hy1 > metadata_.height ||
            rect.hx0 >= rect.hx1 || rect.hy0 >= rect.hy1) {
            return StreamStatus::error(StreamStatusCode::InvalidArgument, "invalid synthetic tile request");
        }
        const std::size_t width = static_cast<std::size_t>(rect.hx1 - rect.hx0);
        const std::size_t height = static_cast<std::size_t>(rect.hy1 - rect.hy0);
        if (width > std::numeric_limits<std::size_t>::max() / height || rawCount != width * height) {
            return StreamStatus::error(StreamStatusCode::InvalidArgument, "synthetic tile count mismatch");
        }

        std::array<std::uint8_t, 2> sample{};
        std::size_t out = 0u;
        for (int y = rect.hy0; y < rect.hy1; ++y) {
            for (int x = rect.hx0; x < rect.hx1; ++x) {
                const std::uint64_t index =
                    static_cast<std::uint64_t>(y) * static_cast<std::uint64_t>(metadata_.width) +
                    static_cast<std::uint64_t>(x);
                const std::uint64_t offset = kSyntheticHeaderBytes + index * 2u;
                if (!bytes_->readExact(offset, sample.data(), sample.size())) {
                    return StreamStatus::error(StreamStatusCode::SourceFailed, "synthetic sample read failed");
                }
                rawOut[out++] = readLe16(sample.data());
            }
        }
        return StreamStatus::ok();
    }

    StreamStatus readRowBias(int y0, int y1, float* out, std::size_t count) override {
        if (out == nullptr || y0 < 0 || y1 < y0 || y1 > metadata_.height ||
            count != static_cast<std::size_t>(y1 - y0)) {
            return StreamStatus::error(StreamStatusCode::InvalidArgument, "invalid synthetic row-bias request");
        }
        std::fill(out, out + count, 0.0f);
        return StreamStatus::ok();
    }

    StreamStatus readColBias(int x0, int x1, float* out, std::size_t count) override {
        if (out == nullptr || x0 < 0 || x1 < x0 || x1 > metadata_.width ||
            count != static_cast<std::size_t>(x1 - x0)) {
            return StreamStatus::error(StreamStatusCode::InvalidArgument, "invalid synthetic col-bias request");
        }
        std::fill(out, out + count, 0.0f);
        return StreamStatus::ok();
    }

private:
    std::shared_ptr<IRawByteSource> bytes_;
    DngMetadata metadata_{};
};

class SyntheticFixtureAdapter final : public IRawSourceAdapter {
public:
    RawFormatFamily formatFamily() const noexcept override {
        return RawFormatFamily::SyntheticConformanceFixture;
    }

    AdapterStatus open(
        std::shared_ptr<IRawByteSource> bytes,
        const RawSourceOpenRequest& request,
        std::unique_ptr<IRawTileSource>& outSource,
        RawSourceDescriptor& outDescriptor) noexcept override {
        outSource.reset();
        outDescriptor = {};

        const auto valid = validateCommon(bytes, request);
        if (!valid) return valid;
        if (request.declaredFormat != RawFormatFamily::SyntheticConformanceFixture) {
            return AdapterStatus::error(AdapterStatusCode::InvalidArgument, "synthetic adapter received wrong format");
        }
        if (!request.color.valid || request.color.bindingId.empty()) {
            return AdapterStatus::error(
                AdapterStatusCode::InvalidArgument,
                "synthetic conformance fixture requires explicit test color binding");
        }
        if (bytes->sizeBytes() < kSyntheticHeaderBytes) {
            return AdapterStatus::error(AdapterStatusCode::InvalidContainer, "synthetic fixture too small");
        }

        std::array<std::uint8_t, kSyntheticHeaderBytes> header{};
        if (!bytes->readExact(0u, header.data(), header.size())) {
            return AdapterStatus::error(AdapterStatusCode::DecodeFailed, "synthetic header read failed");
        }
        if (!std::equal(kSyntheticMagic.begin(), kSyntheticMagic.end(), header.begin())) {
            return AdapterStatus::error(AdapterStatusCode::InvalidContainer, "synthetic fixture magic mismatch");
        }

        const std::uint32_t width = readLe32(header.data() + 8u);
        const std::uint32_t height = readLe32(header.data() + 12u);
        const std::uint8_t cfaCode = header[16u];
        const std::uint8_t orientation = header[17u];
        const std::uint16_t white = readLe16(header.data() + 18u);

        if (width < 2u || height < 2u || cfaCode > 3u || orientation != 1u || white == 0u) {
            return AdapterStatus::error(AdapterStatusCode::InvalidContainer, "synthetic fixture header invalid");
        }
        const std::uint64_t pixels = static_cast<std::uint64_t>(width) * static_cast<std::uint64_t>(height);
        if (pixels > (std::numeric_limits<std::uint64_t>::max() - kSyntheticHeaderBytes) / 2u) {
            return AdapterStatus::error(AdapterStatusCode::InvalidContainer, "synthetic fixture size overflow");
        }
        const std::uint64_t expectedBytes = kSyntheticHeaderBytes + pixels * 2u;
        if (bytes->sizeBytes() != expectedBytes) {
            return AdapterStatus::error(AdapterStatusCode::InvalidContainer, "synthetic payload length mismatch");
        }

        DngMetadata metadata;
        metadata.width = static_cast<int>(width);
        metadata.height = static_cast<int>(height);
        metadata.cfa = static_cast<CfaPattern>(cfaCode);
        metadata.orientation = Orientation::Normal;
        metadata.whiteLevel = static_cast<float>(white);
        metadata.blackPhase = {
            static_cast<float>(readLe16(header.data() + 20u)),
            static_cast<float>(readLe16(header.data() + 22u)),
            static_cast<float>(readLe16(header.data() + 24u)),
            static_cast<float>(readLe16(header.data() + 26u)),
        };
        metadata.cameraToXyzD50 = request.color.cameraToXyzD50;
        metadata.sourceId = request.sourceSeal.sourceEvidenceId;
        metadata.hasNoiseProfile = false;
        metadata.hasGainField = false;
        metadata.hasResidualBlack = false;

        auto source = std::unique_ptr<SyntheticFixtureSource>(
            new (std::nothrow) SyntheticFixtureSource(std::move(bytes), metadata));
        if (!source) {
            return AdapterStatus::error(AdapterStatusCode::BudgetExceeded, "synthetic source allocation failed");
        }
        if (source->residentBytesUpperBound() > request.maxResidentBytes) {
            return AdapterStatus::error(AdapterStatusCode::BudgetExceeded, "synthetic source exceeds memory budget");
        }

        outDescriptor.format = RawFormatFamily::SyntheticConformanceFixture;
        outDescriptor.storageRepresentation = StorageRepresentation::SyntheticCfa;
        outDescriptor.sampleTopology = SampleTopologyFamily::Bayer2x2;
        outDescriptor.processingLineage = ProcessingLineageClass::SyntheticConformanceOnly;
        outDescriptor.decoderId = "truthraw.synthetic-proprietary-conformance.v0.1";
        outDescriptor.sourceEvidenceId = request.sourceSeal.sourceEvidenceId;
        outDescriptor.sourceSealAcceptedAtBoundary = true;
        outDescriptor.exactCfaSamplesAvailable = true;
        outDescriptor.scientificColorBindingProvided = true;
        outDescriptor.measurementAdmissionReady = true;
        outDescriptor.scientificAdmissionReady = true;
        outDescriptor.syntheticConformanceOnly = true;
        outDescriptor.physicalExposureCountKnown = false;
        outDescriptor.physicalFrameCount = 0u;
        outDescriptor.singleExposureCertified = false;
        outDescriptor.storedSampleSenselSemanticsCertified = false;
        outDescriptor.requiresTopologySpecificSolver = false;
        outDescriptor.directSensorAdcClaimAllowed = false;
        outDescriptor.fullRawFrameMaterialized = false;

        outSource = std::move(source);
        return AdapterStatus::ok();
    }
};

} // namespace

AdapterStatus RawSourceAdapterRegistry::registerAdapter(
    std::shared_ptr<IRawSourceAdapter> adapter) noexcept {
    if (!adapter) {
        return AdapterStatus::error(AdapterStatusCode::InvalidArgument, "null RAW source adapter");
    }
    if (hasAdapter(adapter->formatFamily())) {
        return AdapterStatus::error(AdapterStatusCode::DuplicateAdapter, "adapter already registered");
    }
    try {
        adapters_.push_back(std::move(adapter));
    } catch (...) {
        return AdapterStatus::error(AdapterStatusCode::BudgetExceeded, "adapter registry allocation failed");
    }
    return AdapterStatus::ok();
}

AdapterStatus RawSourceAdapterRegistry::open(
    std::shared_ptr<IRawByteSource> bytes,
    const RawSourceOpenRequest& request,
    std::unique_ptr<IRawTileSource>& outSource,
    RawSourceDescriptor& outDescriptor) const noexcept {
    outSource.reset();
    outDescriptor = {};
    const auto it = std::find_if(
        adapters_.begin(),
        adapters_.end(),
        [&](const auto& adapter) { return adapter && adapter->formatFamily() == request.declaredFormat; });
    if (it == adapters_.end()) {
        return AdapterStatus::error(
            AdapterStatusCode::AdapterUnavailable,
            std::string("no decoder adapter registered for ") + rawFormatName(request.declaredFormat));
    }
    return (*it)->open(std::move(bytes), request, outSource, outDescriptor);
}

bool RawSourceAdapterRegistry::hasAdapter(RawFormatFamily format) const noexcept {
    return std::any_of(
        adapters_.begin(),
        adapters_.end(),
        [&](const auto& adapter) { return adapter && adapter->formatFamily() == format; });
}

std::shared_ptr<IRawSourceAdapter> makeDngAdapter() {
    return std::make_shared<DngAdapter>();
}

std::shared_ptr<IRawSourceAdapter> makeSyntheticConformanceFixtureAdapter() {
    return std::make_shared<SyntheticFixtureAdapter>();
}

const char* rawFormatName(RawFormatFamily format) noexcept {
    switch (format) {
        case RawFormatFamily::Dng: return "DNG";
        case RawFormatFamily::CanonCr3: return "CANON_CR3";
        case RawFormatFamily::CanonCr2: return "CANON_CR2";
        case RawFormatFamily::NikonNef: return "NIKON_NEF";
        case RawFormatFamily::NikonNrw: return "NIKON_NRW";
        case RawFormatFamily::SonyArw: return "SONY_ARW";
        case RawFormatFamily::FujifilmRaf: return "FUJIFILM_RAF";
        case RawFormatFamily::PanasonicRw2: return "PANASONIC_RW2";
        case RawFormatFamily::OlympusOrf: return "OLYMPUS_ORF";
        case RawFormatFamily::PentaxPef: return "PENTAX_PEF";
        case RawFormatFamily::LeicaRwl: return "LEICA_RWL";
        case RawFormatFamily::Hasselblad3fr: return "HASSELBLAD_3FR";
        case RawFormatFamily::HasselbladFff: return "HASSELBLAD_FFF";
        case RawFormatFamily::PhaseOneIiq: return "PHASE_ONE_IIQ";
        case RawFormatFamily::SigmaX3f: return "SIGMA_X3F";
        case RawFormatFamily::SamsungSrw: return "SAMSUNG_SRW";
        case RawFormatFamily::EpsonErf: return "EPSON_ERF";
        case RawFormatFamily::KodakDcrKdc: return "KODAK_DCR_KDC";
        case RawFormatFamily::MinoltaMrw: return "MINOLTA_MRW";
        case RawFormatFamily::MamiyaMef: return "MAMIYA_MEF";
        case RawFormatFamily::GenericRaw: return "GENERIC_RAW";
        case RawFormatFamily::SyntheticConformanceFixture: return "SYNTHETIC_CONFORMANCE_FIXTURE";
    }
    return "UNKNOWN_RAW_FORMAT";
}

const char* adapterStatusName(AdapterStatusCode code) noexcept {
    switch (code) {
        case AdapterStatusCode::Ok: return "OK";
        case AdapterStatusCode::InvalidArgument: return "INVALID_ARGUMENT";
        case AdapterStatusCode::SourceSealMismatch: return "SOURCE_SEAL_MISMATCH";
        case AdapterStatusCode::AdapterUnavailable: return "ADAPTER_UNAVAILABLE";
        case AdapterStatusCode::DuplicateAdapter: return "DUPLICATE_ADAPTER";
        case AdapterStatusCode::InvalidContainer: return "INVALID_CONTAINER";
        case AdapterStatusCode::UnsupportedContainerFeature: return "UNSUPPORTED_CONTAINER_FEATURE";
        case AdapterStatusCode::DecodeFailed: return "DECODE_FAILED";
        case AdapterStatusCode::BudgetExceeded: return "BUDGET_EXCEEDED";
    }
    return "UNKNOWN";
}

} // namespace truthraw::multivendor_raw_source_adapter::v0_1
