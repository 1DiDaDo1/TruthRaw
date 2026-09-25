#include "free_world_scientific_open_scene_binding_v0_3.h"

#include <algorithm>
#include <bit>
#include <cmath>
#include <limits>

namespace truthraw::free_world_scientific_open_scene_binding::v0_3 {
namespace {

free_world::SourceCreationRole mapRole(field::CreationRole role) noexcept {
    switch (role) {
        case field::CreationRole::SourceMeasuredCfa:
            return free_world::SourceCreationRole::SourceMeasuredCfa;
        case field::CreationRole::ScientificReconstruction:
            return free_world::SourceCreationRole::ScientificReconstruction;
        case field::CreationRole::DenseProjection:
            return free_world::SourceCreationRole::DenseProjection;
        case field::CreationRole::RestorationDerivative:
            return free_world::SourceCreationRole::RestorationDerivative;
        case field::CreationRole::Unknown:
            return free_world::SourceCreationRole::Unknown;
    }
    return free_world::SourceCreationRole::Unknown;
}

free_world::SourceAuthority mapAuthority(field::Authority authority) noexcept {
    switch (authority) {
        case field::Authority::CalibratedEstimate:
            return free_world::SourceAuthority::CalibratedEstimate;
        case field::Authority::Reconstructed:
            return free_world::SourceAuthority::Reconstructed;
        case field::Authority::Censored:
            return free_world::SourceAuthority::Censored;
        case field::Authority::Unknown:
            return free_world::SourceAuthority::Unknown;
    }
    return free_world::SourceAuthority::Unknown;
}

free_world::BoundDomain mapBoundDomain(field::BoundDomain domain) noexcept {
    switch (domain) {
        case field::BoundDomain::None:
            return free_world::BoundDomain::None;
        case field::BoundDomain::SourceRawCode:
            return free_world::BoundDomain::SourceRawCode;
        case field::BoundDomain::SceneLinear:
            return free_world::BoundDomain::SceneLinear;
    }
    return free_world::BoundDomain::None;
}

}  // namespace

BoundScenePlane::BoundScenePlane(
    master::IScientificMasterTileSource& scientificMaster,
    local::IFieldTileSource& openSceneField,
    std::uint32_t width,
    std::uint32_t height) noexcept
    : master_(scientificMaster),
      field_(openSceneField),
      width_(width),
      height_(height) {
    const auto g = field_.geometry();
    if (width_ == 0u || height_ == 0u ||
        g.sourceWidth != width_ || g.sourceHeight != height_) {
        error_ = "Scientific Master/Open Scene source geometry mismatch";
        return;
    }
    valid_ = true;
}

std::uint32_t BoundScenePlane::width() const noexcept {
    return width_;
}

std::uint32_t BoundScenePlane::height() const noexcept {
    return height_;
}

bool BoundScenePlane::valid() const noexcept {
    return valid_;
}

const std::string& BoundScenePlane::error() const noexcept {
    return error_;
}

BindingReport BoundScenePlane::report() const noexcept {
    auto out = report_;
    out.fieldSchemaValidated = out.channelRecordsValidated > 0u;
    out.valueIdentityVerifiedForLoadedRecords =
        out.channelRecordsValidated > 0u &&
        out.masterFieldValueBitMismatches == 0u &&
        out.masterFieldValueBitMatches == out.channelRecordsValidated;
    return out;
}

const BoundScenePlane::CachedTile*
BoundScenePlane::findOrLoadCanonicalTile(
    std::uint32_t tileX,
    std::uint32_t tileY) const noexcept {
    for (auto& slot : cache_) {
        if (slot.valid && slot.x == tileX && slot.y == tileY) {
            slot.lastUse = ++cacheClock_;
            return &slot;
        }
    }

    auto* victim = &cache_[0];
    for (auto& slot : cache_) {
        if (!slot.valid) {
            victim = &slot;
            break;
        }
        if (slot.lastUse < victim->lastUse) victim = &slot;
    }
    if (!loadCanonicalTile(tileX, tileY, *victim)) return nullptr;
    victim->lastUse = ++cacheClock_;
    return victim;
}

bool BoundScenePlane::loadCanonicalTile(
    std::uint32_t tileX,
    std::uint32_t tileY,
    CachedTile& destination) const noexcept {
    try {
        destination.valid = false;
        if (!valid_ || tileX >= width_ || tileY >= height_ ||
            (tileX % field::kCanonicalTileEdge) != 0u ||
            (tileY % field::kCanonicalTileEdge) != 0u) {
            error_ = "invalid canonical Scientific Master/Open Scene tile request";
            return false;
        }

        const std::uint32_t tileWidth =
            std::min(field::kCanonicalTileEdge, width_ - tileX);
        const std::uint32_t tileHeight =
            std::min(field::kCanonicalTileEdge, height_ - tileY);
        const std::size_t pixels =
            static_cast<std::size_t>(tileWidth) * tileHeight;
        if (pixels > std::numeric_limits<std::size_t>::max() / 3u) {
            error_ = "Scientific Master/Open Scene tile size overflow";
            return false;
        }
        const std::size_t channels = pixels * 3u;

        masterRgb_.assign(channels, 0.0f);
        const auto masterStatus = master_.readCameraNativeTile(
            tileX, tileY, tileWidth, tileHeight,
            masterRgb_.data(), masterRgb_.size());
        if (!masterStatus) {
            error_ = "Scientific Master tile read failed: " +
                masterStatus.message;
            return false;
        }

        fieldRecords_.assign(channels, field::ChannelRecord{});
        if (!field_.readSourceTile(
                tileX, tileY, tileWidth, tileHeight,
                fieldRecords_.data(), fieldRecords_.size())) {
            error_ = "Open Scene Field tile read failed";
            return false;
        }

        destination.pixels.assign(pixels, free_world::SourcePixel{});
        for (std::size_t i = 0u; i < channels; ++i) {
            const auto& record = fieldRecords_[i];
            if (!field::validate_record(record) || !record.valuePresent) {
                error_ = "Open Scene Field record failed v0.85 validation";
                return false;
            }
            ++report_.channelRecordsValidated;

            if (std::bit_cast<std::uint32_t>(record.value) !=
                std::bit_cast<std::uint32_t>(masterRgb_[i])) {
                ++report_.masterFieldValueBitMismatches;
                error_ = "Scientific Master/Open Scene value bit identity mismatch";
                return false;
            }
            ++report_.masterFieldValueBitMatches;

            auto& dst = destination.pixels[i / 3u].channel[i % 3u];
            dst.value = static_cast<double>(masterRgb_[i]);
            dst.role = mapRole(record.role);
            dst.authority = mapAuthority(record.authority);
            dst.uncertaintyKnown = record.p95Known;
            dst.p95Uncertainty =
                record.p95Known ? static_cast<double>(record.p95) : 0.0;
            dst.boundKnown = record.boundKnown;
            dst.lowerBound =
                record.boundKnown ? static_cast<double>(record.bound) : 0.0;
            dst.boundDomain = mapBoundDomain(record.boundDomain);
            dst.contributionMask = record.contributionMask;
        }

        destination.x = tileX;
        destination.y = tileY;
        destination.width = tileWidth;
        destination.height = tileHeight;
        destination.valid = true;
        ++report_.tileLoads;
        error_.clear();
        return true;
    } catch (...) {
        error_ = "unexpected Scientific Master/Open Scene binding failure";
        destination.valid = false;
        return false;
    }
}

bool BoundScenePlane::readPixel(
    std::uint32_t x,
    std::uint32_t y,
    free_world::SourcePixel& out) const noexcept {
    out = free_world::SourcePixel{};
    if (!valid_ || x >= width_ || y >= height_) return false;

    const std::uint32_t tileX =
        (x / field::kCanonicalTileEdge) * field::kCanonicalTileEdge;
    const std::uint32_t tileY =
        (y / field::kCanonicalTileEdge) * field::kCanonicalTileEdge;

    const CachedTile* tile = findOrLoadCanonicalTile(tileX, tileY);
    if (tile == nullptr || !tile->valid) return false;

    if (x < tile->x || y < tile->y ||
        x >= tile->x + tile->width ||
        y >= tile->y + tile->height) {
        error_ = "Scientific Master/Open Scene cache address mismatch";
        return false;
    }

    const std::size_t localX = x - tile->x;
    const std::size_t localY = y - tile->y;
    const std::size_t index =
        localY * static_cast<std::size_t>(tile->width) + localX;
    if (index >= tile->pixels.size()) {
        error_ = "Scientific Master/Open Scene cache index overflow";
        return false;
    }
    out = tile->pixels[index];
    return true;
}

const char* schema_name() noexcept {
    return "FreeWorldScientificOpenSceneBinding/0.3";
}

}  // namespace truthraw::free_world_scientific_open_scene_binding::v0_3
