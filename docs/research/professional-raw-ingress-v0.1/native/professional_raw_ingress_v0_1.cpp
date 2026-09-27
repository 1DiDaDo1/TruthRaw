#include "professional_raw_ingress_v0_1.h"

namespace truthraw::professional_raw_ingress::v0_1 {

bool valid(const DecoderResourceProfile& profile) noexcept {
    if (profile.memoryMode == DecoderMemoryMode::Unknown) return false;
    if (profile.requiresFullFrameMaterialization &&
        profile.memoryMode != DecoderMemoryMode::FullFrameMaterialized) return false;
    if (profile.supportsRandomAccessTiles &&
        profile.memoryMode == DecoderMemoryMode::FullFrameMaterialized) return false;
    if ((profile.memoryMode == DecoderMemoryMode::RandomAccessTile ||
         profile.memoryMode == DecoderMemoryMode::StorageUnitBounded) &&
        profile.residentUpperBoundBytes == 0) return false;
    return true;
}

IngressDecision classify(const IngressDescriptor& input) noexcept {
    IngressDecision out{};

    if (input.container == ContainerFamily::Unknown ||
        input.topology == MeasurementTopology::Unknown ||
        input.compression == CompressionSemantics::Unknown ||
        input.decodeCertification == DecodeCertification::Unsupported ||
        !valid(input.resources)) {
        return out;
    }

    if (input.topology == MeasurementTopology::ComputationalRaw) {
        out.admission = ScientificAdmission::DerivedMeasurement;
        out.evidenceClass = EvidenceClass::ComputationalRaw;
        out.requiresDerivedOrCounterfactualBoundary = true;
        return out;
    }

    if (input.topology == MeasurementTopology::MultiShotComposite) {
        out.admission = ScientificAdmission::DerivedMeasurement;
        out.evidenceClass = EvidenceClass::MultiCaptureRaw;
        out.requiresDerivedOrCounterfactualBoundary = true;
        return out;
    }

    if (input.decodeCertification == DecodeCertification::DecoderAvailableUncertified ||
        !input.provenance.decoderBytePathVerified) {
        out.admission = ScientificAdmission::ResearchOnly;
        out.evidenceClass = EvidenceClass::ResearchOnly;
        return out;
    }

    if (!input.downstreamTopologyCertified) {
        out.admission = ScientificAdmission::ResearchOnly;
        out.evidenceClass = EvidenceClass::ResearchOnly;
        return out;
    }

    const bool singleEvidence = input.singlePhysicalFrameVerified &&
                                input.singleIndependentEvidenceVerified;
    if (!singleEvidence) {
        out.admission = ScientificAdmission::ResearchOnly;
        out.evidenceClass = EvidenceClass::ResearchOnly;
        return out;
    }

    if (input.topology == MeasurementTopology::Bayer2x2 &&
        (input.compression == CompressionSemantics::Uncompressed ||
         input.compression == CompressionSemantics::LosslessVerified)) {
        out.admission = ScientificAdmission::SingleFrameDirectCfa;
        out.mayEnterSingleFrameScientificMaster = true;
        out.evidenceClass =
            input.decodeCertification == DecodeCertification::NativeCertified
                ? EvidenceClass::DirectNativeCertified
                : EvidenceClass::LosslessDecodedCertified;
        return out;
    }

    if (input.compression == CompressionSemantics::Lossy ||
        input.compression == CompressionSemantics::VendorOpaque) {
        out.admission = ScientificAdmission::DerivedMeasurement;
        out.evidenceClass = EvidenceClass::DerivedRawSupported;
        out.requiresDerivedOrCounterfactualBoundary = true;
        return out;
    }

    out.admission = ScientificAdmission::ResearchOnly;
    out.evidenceClass = EvidenceClass::ResearchOnly;
    return out;
}

}  // namespace truthraw::professional_raw_ingress::v0_1
