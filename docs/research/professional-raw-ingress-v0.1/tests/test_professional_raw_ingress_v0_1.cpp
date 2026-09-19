#include "professional_raw_ingress_v0_1.h"

#include <cstdlib>
#include <iostream>

using namespace truthraw::professional_raw_ingress::v0_1;

namespace {
DecoderResourceProfile tile_profile() {
    return {DecoderMemoryMode::RandomAccessTile, 8u * 1024u * 1024u, 4u * 1024u * 1024u, true, false};
}

DecoderResourceProfile full_profile() {
    return {DecoderMemoryMode::FullFrameMaterialized, 512u * 1024u * 1024u, 32u * 1024u * 1024u, false, true};
}

DecoderProvenance verified_provenance() {
    return {"fixture", "0.1", "deadbeef", "Example", "Camera", "lossless", 14, true};
}

void require(bool condition, const char* label) {
    if (!condition) {
        std::cerr << "REQUIRE_FAILED " << label << '\n';
        std::exit(2);
    }
}
}

int main() {
    {
        IngressDescriptor d{};
        const auto r = classify(d);
        require(r.admission == ScientificAdmission::Blocked, "unknown_blocked");
        require(r.evidenceClass == EvidenceClass::FailClosedUnsupported, "unknown_fail_closed");
    }
    {
        IngressDescriptor d{};
        d.container = ContainerFamily::Dng;
        d.topology = MeasurementTopology::Bayer2x2;
        d.compression = CompressionSemantics::Uncompressed;
        d.decodeCertification = DecodeCertification::NativeCertified;
        d.resources = tile_profile();
        d.provenance = verified_provenance();
        d.singlePhysicalFrameVerified = true;
        d.singleIndependentEvidenceVerified = true;
        d.downstreamTopologyCertified = true;
        const auto r = classify(d);
        require(r.admission == ScientificAdmission::SingleFrameDirectCfa, "native_dng_admission");
        require(r.evidenceClass == EvidenceClass::DirectNativeCertified, "native_dng_class");
        require(r.mayEnterSingleFrameScientificMaster, "native_dng_master");
    }
    {
        IngressDescriptor d{};
        d.container = ContainerFamily::CanonCr3;
        d.topology = MeasurementTopology::Bayer2x2;
        d.compression = CompressionSemantics::LosslessVerified;
        d.decodeCertification = DecodeCertification::AdapterCertified;
        d.resources = full_profile();
        d.provenance = verified_provenance();
        d.singlePhysicalFrameVerified = true;
        d.singleIndependentEvidenceVerified = true;
        d.downstreamTopologyCertified = true;
        const auto r = classify(d);
        require(r.admission == ScientificAdmission::SingleFrameDirectCfa, "external_lossless_admission");
        require(r.evidenceClass == EvidenceClass::LosslessDecodedCertified, "external_lossless_class");
    }
    {
        IngressDescriptor d{};
        d.container = ContainerFamily::FujifilmRaf;
        d.topology = MeasurementTopology::XTrans6x6;
        d.compression = CompressionSemantics::LosslessVerified;
        d.decodeCertification = DecodeCertification::AdapterCertified;
        d.resources = full_profile();
        d.provenance = verified_provenance();
        d.singlePhysicalFrameVerified = true;
        d.singleIndependentEvidenceVerified = true;
        d.downstreamTopologyCertified = false;
        const auto r = classify(d);
        require(r.admission == ScientificAdmission::ResearchOnly, "xtrans_research_only");
        require(!r.mayEnterSingleFrameScientificMaster, "xtrans_no_master");
    }
    {
        IngressDescriptor d{};
        d.container = ContainerFamily::Dng;
        d.topology = MeasurementTopology::ComputationalRaw;
        d.compression = CompressionSemantics::LosslessVerified;
        d.decodeCertification = DecodeCertification::AdapterCertified;
        d.resources = tile_profile();
        d.provenance = verified_provenance();
        const auto r = classify(d);
        require(r.evidenceClass == EvidenceClass::ComputationalRaw, "computational_class");
        require(r.requiresDerivedOrCounterfactualBoundary, "computational_boundary");
        require(!r.mayEnterSingleFrameScientificMaster, "computational_no_master");
    }
    {
        IngressDescriptor d{};
        d.container = ContainerFamily::OlympusOrfOri;
        d.topology = MeasurementTopology::MultiShotComposite;
        d.compression = CompressionSemantics::LosslessVerified;
        d.decodeCertification = DecodeCertification::AdapterCertified;
        d.resources = full_profile();
        d.provenance = verified_provenance();
        const auto r = classify(d);
        require(r.evidenceClass == EvidenceClass::MultiCaptureRaw, "multishot_class");
        require(!r.mayEnterSingleFrameScientificMaster, "multishot_no_master");
    }
    {
        IngressDescriptor d{};
        d.container = ContainerFamily::NikonNefNrw;
        d.topology = MeasurementTopology::Bayer2x2;
        d.compression = CompressionSemantics::LosslessVerified;
        d.decodeCertification = DecodeCertification::AdapterCertified;
        d.resources = full_profile();
        d.provenance = verified_provenance();
        d.provenance.decoderBytePathVerified = false;
        d.singlePhysicalFrameVerified = true;
        d.singleIndependentEvidenceVerified = true;
        d.downstreamTopologyCertified = true;
        const auto r = classify(d);
        require(r.evidenceClass == EvidenceClass::ResearchOnly, "unverified_decoder_research_only");
    }
    {
        const DecoderResourceProfile impossible{DecoderMemoryMode::FullFrameMaterialized, 1, 1, true, true};
        require(!valid(impossible), "impossible_resource_profile_rejected");
    }

    std::cout << "PROFESSIONAL_RAW_INGRESS_V0_1_TEST_PASS\n";
    std::cout << "extensionAloneCertifies=0\n";
    std::cout << "nativeDngDirectCfa=1\n";
    std::cout << "certifiedExternalLossless=1\n";
    std::cout << "xtransWithoutDownstreamTopology=research_only\n";
    std::cout << "computationalRawDirectCfa=0\n";
    std::cout << "multiShotDirectCfa=0\n";
    return 0;
}
