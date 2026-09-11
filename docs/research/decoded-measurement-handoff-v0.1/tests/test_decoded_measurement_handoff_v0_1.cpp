#include "decoded_measurement_handoff_v0_1.h"
#include "professional_raw_gatehouse_runtime_v0_1.h"
#include "professional_raw_decoder_adapter_v0_1.h"

#include <algorithm>
#include <array>
#include <cstdint>
#include <iostream>
#include <span>
#include <unistd.h>
#include <vector>

namespace handoff = truthraw::decoded_measurement_handoff::v0_1;
namespace gate = truthraw::professional_raw_gatehouse::v0_1;
namespace adapter = truthraw::professional_raw_decoder_adapter::v0_1;
namespace ingress = truthraw::professional_raw_ingress::v0_1;

#define CHECK_TRUE(expr) do { \
    if (!(expr)) { \
        std::cerr << "CHECK failed at line " << __LINE__ << ": " #expr "\n"; \
        return __LINE__; \
    } \
} while (false)

struct FdGuard {
    int fd = -1;
    ~FdGuard() { if (fd >= 0) ::close(fd); }
};

static handoff::Hash256 make_hash(std::uint8_t seed) {
    handoff::Hash256 out{};
    for (std::size_t i = 0; i < out.size(); ++i) {
        out[i] = static_cast<std::uint8_t>(seed + static_cast<std::uint8_t>(i));
    }
    return out;
}

static adapter::AdapterOutput make_external_fixture() {
    ingress::DecoderResourceProfile resources{};
    resources.memoryMode = ingress::DecoderMemoryMode::FullFrameMaterialized;
    resources.residentUpperBoundBytes = 8ULL * 1024ULL * 1024ULL;
    resources.scratchUpperBoundBytes = 1ULL * 1024ULL * 1024ULL;
    resources.requiresFullFrameMaterialization = true;

    ingress::DecoderProvenance provenance{};
    provenance.adapterId = "fixture.lossless.decoder";
    provenance.adapterVersion = "1";
    provenance.adapterBuildHash = "fixture-build";
    provenance.cameraMake = "Fixture";
    provenance.cameraModel = "Pro";
    provenance.codecVariant = "lossless";
    provenance.sourceBitDepth = 14;
    provenance.decoderBytePathVerified = true;

    adapter::AdapterEvidenceBinding evidence{};
    evidence.originalSourceSealed = true;
    evidence.sourceEvidenceBindingVerified = true;
    evidence.decodedOutputBoundToSource = true;
    evidence.sampleEquivalenceVerified = true;

    return adapter::describe_external_decoder(
        ingress::ContainerFamily::CanonCr3,
        ingress::MeasurementTopology::Bayer2x2,
        ingress::CompressionSemantics::LosslessVerified,
        ingress::DecodeCertification::AdapterCertified,
        resources,
        provenance,
        adapter::SampleSemantics::LosslessDecodedEquivalentSamples,
        evidence,
        true,
        true,
        true);
}

int main() {
    char path[] = "/tmp/truthraw_handoff_XXXXXX";
    FdGuard store{::mkstemp(path)};
    CHECK_TRUE(store.fd >= 0);
    CHECK_TRUE(::unlink(path) == 0); // anonymous temporary backing file for the test

    handoff::Descriptor descriptor{};
    descriptor.width = 17;
    descriptor.height = 13;
    descriptor.sourceBitDepth = 14;
    descriptor.topology = ingress::MeasurementTopology::Bayer2x2;
    descriptor.evidenceClass = ingress::EvidenceClass::LosslessDecodedCertified;
    descriptor.sourceCompression = ingress::CompressionSemantics::LosslessVerified;
    descriptor.cfa2x2 = {
        handoff::CfaColor::Red,
        handoff::CfaColor::Green,
        handoff::CfaColor::Green,
        handoff::CfaColor::Blue};
    descriptor.physicalFrameCount = 1;
    descriptor.independentEvidenceCount = 1;
    descriptor.sourceEvidenceHash = make_hash(1);
    descriptor.decoderAuditHash = make_hash(101);
    CHECK_TRUE(handoff::valid_descriptor(descriptor));

    auto invalidDescriptor = descriptor;
    invalidDescriptor.independentEvidenceCount = 2;
    CHECK_TRUE(!handoff::valid_descriptor(invalidDescriptor));

    std::vector<std::uint16_t> samples(
        static_cast<std::size_t>(descriptor.width) * descriptor.height);
    for (std::size_t i = 0; i < samples.size(); ++i) {
        samples[i] = static_cast<std::uint16_t>((i * 17u + 3u) & 0x3fffu);
    }

    handoff::Writer writer;
    CHECK_TRUE(writer.begin(store.fd, descriptor) == handoff::Status::Ok);
    CHECK_TRUE(writer.samples_expected() == samples.size());
    CHECK_TRUE(writer.resident_bytes_upper_bound() < 16u * 1024u);

    // A partial store can never be sealed.
    CHECK_TRUE(writer.append_samples(std::span<const std::uint16_t>(samples.data(), 37)) ==
               handoff::Status::Ok);
    CHECK_TRUE(writer.seal() == handoff::Status::InvalidState);

    std::size_t position = 37;
    while (position < samples.size()) {
        const std::size_t count = std::min<std::size_t>(31, samples.size() - position);
        CHECK_TRUE(writer.append_samples(
            std::span<const std::uint16_t>(samples.data() + position, count)) == handoff::Status::Ok);
        position += count;
    }
    CHECK_TRUE(writer.samples_written() == samples.size());
    CHECK_TRUE(writer.seal() == handoff::Status::Ok);
    CHECK_TRUE(writer.sealed());

    handoff::Reader reader;
    CHECK_TRUE(reader.open(store.fd) == handoff::Status::Ok);
    CHECK_TRUE(reader.info().sealed);
    CHECK_TRUE(reader.info().descriptor.physicalFrameCount == 1u);
    CHECK_TRUE(reader.info().descriptor.independentEvidenceCount == 1u);
    CHECK_TRUE(reader.resident_bytes_upper_bound() < 16u * 1024u);

    std::array<std::uint16_t, 15> rect{};
    CHECK_TRUE(reader.read_rect(3, 4, 5, 3, rect) == handoff::Status::InvalidState);
    CHECK_TRUE(reader.verify_payload_integrity() == handoff::Status::Ok);
    CHECK_TRUE(reader.info().payloadIntegrityVerified);
    CHECK_TRUE(reader.read_rect(3, 4, 5, 3, rect) == handoff::Status::Ok);

    std::size_t dst = 0;
    for (std::uint32_t row = 0; row < 3; ++row) {
        for (std::uint32_t col = 0; col < 5; ++col) {
            const std::size_t sourceIndex =
                static_cast<std::size_t>(4u + row) * descriptor.width + (3u + col);
            CHECK_TRUE(rect[dst++] == samples[sourceIndex]);
        }
    }

    // Wire a verified store into the Gatehouse detach rule.
    gate::DeviceEnvelope device{};
    device.appMemoryClassBytes = 1024ULL * 1024ULL * 1024ULL;
    device.currentlyAvailableBytes = 768ULL * 1024ULL * 1024ULL;
    device.cpuThreadBudget = 8;
    const auto plan = gate::plan_resources(device);
    CHECK_TRUE(plan.valid);

    const auto external = make_external_fixture();
    CHECK_TRUE(gate::route_adapter_output(external) == gate::EntryRoute::GatehouseRequired);
    const auto decode = gate::admit_decoder(plan, external.descriptor.resources);
    CHECK_TRUE(decode.admitted);

    gate::TransitHandoff transit{};
    transit.originalSourceStillSealed = true;
    transit.sourceEvidenceBindingVerified = true;
    transit.decodedRepresentationImmutable = true;
    transit.decodedRepresentationPersistedOrExternallyOwned = true;
    transit.decodedRepresentationIntegrityVerified = reader.info().payloadIntegrityVerified;
    transit.decoderContextLive = true;
    transit.fullFrameMaterializedDuringDecode = true;
    transit.decodedRepresentationResidentBytes = reader.resident_bytes_upper_bound();
    transit.physicalFrameCount = reader.info().descriptor.physicalFrameCount;
    transit.independentEvidenceCount = reader.info().descriptor.independentEvidenceCount;
    transit.evidenceClass = reader.info().descriptor.evidenceClass;
    transit.topology = reader.info().descriptor.topology;

    CHECK_TRUE(gate::may_seal_external_handoff(external, decode, transit));
    transit.sealed = true;
    CHECK_TRUE(!gate::may_enter_main_house_after_detach(
        gate::GatehouseState::HandoffSealed, transit));
    CHECK_TRUE(!gate::may_enter_main_house_after_detach(
        gate::GatehouseState::Detached, transit));

    // Only after decoder destruction/detach may Main House consume the store.
    transit.decoderContextLive = false;
    CHECK_TRUE(gate::may_enter_main_house_after_detach(
        gate::GatehouseState::Detached, transit));

    // Corruption after storage is detectable before a new consumer reads pixels.
    std::uint8_t byte = 0;
    const off_t tamperOffset = static_cast<off_t>(handoff::kHeaderBytes + 11u);
    CHECK_TRUE(::pread(store.fd, &byte, 1, tamperOffset) == 1);
    byte ^= 0x5au;
    CHECK_TRUE(::pwrite(store.fd, &byte, 1, tamperOffset) == 1);

    handoff::Reader corrupted;
    CHECK_TRUE(corrupted.open(store.fd) == handoff::Status::Ok);
    CHECK_TRUE(corrupted.verify_payload_integrity() == handoff::Status::CorruptPayload);

    std::cout << "Decoded Measurement Handoff v0.1 PASS\n";
    std::cout << "writer_resident_upper_bound=" << writer.resident_bytes_upper_bound() << "\n";
    std::cout << "reader_resident_upper_bound=" << reader.resident_bytes_upper_bound() << "\n";
    std::cout << "full_frame_handoff_buffer=0\n";
    std::cout << "payload_integrity_required_before_main_house=1\n";
    std::cout << "decoder_context_live_at_main_house=0\n";
    std::cout << "physical_frame_count=1\n";
    std::cout << "independent_evidence_count=1\n";
    return 0;
}
