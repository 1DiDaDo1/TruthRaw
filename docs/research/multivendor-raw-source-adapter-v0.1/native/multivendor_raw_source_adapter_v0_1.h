#pragma once

#include "full_frame_streaming_v0_1.h"

#include <array>
#include <cstddef>
#include <cstdint>
#include <memory>
#include <string>
#include <vector>

namespace truthraw::multivendor_raw_source_adapter::v0_1 {

using Hash256 = std::array<std::uint8_t, 32>;

enum class RawFormatFamily : std::uint8_t {
    Dng = 0,
    CanonCr3,
    CanonCr2,
    NikonNef,
    NikonNrw,
    SonyArw,
    FujifilmRaf,
    PanasonicRw2,
    OlympusOrf,
    PentaxPef,
    LeicaRwl,
    Hasselblad3fr,
    HasselbladFff,
    PhaseOneIiq,
    SigmaX3f,
    SamsungSrw,
    EpsonErf,
    KodakDcrKdc,
    MinoltaMrw,
    MamiyaMef,
    GenericRaw,
    SyntheticConformanceFixture,
};

enum class AdapterStatusCode : std::uint8_t {
    Ok = 0,
    InvalidArgument,
    SourceSealMismatch,
    AdapterUnavailable,
    DuplicateAdapter,
    InvalidContainer,
    UnsupportedContainerFeature,
    DecodeFailed,
    BudgetExceeded,
};

struct AdapterStatus final {
    AdapterStatusCode code = AdapterStatusCode::Ok;
    std::string message;
    explicit operator bool() const noexcept { return code == AdapterStatusCode::Ok; }

    static AdapterStatus ok() { return {}; }
    static AdapterStatus error(AdapterStatusCode code, std::string message) {
        AdapterStatus out;
        out.code = code;
        out.message = std::move(message);
        return out;
    }
};

// Vendor-neutral random-access transport. Source sealing happens before adapter
// selection. Adapters may read source bytes but may not mutate them.
class IRawByteSource {
public:
    virtual ~IRawByteSource() = default;
    virtual std::uint64_t sizeBytes() const noexcept = 0;
    virtual std::size_t residentBytesUpperBound() const noexcept = 0;
    virtual bool readExact(std::uint64_t offset, void* dst, std::size_t count) noexcept = 0;
};

struct RawSourceSeal final {
    bool valid = false;
    Hash256 sha256{};
    std::uint64_t byteLength = 0;
    std::string sourceEvidenceId;
};

struct RawColorBinding final {
    bool valid = false;
    std::string bindingId;
    std::array<float, 9> cameraToXyzD50 = {1,0,0, 0,1,0, 0,0,1};
};

struct RawSourceOpenRequest final {
    RawFormatFamily declaredFormat = RawFormatFamily::GenericRaw;
    RawSourceSeal sourceSeal;
    RawColorBinding color;
    std::size_t maxResidentBytes = 8u * 1024u * 1024u;
};

struct RawSourceDescriptor final {
    RawFormatFamily format = RawFormatFamily::GenericRaw;
    std::string decoderId;
    std::string sourceEvidenceId;

    bool sourceSealAcceptedAtBoundary = false;
    bool exactCfaSamplesAvailable = false;
    bool scientificColorBindingProvided = false;
    bool syntheticConformanceOnly = false;

    // Container/sample decoding alone never proves untouched ADC provenance.
    bool directSensorAdcClaimAllowed = false;
    bool fullRawFrameMaterialized = false;
};

class IRawSourceAdapter {
public:
    virtual ~IRawSourceAdapter() = default;
    virtual RawFormatFamily formatFamily() const noexcept = 0;

    virtual AdapterStatus open(
        std::shared_ptr<IRawByteSource> bytes,
        const RawSourceOpenRequest& request,
        std::unique_ptr<streaming_v0_1::IRawTileSource>& outSource,
        RawSourceDescriptor& outDescriptor) noexcept = 0;
};

class RawSourceAdapterRegistry final {
public:
    AdapterStatus registerAdapter(std::shared_ptr<IRawSourceAdapter> adapter) noexcept;

    AdapterStatus open(
        std::shared_ptr<IRawByteSource> bytes,
        const RawSourceOpenRequest& request,
        std::unique_ptr<streaming_v0_1::IRawTileSource>& outSource,
        RawSourceDescriptor& outDescriptor) const noexcept;

    bool hasAdapter(RawFormatFamily format) const noexcept;

private:
    std::vector<std::shared_ptr<IRawSourceAdapter>> adapters_;
};

std::shared_ptr<IRawSourceAdapter> makeDngAdapter();

// Test-only ABI fixture. It proves that a non-DNG adapter can populate the same
// IRawTileSource contract. It must never be promoted as real vendor support.
std::shared_ptr<IRawSourceAdapter> makeSyntheticConformanceFixtureAdapter();

const char* rawFormatName(RawFormatFamily format) noexcept;
const char* adapterStatusName(AdapterStatusCode code) noexcept;

} // namespace truthraw::multivendor_raw_source_adapter::v0_1
