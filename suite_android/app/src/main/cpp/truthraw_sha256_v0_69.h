#pragma once
#include <array>
#include <cstddef>
#include <cstdint>
#include <span>
#include <string>

namespace truthraw::sha256_v0_69 {
using Digest = std::array<std::uint8_t, 32>;

class Hasher final {
public:
    Hasher() = default;
    void update(const std::uint8_t* data, std::size_t size) noexcept;
    void update(std::span<const std::uint8_t> bytes) noexcept {
        update(bytes.data(), bytes.size());
    }
    Digest finalize() noexcept;
private:
    void transform(const std::uint8_t* block) noexcept;
    std::array<std::uint32_t, 8> state_{
        0x6a09e667u,0xbb67ae85u,0x3c6ef372u,0xa54ff53au,
        0x510e527fu,0x9b05688cu,0x1f83d9abu,0x5be0cd19u};
    std::array<std::uint8_t,64> block_{};
    std::size_t used_=0;
    std::uint64_t total_=0;
    bool finalized_=false;
};

std::string hex(const Digest& digest);
}
