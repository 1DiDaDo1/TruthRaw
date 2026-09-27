#include "professional_raw_decoder_adapter_v0_1.h"

#include <limits>

namespace truthraw::professional_raw_decoder_adapter::v0_1 {

bool valid_identity(const AdapterIdentity& identity) noexcept {
    return !identity.id.empty() &&
           !identity.version.empty() &&
           !identity.buildHash.empty();
}

bool valid_evidence_binding(const AdapterOutput& output) noexcept {
    if (output.kind == AdapterKind::Unknown ||
        output.samples == SampleSemantics::Unknown) {
        return false;
    }
    if (!output.evidence.originalSourceSealed ||
        !output.evidence.sourceEvidenceBindingVerified ||
        !output.evidence.decodedOutputBoundToSource) {
        return false;
    }
    if (output.samples == SampleSemantics::LosslessDecodedEquivalentSamples &&
        !output.evidence.sampleEquivalenceVerified) {
        return false;
    }
    if (output.samples == SampleSemantics::StoredCodeValues &&
        output.kind != AdapterKind::NativeTileDng) {
        return false;
    }
    return true;
}

ingress::IngressDecision classify_adapter_output(
    const AdapterOutput& output) noexcept {
    if (!valid_evidence_binding(output)) {
        return {};
    }

    // Derived samples may be useful, but they are not Direct-CFA evidence.
    if (output.samples == SampleSemantics::DerivedSamples) {
        auto descriptor = output.descriptor;
        if (descriptor.topology != ingress::MeasurementTopology::ComputationalRaw &&
            descriptor.topology != ingress::MeasurementTopology::MultiShotComposite) {
            descriptor.compression = ingress::CompressionSemantics::Lossy;
        }
        auto decision = ingress::classify(descriptor);
        decision.mayEnterSingleFrameScientificMaster = false;
        decision.requiresDerivedOrCounterfactualBoundary = true;
        if (decision.admission == ingress::ScientificAdmission::SingleFrameDirectCfa) {
            decision.admission = ingress::ScientificAdmission::DerivedMeasurement;
            decision.evidenceClass = ingress::EvidenceClass::DerivedRawSupported;
        }
        return decision;
    }

    return ingress::classify(output.descriptor);
}

ResourceDecision admit_resources(
    const ingress::DecoderResourceProfile& resources,
    std::uint64_t availableBytes) noexcept {
    ResourceDecision out{};
    out.availableBytes = availableBytes;
    if (!ingress::valid(resources)) {
        return out;
    }

    if (resources.residentUpperBoundBytes >
        std::numeric_limits<std::uint64_t>::max() - resources.scratchUpperBoundBytes) {
        return out;
    }

    out.requiredUpperBoundBytes =
        resources.residentUpperBoundBytes + resources.scratchUpperBoundBytes;
    out.admitted = out.requiredUpperBoundBytes <= availableBytes;
    return out;
}

AdapterOutput describe_native_tile_dng(
    const tiledng::TileNativeDngSource& source,
    const AdapterIdentity& identity,
    bool sourceEvidenceBindingVerified,
    bool colorBindingAuthorityVerified) noexcept {
    AdapterOutput out{};
    out.kind = AdapterKind::NativeTileDng;
    out.samples = SampleSemantics::StoredCodeValues;

    const auto& audit = source.audit();
    ingress::DecoderResourceProfile resources{};
    resources.memoryMode = ingress::DecoderMemoryMode::RandomAccessTile;
    resources.residentUpperBoundBytes =
        static_cast<std::uint64_t>(source.residentBytesUpperBound());
    resources.scratchUpperBoundBytes = 0;
    resources.supportsRandomAccessTiles = true;
    resources.requiresFullFrameMaterialization = false;

    ingress::DecoderProvenance provenance{};
    provenance.adapterId = identity.id;
    provenance.adapterVersion = identity.version;
    provenance.adapterBuildHash = identity.buildHash;
    provenance.codecVariant = "strict-tiff-dng-v0.1";
    provenance.sourceBitDepth = 16;
    provenance.decoderBytePathVerified =
        valid_identity(identity) &&
        !audit.fullFileMaterialized &&
        !audit.fullRawMaterialized;

    out.descriptor.container = ingress::ContainerFamily::Dng;
    out.descriptor.topology = ingress::MeasurementTopology::Bayer2x2;
    out.descriptor.compression = ingress::CompressionSemantics::Uncompressed;
    out.descriptor.decodeCertification =
        provenance.decoderBytePathVerified && colorBindingAuthorityVerified
            ? ingress::DecodeCertification::NativeCertified
            : ingress::DecodeCertification::DecoderAvailableUncertified;
    out.descriptor.resources = resources;
    out.descriptor.provenance = provenance;
    out.descriptor.singlePhysicalFrameVerified = true;
    out.descriptor.singleIndependentEvidenceVerified = true;
    out.descriptor.downstreamTopologyCertified = colorBindingAuthorityVerified;

    out.evidence.originalSourceSealed = sourceEvidenceBindingVerified;
    out.evidence.sourceEvidenceBindingVerified = sourceEvidenceBindingVerified;
    out.evidence.decodedOutputBoundToSource = sourceEvidenceBindingVerified;
    out.evidence.sampleEquivalenceVerified = true;
    return out;
}

AdapterOutput describe_external_decoder(
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
    bool downstreamTopologyCertified) noexcept {
    AdapterOutput out{};
    out.kind = AdapterKind::ExternalDecoder;
    out.samples = samples;
    out.evidence = evidence;
    out.descriptor.container = container;
    out.descriptor.topology = topology;
    out.descriptor.compression = compression;
    out.descriptor.decodeCertification = certification;
    out.descriptor.resources = resources;
    out.descriptor.provenance = provenance;
    out.descriptor.singlePhysicalFrameVerified = singlePhysicalFrameVerified;
    out.descriptor.singleIndependentEvidenceVerified =
        singleIndependentEvidenceVerified;
    out.descriptor.downstreamTopologyCertified = downstreamTopologyCertified;
    return out;
}

} // namespace truthraw::professional_raw_decoder_adapter::v0_1
