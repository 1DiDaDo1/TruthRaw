#include "professional_raw_ingress_v0_1.h"

#include <cassert>
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
}

int main() {
    {
        IngressDescriptor d{};
        auto r = classify(d);
        assert(r.admission == ScientificAdmission::Blocked);
        assert(r.evidenceClass == EvidenceClass::FailClosedUnsupported);
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
        auto r = classify(d);
        assert(r.admission == ScientificAdmission::SingleFrameDirectCfa);
        assert(r.evidenceClass == EvidenceClass::DirectNativeCertified);
        assert(r.mayEnterSingleFrameScientificMaster);
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
        auto r = classify(d);
        assert(r.admission == ScientificAdmission::SingleFrameDirectCfa);
        assert(r.evidenceClass == EvidenceClass::LosslessDecodedCertified);
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
        auto r = classify(d);
        assert(r.admission == ScientificAdmission::ResearchOnly);
        assert(!r.mayEnterSingleFrameScientificMaster);
    }
    {
        IngressDescriptor d{};
        d.container = ContainerFamily::Dng;
        d.topology = MeasurementTopology::ComputationalRaw;
        d.compression = CompressionSemantics::LosslessVerified;
        d.decodeCertification = DecodeCertification::AdapterCertified;
        d.resources = tile_profile();
        d.provenance = verified_provenance();
        auto r = classify(d);
        assert(r.evidenceClass == EvidenceClass::ComputationalRaw);
        assert(r.requiresDerivedOrCounterfactualBoundary);
        assert(!r.mayEnterSingleFrameScientificMaster);
    }
    {
        IngressDescriptor d{};
        d.container = ContainerFamily::OlympusOrfOri;
        d.topology = MeasurementTopology::MultiShotComposite;
        d.compression = CompressionSemantics::LosslessVerified;
        d.decodeCertification = DecodeCertification::AdapterCertified;
        d.resources = full_profile();
        d.provenance = verified_provenance();
        auto r = classify(d);
        assert(r.evidenceClass == EvidenceClass::MultiCaptureRaw);
        assert(!r.mayEnterSingleFrameScientificMaster);
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
        auto r = classify(d);
        assert(r.evidenceClass == EvidenceClass::ResearchOnly);
    }
    {
        DecoderResourceProfile impossible{DecoderMemoryMode::FullFrameMaterialized, 1, 1, true, true};
        assert(!valid(impossible));
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
