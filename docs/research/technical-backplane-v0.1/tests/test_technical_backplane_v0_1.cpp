#include "technical_backplane_v0_1.h"

#include <algorithm>
#include <cstdint>
#include <iostream>

using namespace truthraw::technical_backplane::v0_1;

#define CHECK(x) do { if (!(x)) { std::cerr << "CHECK failed: " #x << " line " << __LINE__ << '\n'; return 1; } } while (0)

static Hash256 make_hash(std::uint8_t seed) {
    Hash256 h{};
    for (std::size_t i = 0; i < h.size(); ++i) h[i] = static_cast<std::uint8_t>(seed + i * 7u);
    return h;
}

static std::size_t count_subsequence(const SerializedBackplane& haystack, const Hash256& needle) {
    std::size_t count = 0;
    for (std::size_t i = 0; i + needle.size() <= haystack.size(); ++i) {
        if (std::equal(needle.begin(), needle.end(), haystack.begin() + static_cast<std::ptrdiff_t>(i))) ++count;
    }
    return count;
}

int main() {
    static_assert(kSerializedBytes == 180);
    State state{};
    state.sourceEvidenceHash = make_hash(1);
    state.scientificMasterHash = make_hash(33);
    state.zeroLineHash = make_hash(65);
    state.sceneScaleHash = make_hash(97);
    for (std::size_t i = 0; i < kRoomCount; ++i) {
        state.roomStatus[i] = (i % 3 == 0) ? RoomStatus::Available : RoomStatus::ResearchOnly;
    }
    state.claimStatus = ClaimStatus::Candidate;

    SerializedBackplane a{}, b{};
    CHECK(serialize(state, a) == Status::Ok);
    CHECK(serialize(state, b) == Status::Ok);
    CHECK(a == b);
    CHECK(count_subsequence(a, state.zeroLineHash) == 1);

    State roundtrip{};
    CHECK(deserialize(a, roundtrip) == Status::Ok);
    CHECK(roundtrip.sourceEvidenceHash == state.sourceEvidenceHash);
    CHECK(roundtrip.scientificMasterHash == state.scientificMasterHash);
    CHECK(roundtrip.zeroLineHash == state.zeroLineHash);
    CHECK(roundtrip.sceneScaleHash == state.sceneScaleHash);
    CHECK(roundtrip.physicalFrameCount == 1);
    CHECK(roundtrip.independentEvidenceCount == 1);
    CHECK(roundtrip.roomStatus == state.roomStatus);
    CHECK(roundtrip.claimStatus == state.claimStatus);

    auto corrupt = a;
    corrupt[91] ^= 0x01u;
    CHECK(deserialize(corrupt, roundtrip) == Status::CorruptRecord);

    corrupt = a;
    corrupt[170] = 1u;
    const auto c = crc32(std::span<const std::uint8_t>(corrupt.data(), 176));
    corrupt[176] = static_cast<std::uint8_t>(c & 0xffu);
    corrupt[177] = static_cast<std::uint8_t>((c >> 8u) & 0xffu);
    corrupt[178] = static_cast<std::uint8_t>((c >> 16u) & 0xffu);
    corrupt[179] = static_cast<std::uint8_t>((c >> 24u) & 0xffu);
    CHECK(deserialize(corrupt, roundtrip) == Status::CorruptRecord);

    auto invalid = state;
    invalid.independentEvidenceCount = 2;
    CHECK(serialize(invalid, b) == Status::EvidenceInvariantViolation);
    invalid = state;
    invalid.physicalFrameCount = 2;
    CHECK(serialize(invalid, b) == Status::EvidenceInvariantViolation);
    invalid = state;
    invalid.forbiddenFlags = ScientificMasterModified;
    CHECK(serialize(invalid, b) == Status::MutableScientificState);
    invalid = state;
    invalid.forbiddenFlags = ZeroLineModified;
    CHECK(serialize(invalid, b) == Status::MutableScientificState);
    invalid = state;
    invalid.forbiddenFlags = AppearanceCountedAsEvidence;
    CHECK(serialize(invalid, b) == Status::MutableScientificState);
    invalid = state;
    invalid.forbiddenFlags = CounterfactualCountedAsEvidence;
    CHECK(serialize(invalid, b) == Status::MutableScientificState);
    invalid = state;
    invalid.zeroLineHash.fill(0);
    CHECK(serialize(invalid, b) == Status::InvalidInput);
    invalid = state;
    invalid.roomStatus[4] = static_cast<RoomStatus>(255);
    CHECK(serialize(invalid, b) == Status::InvalidStatus);

    std::cout << "TECHNICAL_BACKPLANE_V0_1_TEST_PASS\n";
    std::cout << "serializedBytes=" << kSerializedBytes << '\n';
    std::cout << "zeroLineOccurrences=" << count_subsequence(a, state.zeroLineHash) << '\n';
    std::cout << "physicalFrameCount=" << state.physicalFrameCount << '\n';
    std::cout << "independentEvidenceCount=" << state.independentEvidenceCount << '\n';
    std::cout << "crc32=" << crc32(std::span<const std::uint8_t>(a.data(), 176)) << '\n';
    return 0;
}
