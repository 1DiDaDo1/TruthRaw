#include "professional_raw_decoder_adapter_v0_1.h"

#include <cstdlib>
#include <iostream>

using namespace truthraw::professional_raw_decoder_adapter::v0_1;
namespace ingress = truthraw::professional_raw_ingress::v0_1;

namespace {
void require(bool condition, const char* message) {
    if (!condition) {
        std::cerr << "FAIL: " << message << "\n";
        std::exit(2);
    }
}

ingress::DecoderResourceProfile random_access(std::uint64_t resident,
                                               std::uint64_t scratch = 0) {
    ingress::DecoderResourceProfile p{};
    p.memoryMode = ingress::DecoderMemoryMode::RandomAccessTile;
    p.residentUpperBoundBytes = resident;
    p.scratchUpperBoundBytes = scratch;
    p.supportsRandomAccessTiles = true;
    p.requiresFullFrameMaterialization = false;
    return p;
}

ingress::DecoderResourceProfile full_frame(std::uint64_t resident,
                                            std::uint64_t scratch = 0) {
    ingress::DecoderResourceProfile p{};
    p.memoryMode = ingress::DecoderMemoryMode::FullFrameMaterialized;
    p.residentUpperBoundBytes = resident;
    p.scratchUpperBoundBytes = scratch;
    p.supportsRandomAccessTiles = false;
    p.requiresFullFrameMaterialization = true;
    return p;
}

ingress::DecoderProvenance provenance(const char* id,
                                      const char* codec,
                                      bool verified) {
    ingress::DecoderProvenance p{};
    p.adapterId = id;
    p.adapterVersion = "0.1";
    p.adapterBuildHash = "0123456789abcdef";
    p.cameraMake = "fixture";
    p.cameraModel = "fixture";
    p.codecVariant = codec;
    p.sourceBitDepth = 14;
    p.decoderBytePathVerified = verified;
    return p;
}

AdapterEvidenceBinding sealed_equivalent() {
    AdapterEvidenceBinding b{};
    b.originalSourceSealed = true;
    b.sourceEvidenceBindingVerified = true;
    b.decodedOutputBoundToSource = true;
    b.sampleEquivalenceVerified = true;
    return b;
}
}

int main() {
    const auto cr3 = describe_external_decoder(
        ingress::ContainerFamily::CanonCr3,
        ingress::MeasurementTopology::Bayer2x2,
        ingress::CompressionSemantics::LosslessVerified,
        ingress::DecodeCertification::AdapterCertified,
        random_access(8u * 1024u * 1024u, 2u * 1024u * 1024u),
        provenance("fixture-cr3", "lossless-cr3-fixture", true),
        SampleSemantics::LosslessDecodedEquivalentSamples,
        sealed_equivalent(), true, true, true);
    const auto cr3_decision = classify_adapter_output(cr3);
    require(cr3_decision.admission ==
                ingress::ScientificAdmission::SingleFrameDirectCfa,
            "verified lossless CR3 should enter Direct-CFA");
    require(cr3_decision.evidenceClass ==
                ingress::EvidenceClass::LosslessDecodedCertified,
            "external lossless decode must not be labeled native");

    auto weak_binding = sealed_equivalent();
    weak_binding.sampleEquivalenceVerified = false;
    const auto weak = describe_external_decoder(
        ingress::ContainerFamily::CanonCr3,
        ingress::MeasurementTopology::Bayer2x2,
        ingress::CompressionSemantics::LosslessVerified,
        ingress::DecodeCertification::AdapterCertified,
        random_access(8u * 1024u * 1024u),
        provenance("fixture-cr3", "lossless-cr3-fixture", true),
        SampleSemantics::LosslessDecodedEquivalentSamples,
        weak_binding, true, true, true);
    require(classify_adapter_output(weak).admission ==
                ingress::ScientificAdmission::Blocked,
            "unproven sample equivalence must block");

    const auto xtrans = describe_external_decoder(
        ingress::ContainerFamily::FujifilmRaf,
        ingress::MeasurementTopology::XTrans6x6,
        ingress::CompressionSemantics::LosslessVerified,
        ingress::DecodeCertification::AdapterCertified,
        random_access(16u * 1024u * 1024u),
        provenance("fixture-raf", "raf-lossless", true),
        SampleSemantics::LosslessDecodedEquivalentSamples,
        sealed_equivalent(), true, true, false);
    require(classify_adapter_output(xtrans).admission ==
                ingress::ScientificAdmission::ResearchOnly,
            "X-Trans must not masquerade as Bayer");

    const auto comp = describe_external_decoder(
        ingress::ContainerFamily::Dng,
        ingress::MeasurementTopology::ComputationalRaw,
        ingress::CompressionSemantics::LosslessVerified,
        ingress::DecodeCertification::AdapterCertified,
        random_access(4u * 1024u * 1024u),
        provenance("fixture-computational", "dng-derived", true),
        SampleSemantics::DerivedSamples,
        sealed_equivalent(), true, true, false);
    const auto comp_decision = classify_adapter_output(comp);
    require(comp_decision.admission ==
                ingress::ScientificAdmission::DerivedMeasurement,
            "computational RAW must remain derived");
    require(!comp_decision.mayEnterSingleFrameScientificMaster,
            "derived samples cannot enter single-frame Direct-CFA master");

    const auto multi = describe_external_decoder(
        ingress::ContainerFamily::OlympusOrfOri,
        ingress::MeasurementTopology::MultiShotComposite,
        ingress::CompressionSemantics::LosslessVerified,
        ingress::DecodeCertification::AdapterCertified,
        random_access(12u * 1024u * 1024u),
        provenance("fixture-ori", "pixel-shift-composite", true),
        SampleSemantics::DerivedSamples,
        sealed_equivalent(), false, false, false);
    require(classify_adapter_output(multi).evidenceClass ==
                ingress::EvidenceClass::MultiCaptureRaw,
            "multi-shot composite must retain multi-capture class");

    const auto huge = describe_external_decoder(
        ingress::ContainerFamily::PhaseOneIiq,
        ingress::MeasurementTopology::Bayer2x2,
        ingress::CompressionSemantics::LosslessVerified,
        ingress::DecodeCertification::AdapterCertified,
        full_frame(384ull * 1024ull * 1024ull, 32ull * 1024ull * 1024ull),
        provenance("fixture-iiq", "iiq-lossless", true),
        SampleSemantics::LosslessDecodedEquivalentSamples,
        sealed_equivalent(), true, true, true);
    const auto huge_decision = classify_adapter_output(huge);
    require(huge_decision.admission ==
                ingress::ScientificAdmission::SingleFrameDirectCfa,
            "resource size must not change truth class");
    require(!admit_resources(huge.descriptor.resources,
                             64ull * 1024ull * 1024ull).admitted,
            "low-resource device must reject oversized decoder workspace");
    require(admit_resources(huge.descriptor.resources,
                            512ull * 1024ull * 1024ull).admitted,
            "larger resource envelope may admit same decoder");

    std::cout
        << "PROFESSIONAL_RAW_DECODER_ADAPTER_V0_1_TEST_PASS\n"
        << "cr3_direct_cfa=1\n"
        << "weak_equivalence_blocked=1\n"
        << "xtrans_not_bayer=1\n"
        << "computational_derived=1\n"
        << "multishot_separate=1\n"
        << "resource_truth_orthogonal=1\n";
    return 0;
}
