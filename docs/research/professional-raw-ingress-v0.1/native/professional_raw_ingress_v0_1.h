#pragma once

#include <cstddef>
#include <cstdint>
#include <string_view>

namespace truthraw::professional_raw_ingress::v0_1 {

enum class ContainerFamily : std::uint8_t {
    Unknown = 0,
    Dng,
    CanonCr2,
    CanonCr3,
    NikonNefNrw,
    SonyArwSrfSr2,
    FujifilmRaf,
    PanasonicRw2,
    OlympusOrfOri,
    PentaxPef,
    Hasselblad3frFff,
    PhaseOneIiq,
    LeafMosMef,
    SigmaX3f,
    CinemaRaw,
};

enum class MeasurementTopology : std::uint8_t {
    Unknown = 0,
    Bayer2x2,
    XTrans6x6,
    Monochrome,
    LayeredFoveon,
    LinearRgb,
    MultiShotComposite,
    ComputationalRaw,
};

enum class CompressionSemantics : std::uint8_t {
    Unknown = 0,
    Uncompressed,
    LosslessVerified,
    Lossy,
    VendorOpaque,
};

enum class DecodeCertification : std::uint8_t {
    Unsupported = 0,
    DecoderAvailableUncertified,
    AdapterCertified,
    NativeCertified,
};

enum class ScientificAdmission : std::uint8_t {
    Blocked = 0,
    ResearchOnly,
    DerivedMeasurement,
    SingleFrameDirectCfa,
};

enum class EvidenceClass : std::uint8_t {
    FailClosedUnsupported = 0,
    ResearchOnly,
    DerivedRawSupported,
    ComputationalRaw,
    MultiCaptureRaw,
    LosslessDecodedCertified,
    DirectNativeCertified,
};

enum class DecoderMemoryMode : std::uint8_t {
    Unknown = 0,
    RandomAccessTile,
    StorageUnitBounded,
    FullFrameMaterialized,
};

struct DecoderResourceProfile {
    DecoderMemoryMode memoryMode = DecoderMemoryMode::Unknown;
    std::uint64_t residentUpperBoundBytes = 0;
    std::uint64_t scratchUpperBoundBytes = 0;
    bool supportsRandomAccessTiles = false;
    bool requiresFullFrameMaterialization = false;
};

struct DecoderProvenance {
    std::string_view adapterId;
    std::string_view adapterVersion;
    std::string_view adapterBuildHash;
    std::string_view cameraMake;
    std::string_view cameraModel;
    std::string_view codecVariant;
    std::uint16_t sourceBitDepth = 0;
    bool decoderBytePathVerified = false;
};

struct IngressDescriptor {
    ContainerFamily container = ContainerFamily::Unknown;
    MeasurementTopology topology = MeasurementTopology::Unknown;
    CompressionSemantics compression = CompressionSemantics::Unknown;
    DecodeCertification decodeCertification = DecodeCertification::Unsupported;
    DecoderResourceProfile resources{};
    DecoderProvenance provenance{};
    bool singlePhysicalFrameVerified = false;
    bool singleIndependentEvidenceVerified = false;
    bool downstreamTopologyCertified = false;
};

struct IngressDecision {
    ScientificAdmission admission = ScientificAdmission::Blocked;
    EvidenceClass evidenceClass = EvidenceClass::FailClosedUnsupported;
    bool sourceMustRemainSealed = true;
    bool mayEnterSingleFrameScientificMaster = false;
    bool requiresDerivedOrCounterfactualBoundary = false;
};

[[nodiscard]] bool valid(const DecoderResourceProfile& profile) noexcept;
[[nodiscard]] IngressDecision classify(const IngressDescriptor& input) noexcept;

}  // namespace truthraw::professional_raw_ingress::v0_1
