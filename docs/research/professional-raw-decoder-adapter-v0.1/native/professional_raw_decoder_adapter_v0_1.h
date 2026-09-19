#pragma once

#include "professional_raw_ingress_v0_1.h"
#include "tile_native_dng_source_v0_1.h"

#include <cstddef>
#include <cstdint>
#include <string_view>

namespace truthraw::professional_raw_decoder_adapter::v0_1 {

namespace ingress = truthraw::professional_raw_ingress::v0_1;
namespace tiledng = truthraw::tile_dng_v0_1;

enum class AdapterKind : std::uint8_t {
    Unknown = 0,
    NativeTileDng,
    ExternalDecoder,
};

enum class SampleSemantics : std::uint8_t {
    Unknown = 0,
    StoredCodeValues,
    LosslessDecodedEquivalentSamples,
    DerivedSamples,
};

struct AdapterIdentity {
    std::string_view id;
    std::string_view version;
    std::string_view buildHash;
};

struct AdapterEvidenceBinding {
    bool originalSourceSealed = false;
    bool sourceEvidenceBindingVerified = false;
    bool decodedOutputBoundToSource = false;
    bool sampleEquivalenceVerified = false;
};

struct AdapterOutput {
    AdapterKind kind = AdapterKind::Unknown;
    SampleSemantics samples = SampleSemantics::Unknown;
    ingress::IngressDescriptor descriptor{};
    AdapterEvidenceBinding evidence{};
};

struct ResourceDecision {
    bool admitted = false;
    std::uint64_t requiredUpperBoundBytes = 0;
    std::uint64_t availableBytes = 0;
};

[[nodiscard]] bool valid_identity(const AdapterIdentity& identity) noexcept;
[[nodiscard]] bool valid_evidence_binding(
    const AdapterOutput& output) noexcept;

[[nodiscard]] ingress::IngressDecision classify_adapter_output(
    const AdapterOutput& output) noexcept;

[[nodiscard]] ResourceDecision admit_resources(
    const ingress::DecoderResourceProfile& resources,
    std::uint64_t availableBytes) noexcept;

// Native adapter #1: bridge the already-opened strict TileNativeDngSource
// into the format-neutral Professional RAW Ingress vocabulary.
// The Archivist remains responsible for the sourceEvidence binding boolean.
[[nodiscard]] AdapterOutput describe_native_tile_dng(
    const tiledng::TileNativeDngSource& source,
    const AdapterIdentity& identity,
    bool sourceEvidenceBindingVerified,
    bool colorBindingAuthorityVerified) noexcept;

// Test/adapter-author helper. External codec implementations (CR3/NEF/ARW/RAF/
// IIQ/etc.) must populate all fields explicitly; file extension is never used
// as evidence.
[[nodiscard]] AdapterOutput describe_external_decoder(
    ingress::ContainerFamily container,
    ingress::MeasurementTopology topology,
    ingress::CompressionSemantics compression,
    ingress::DecodeCertification certification,
    const ingress::DecoderResourceProfile& resources,
    const ingress::DecoderProvenance& provenance,
    SampleSemantics samples,
    const AdapterEvidenceBinding& evidence,
    bool singlePhysicalFrameVerified,
    bool singleIndependentEvidenceVerified,
    bool downstreamTopologyCertified) noexcept;

} // namespace truthraw::professional_raw_decoder_adapter::v0_1
