#include "scientific_preview_source_binding_v0_1.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstring>
#include <limits>
#include <vector>

namespace truthraw::scientific_preview_binding_v0_1 {
namespace {

constexpr std::array<std::uint32_t, 64> kShaK = {
    0x428a2f98u,0x71374491u,0xb5c0fbcfu,0xe9b5dba5u,0x3956c25bu,0x59f111f1u,0x923f82a4u,0xab1c5ed5u,
    0xd807aa98u,0x12835b01u,0x243185beu,0x550c7dc3u,0x72be5d74u,0x80deb1feu,0x9bdc06a7u,0xc19bf174u,
    0xe49b69c1u,0xefbe4786u,0x0fc19dc6u,0x240ca1ccu,0x2de92c6fu,0x4a7484aau,0x5cb0a9dcu,0x76f988dau,
    0x983e5152u,0xa831c66du,0xb00327c8u,0xbf597fc7u,0xc6e00bf3u,0xd5a79147u,0x06ca6351u,0x14292967u,
    0x27b70a85u,0x2e1b2138u,0x4d2c6dfcu,0x53380d13u,0x650a7354u,0x766a0abbu,0x81c2c92eu,0x92722c85u,
    0xa2bfe8a1u,0xa81a664bu,0xc24b8b70u,0xc76c51a3u,0xd192e819u,0xd6990624u,0xf40e3585u,0x106aa070u,
    0x19a4c116u,0x1e376c08u,0x2748774cu,0x34b0bcb5u,0x391c0cb3u,0x4ed8aa4au,0x5b9cca4fu,0x682e6ff3u,
    0x748f82eeu,0x78a5636fu,0x84c87814u,0x8cc70208u,0x90befffau,0xa4506cebu,0xbef9a3f7u,0xc67178f2u
};

std::uint32_t rotr(std::uint32_t x, unsigned n) noexcept { return (x >> n) | (x << (32u - n)); }

class Sha256 {
public:
    void update(const std::uint8_t* data, std::size_t n) {
        totalBytes_ += n;
        while (n > 0) {
            const std::size_t take = std::min(n, block_.size() - used_);
            std::memcpy(block_.data() + used_, data, take);
            used_ += take; data += take; n -= take;
            if (used_ == block_.size()) { transform(block_.data()); used_ = 0; }
        }
    }

    std::array<std::uint8_t, 32> finish() {
        const std::uint64_t bitLength = totalBytes_ * 8ull;
        block_[used_++] = 0x80u;
        if (used_ > 56) {
            std::fill(block_.begin() + static_cast<std::ptrdiff_t>(used_), block_.end(), 0u);
            transform(block_.data()); used_ = 0;
        }
        std::fill(block_.begin() + static_cast<std::ptrdiff_t>(used_), block_.begin() + 56, 0u);
        for (int i = 0; i < 8; ++i) block_[63 - i] = static_cast<std::uint8_t>(bitLength >> (8 * i));
        transform(block_.data());
        std::array<std::uint8_t, 32> out{};
        for (std::size_t i = 0; i < state_.size(); ++i) {
            out[4*i] = static_cast<std::uint8_t>(state_[i] >> 24);
            out[4*i+1] = static_cast<std::uint8_t>(state_[i] >> 16);
            out[4*i+2] = static_cast<std::uint8_t>(state_[i] >> 8);
            out[4*i+3] = static_cast<std::uint8_t>(state_[i]);
        }
        return out;
    }

private:
    void transform(const std::uint8_t* block) {
        std::uint32_t w[64]{};
        for (int i = 0; i < 16; ++i) {
            const int q = 4*i;
            w[i] = (std::uint32_t(block[q]) << 24) | (std::uint32_t(block[q+1]) << 16) |
                   (std::uint32_t(block[q+2]) << 8) | std::uint32_t(block[q+3]);
        }
        for (int i = 16; i < 64; ++i) {
            const std::uint32_t s0 = rotr(w[i-15],7) ^ rotr(w[i-15],18) ^ (w[i-15] >> 3);
            const std::uint32_t s1 = rotr(w[i-2],17) ^ rotr(w[i-2],19) ^ (w[i-2] >> 10);
            w[i] = w[i-16] + s0 + w[i-7] + s1;
        }
        std::uint32_t a=state_[0],b=state_[1],c=state_[2],d=state_[3],e=state_[4],f=state_[5],g=state_[6],h=state_[7];
        for (int i = 0; i < 64; ++i) {
            const std::uint32_t S1 = rotr(e,6) ^ rotr(e,11) ^ rotr(e,25);
            const std::uint32_t ch = (e & f) ^ ((~e) & g);
            const std::uint32_t temp1 = h + S1 + ch + kShaK[static_cast<std::size_t>(i)] + w[i];
            const std::uint32_t S0 = rotr(a,2) ^ rotr(a,13) ^ rotr(a,22);
            const std::uint32_t maj = (a & b) ^ (a & c) ^ (b & c);
            const std::uint32_t temp2 = S0 + maj;
            h=g; g=f; f=e; e=d+temp1; d=c; c=b; b=a; a=temp1+temp2;
        }
        state_[0]+=a; state_[1]+=b; state_[2]+=c; state_[3]+=d;
        state_[4]+=e; state_[5]+=f; state_[6]+=g; state_[7]+=h;
    }

    std::array<std::uint32_t, 8> state_ = {0x6a09e667u,0xbb67ae85u,0x3c6ef372u,0xa54ff53au,0x510e527fu,0x9b05688cu,0x1f83d9abu,0x5be0cd19u};
    std::array<std::uint8_t, 64> block_{};
    std::size_t used_ = 0;
    std::uint64_t totalBytes_ = 0;
};

std::string make_source_id(const std::array<std::uint8_t, 32>& digest) {
    static constexpr char hex[] = "0123456789abcdef";
    std::string out = "sha256:";
    out.reserve(71);
    for (const auto b : digest) { out.push_back(hex[b >> 4]); out.push_back(hex[b & 0x0f]); }
    return out;
}

bool finite_matrix(const std::array<float, 9>& m) noexcept {
    for (const float v : m) if (!std::isfinite(v)) return false;
    return true;
}

ColorClaimScope scope_for(ColorBindingAuthority authority) noexcept {
    if (authority == ColorBindingAuthority::IndependentCalibration) return ColorClaimScope::IndependentlyCalibratedPreview;
    if (authority == ColorBindingAuthority::SourceMetadataBound || authority == ColorBindingAuthority::GatehouseCertifiedMetadata)
        return ColorClaimScope::SourceBoundPreview;
    return ColorClaimScope::None;
}

} // namespace

bool is_canonical_source_evidence_id(const std::string& id) noexcept {
    if (id.size() != 71 || id.compare(0, 7, "sha256:") != 0) return false;
    for (std::size_t i = 7; i < id.size(); ++i) {
        const char c = id[i];
        if (!((c >= '0' && c <= '9') || (c >= 'a' && c <= 'f'))) return false;
    }
    return true;
}

BindingStatus seal_source_sha256(tile_dng_v0_1::IRandomAccessByteSource& source, SourceSeal& out, std::size_t chunkBytes) {
    if (chunkBytes < 1024 || chunkBytes > kDefaultHashChunkBytes) {
        return BindingStatus::error(BindingStatusCode::InvalidArgument, "hash chunk must be between 1 KiB and 64 KiB");
    }
    std::vector<std::uint8_t> chunk(chunkBytes);
    Sha256 sha;
    const std::uint64_t n = source.sizeBytes();
    std::uint64_t offset = 0;
    while (offset < n) {
        const auto remaining = n - offset;
        const std::size_t take = static_cast<std::size_t>(std::min<std::uint64_t>(remaining, chunk.size()));
        if (!source.readExact(offset, chunk.data(), take)) {
            return BindingStatus::error(BindingStatusCode::SourceReadFailed, "source changed or could not be read while sealing");
        }
        sha.update(chunk.data(), take);
        offset += take;
    }
    SourceSeal sealed;
    sealed.sha256 = sha.finish();
    sealed.byteLength = n;
    sealed.hashWorkspacePeakBytes = chunk.size() + 64u + 8u * sizeof(std::uint32_t);
    sealed.sourceEvidenceId = make_source_id(sealed.sha256);
    out = sealed;
    return BindingStatus::ok();
}

BindingStatus reverify_source_sha256(tile_dng_v0_1::IRandomAccessByteSource& source, const SourceSeal& expected, std::size_t chunkBytes) {
    if (!is_canonical_source_evidence_id(expected.sourceEvidenceId) || expected.sourceEvidenceId != make_source_id(expected.sha256))
        return BindingStatus::error(BindingStatusCode::InvalidSourceSeal, "expected source seal is not canonical");
    SourceSeal actual;
    auto status = seal_source_sha256(source, actual, chunkBytes);
    if (!status) return status;
    if (actual.byteLength != expected.byteLength || actual.sha256 != expected.sha256 || actual.sourceEvidenceId != expected.sourceEvidenceId)
        return BindingStatus::error(BindingStatusCode::SourceSealMismatch, "source bytes no longer match the sealed evidence identity");
    return BindingStatus::ok();
}

BindingStatus admit_scientific_color_preview(
    const SourceSeal& sourceSeal,
    const ScientificColorBindingRecord& color,
    const technical_backplane::v0_1::State& backplane,
    ScientificPreviewAdmission& out) {
    if (sourceSeal.byteLength == 0 || !is_canonical_source_evidence_id(sourceSeal.sourceEvidenceId) ||
        sourceSeal.sourceEvidenceId != make_source_id(sourceSeal.sha256))
        return BindingStatus::error(BindingStatusCode::InvalidSourceSeal, "scientific preview requires a canonical non-empty source SHA-256 seal");
    if (!color.validated || scope_for(color.authority) == ColorClaimScope::None)
        return BindingStatus::error(BindingStatusCode::UnauthorizedColorBinding, "preview sentinel/unverified color authority cannot enter scientific color");
    if (color.sourceEvidenceId != sourceSeal.sourceEvidenceId)
        return BindingStatus::error(BindingStatusCode::BindingSourceMismatch, "color binding belongs to different source evidence");
    if (color.bindingId.empty() || !finite_matrix(color.cameraToXyzD50))
        return BindingStatus::error(BindingStatusCode::InvalidMatrix, "color binding ID/matrix is invalid");
    if (color.physicalFrameCount != 1 || color.independentEvidenceCount != 1 ||
        backplane.physicalFrameCount != 1 || backplane.independentEvidenceCount != 1)
        return BindingStatus::error(BindingStatusCode::EvidenceInvariantViolation, "scientific preview cannot alter single-frame evidence counts");
    if (technical_backplane::v0_1::validate(backplane) != technical_backplane::v0_1::Status::Ok)
        return BindingStatus::error(BindingStatusCode::BackplaneRejected, "Technical Backplane validation failed");
    if (backplane.sourceEvidenceHash != sourceSeal.sha256)
        return BindingStatus::error(BindingStatusCode::BackplaneSourceMismatch, "Backplane sourceEvidenceHash does not match sealed source bytes");

    ScientificPreviewAdmission admitted;
    admitted.sourceSeal = sourceSeal;
    admitted.claimScope = scope_for(color.authority);
    admitted.tileNativeOptions.sourceEvidenceId = sourceSeal.sourceEvidenceId;
    admitted.tileNativeOptions.color.valid = true;
    admitted.tileNativeOptions.color.bindingId = color.bindingId;
    admitted.tileNativeOptions.color.cameraToXyzD50 = color.cameraToXyzD50;
    out = admitted;
    return BindingStatus::ok();
}

const char* authority_name(ColorBindingAuthority authority) noexcept {
    switch (authority) {
        case ColorBindingAuthority::Unverified: return "UNVERIFIED";
        case ColorBindingAuthority::PreviewSentinel: return "PREVIEW_SENTINEL";
        case ColorBindingAuthority::SourceMetadataBound: return "SOURCE_METADATA_BOUND";
        case ColorBindingAuthority::GatehouseCertifiedMetadata: return "GATEHOUSE_CERTIFIED_METADATA";
        case ColorBindingAuthority::IndependentCalibration: return "INDEPENDENT_CALIBRATION";
    }
    return "UNKNOWN";
}

const char* status_name(BindingStatusCode code) noexcept {
    switch (code) {
        case BindingStatusCode::Ok: return "OK";
        case BindingStatusCode::InvalidArgument: return "INVALID_ARGUMENT";
        case BindingStatusCode::SourceReadFailed: return "SOURCE_READ_FAILED";
        case BindingStatusCode::InvalidSourceSeal: return "INVALID_SOURCE_SEAL";
        case BindingStatusCode::SourceSealMismatch: return "SOURCE_SEAL_MISMATCH";
        case BindingStatusCode::UnauthorizedColorBinding: return "UNAUTHORIZED_COLOR_BINDING";
        case BindingStatusCode::BindingSourceMismatch: return "BINDING_SOURCE_MISMATCH";
        case BindingStatusCode::InvalidMatrix: return "INVALID_MATRIX";
        case BindingStatusCode::EvidenceInvariantViolation: return "EVIDENCE_INVARIANT_VIOLATION";
        case BindingStatusCode::BackplaneRejected: return "BACKPLANE_REJECTED";
        case BindingStatusCode::BackplaneSourceMismatch: return "BACKPLANE_SOURCE_MISMATCH";
    }
    return "UNKNOWN";
}

} // namespace truthraw::scientific_preview_binding_v0_1
