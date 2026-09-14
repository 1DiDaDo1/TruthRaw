#include "truthraw_dng_certificate_embed_v0_1.h"
#include "truthraw_certificate_v0_1.h"

#include <array>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <string>
#include <unistd.h>
#include <vector>

namespace cert = truthraw::certificate::v0_1;
namespace embed = truthraw::dng_certificate_embed::v0_1;

#define REQUIRE(expr) do { \
    if (!(expr)) { \
        std::cerr << "REQUIRE failed: " #expr " at " << __FILE__ << ':' << __LINE__ << '\n'; \
        std::exit(2); \
    } \
} while (false)

namespace {

void put_u16(std::vector<std::uint8_t>& out, std::size_t o, std::uint16_t v) {
    out[o] = static_cast<std::uint8_t>(v & 0xffu);
    out[o + 1u] = static_cast<std::uint8_t>((v >> 8u) & 0xffu);
}

void put_u32(std::vector<std::uint8_t>& out, std::size_t o, std::uint32_t v) {
    out[o] = static_cast<std::uint8_t>(v & 0xffu);
    out[o + 1u] = static_cast<std::uint8_t>((v >> 8u) & 0xffu);
    out[o + 2u] = static_cast<std::uint8_t>((v >> 16u) & 0xffu);
    out[o + 3u] = static_cast<std::uint8_t>((v >> 24u) & 0xffu);
}

std::uint32_t u32(const std::uint8_t* p) {
    return static_cast<std::uint32_t>(p[0]) |
           (static_cast<std::uint32_t>(p[1]) << 8u) |
           (static_cast<std::uint32_t>(p[2]) << 16u) |
           (static_cast<std::uint32_t>(p[3]) << 24u);
}

cert::Hash256 hash_seed(std::uint8_t seed) {
    cert::Hash256 out{};
    for (std::size_t i = 0u; i < out.size(); ++i) {
        out[i] = static_cast<std::uint8_t>(seed + static_cast<std::uint8_t>(i));
    }
    return out;
}

cert::SerializedCertificate make_certificate() {
    cert::State state{};
    state.projectionClass = cert::ProjectionClass::TruthRawPureFloat32Dng;
    state.claimClass = cert::ClaimClass::Reconstructed;
    state.signatureState = cert::SignatureState::UnsignedDevelopment;
    state.signatureAlgorithm = cert::SignatureAlgorithm::None;
    state.colorClaimScope = 1u;
    state.sourceEvidenceSha256 = hash_seed(0x10u);
    state.scientificMasterSha256 = hash_seed(0x30u);
    state.zeroLineSha256 = hash_seed(0x50u);
    state.sceneScaleSha256 = hash_seed(0x70u);
    state.technicalBackplaneCrc32 = 0x1234abcdu;
    state.buildIdentitySha256 = hash_seed(0x90u);
    cert::SerializedCertificate bytes{};
    REQUIRE(cert::serialize(state, bytes) == cert::Status::Ok);
    return bytes;
}

std::vector<std::uint8_t> minimal_dng_private_data_fixture() {
    constexpr std::uint32_t privateOffset = 26u;
    constexpr std::uint32_t privateCount = 6u;
    std::vector<std::uint8_t> bytes(34u, 0u);
    bytes[0] = 'I'; bytes[1] = 'I';
    put_u16(bytes, 2u, 42u);
    put_u32(bytes, 4u, 8u);
    put_u16(bytes, 8u, 1u);
    put_u16(bytes, 10u, 50740u);
    put_u16(bytes, 12u, 1u);
    put_u32(bytes, 14u, privateCount);
    put_u32(bytes, 18u, privateOffset);
    put_u32(bytes, 22u, 0u);
    const char existing[privateCount] = {'o','l','d','v','1','\0'};
    std::memcpy(bytes.data() + privateOffset, existing, privateCount);
    for (std::size_t i = 32u; i < bytes.size(); ++i) bytes[i] = 0xa5u;
    return bytes;
}

int temp_fd() {
    char path[] = "/tmp/truthraw-cert-embed-XXXXXX";
    const int fd = ::mkstemp(path);
    REQUIRE(fd >= 0);
    REQUIRE(::unlink(path) == 0);
    return fd;
}

void write_fixture(int fd, const std::vector<std::uint8_t>& bytes) {
    REQUIRE(::ftruncate(fd, 0) == 0);
    std::size_t done = 0u;
    while (done < bytes.size()) {
        const ssize_t n = ::write(fd, bytes.data() + done, bytes.size() - done);
        REQUIRE(n > 0);
        done += static_cast<std::size_t>(n);
    }
}

void test_embed_preserves_existing_private_data() {
    const auto certificate = make_certificate();
    const auto fixture = minimal_dng_private_data_fixture();
    const int fd = temp_fd();
    write_fixture(fd, fixture);

    embed::Result result{};
    const auto status = embed::embed_certificate(fd, certificate, result);
    if (!status) std::cerr << status.message << '\n';
    REQUIRE(status);
    REQUIRE(result.oldPrivateDataBytes == 6u);
    REQUIRE(result.newPrivateDataBytes == 6u + cert::kSerializedBytes);
    REQUIRE(result.certificateBytes == cert::kSerializedBytes);
    REQUIRE(result.existingPrivateDataPreserved);
    REQUIRE(result.ifdCommitApplied);

    std::array<std::uint8_t, 8> patch{};
    REQUIRE(::pread(fd, patch.data(), patch.size(), 14) == static_cast<ssize_t>(patch.size()));
    REQUIRE(u32(patch.data()) == result.newPrivateDataBytes);
    const std::uint32_t payloadOffset = u32(patch.data() + 4u);
    REQUIRE(payloadOffset == 36u);

    std::vector<std::uint8_t> payload(result.newPrivateDataBytes);
    REQUIRE(::pread(fd, payload.data(), payload.size(), payloadOffset) ==
            static_cast<ssize_t>(payload.size()));
    REQUIRE(std::memcmp(payload.data(), "oldv1\0", 6u) == 0);
    REQUIRE(std::equal(certificate.begin(), certificate.end(), payload.begin() + 6));
    REQUIRE(result.certificateBlockOffset == payloadOffset + 6u);
    ::close(fd);
}

void test_bad_certificate_rejected_without_mutation() {
    auto certificate = make_certificate();
    certificate[0] = 'X';
    const auto fixture = minimal_dng_private_data_fixture();
    const int fd = temp_fd();
    write_fixture(fd, fixture);

    embed::Result result{};
    REQUIRE(embed::embed_certificate(fd, certificate, result).code ==
            embed::StatusCode::InvalidArgument);
    const off_t end = ::lseek(fd, 0, SEEK_END);
    REQUIRE(end == static_cast<off_t>(fixture.size()));
    ::close(fd);
}

}  // namespace

int main() {
    test_embed_preserves_existing_private_data();
    test_bad_certificate_rejected_without_mutation();
    std::cout << "TRUTHRAW_DNG_CERTIFICATE_EMBED_V0_1_PASS\n";
    std::cout << "certificate_inside_dng_private_data=1\n";
    std::cout << "existing_private_data_preserved=1\n";
    std::cout << "pixel_data_rewrite=0\n";
    return 0;
}
