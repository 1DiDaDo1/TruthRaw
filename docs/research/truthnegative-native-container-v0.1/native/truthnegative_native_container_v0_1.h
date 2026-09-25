#pragma once

#include "open_scene_field_v0_85.h"
#include "truthnegative_continuous_v0_5.h"
#include "truthnegative_local_authority_projection_v0_4.h"
#include "truthraw_sha256_v0_69.h"

#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

namespace truthraw::truthnegative_native_container::v0_1 {

namespace field = truthraw::open_scene_field::v0_85;
namespace local = truthraw::truthnegative_local_authority_projection::v0_4;
namespace tn = truthraw::truthnegative_continuous::v0_5;
using Digest = truthraw::sha256_v0_69::Digest;

inline constexpr const char* kSchemaName =
    "TruthNegativeNativeContainer/0.1";
inline constexpr std::size_t kHeaderBytes = 4096u;
inline constexpr std::uint32_t kContainerVersion = 1u;

class IRandomAccessSink {
public:
    virtual ~IRandomAccessSink() = default;
    virtual bool writeAt(
        std::uint64_t offset,
        const void* data,
        std::size_t size) noexcept = 0;
    virtual bool resize(std::uint64_t size) noexcept = 0;
};

class IRandomAccessSource {
public:
    virtual ~IRandomAccessSource() = default;
    virtual std::uint64_t sizeBytes() const noexcept = 0;
    virtual bool readAt(
        std::uint64_t offset,
        void* data,
        std::size_t size) const noexcept = 0;
};

struct WriteInput final {
    tn::State state{};
    std::string colorBindingId;
};

struct Summary final {
    Digest sourceEvidenceSha256{};
    Digest scientificMasterSha256{};
    Digest authorityFieldSha256{};
    Digest truthNegativeStateSha256{};
    Digest bodySha256{};
    Digest containerSha256{};
    std::uint32_t width = 0u;
    std::uint32_t height = 0u;
    std::uint64_t tileCount = 0u;
    std::uint64_t recordCount = 0u;
    std::uint64_t bodyBytes = 0u;
    std::uint64_t fileBytes = 0u;
    bool physicalFrameCountOne = true;
    bool independentEvidenceCountOne = true;
    bool createsNewEvidence = false;
    bool scientificWritebackAllowed = false;
};

bool write(
    const WriteInput& input,
    local::IFieldTileSource& fieldSource,
    IRandomAccessSink& sink,
    Summary& out) noexcept;

class Reader final : public local::IFieldTileSource {
public:
    Reader() = default;

    bool open(const IRandomAccessSource& source) noexcept;

    bool valid() const noexcept { return valid_; }
    const std::string& error() const noexcept { return error_; }
    const Summary& summary() const noexcept { return summary_; }

    local::Geometry geometry() const noexcept override;
    bool readSourceTile(
        std::uint32_t x,
        std::uint32_t y,
        std::uint32_t width,
        std::uint32_t height,
        field::ChannelRecord* out,
        std::size_t recordCount) noexcept override;

private:
    struct TileIndex final {
        std::uint32_t x = 0u;
        std::uint32_t y = 0u;
        std::uint32_t width = 0u;
        std::uint32_t height = 0u;
        std::uint64_t payloadOffset = 0u;
        std::uint32_t payloadBytes = 0u;
        Digest payloadSha256{};
    };

    const IRandomAccessSource* source_ = nullptr;
    Summary summary_{};
    std::vector<TileIndex> tiles_;
    bool valid_ = false;
    std::string error_;
};

const char* schema_name() noexcept;

}  // namespace truthraw::truthnegative_native_container::v0_1
