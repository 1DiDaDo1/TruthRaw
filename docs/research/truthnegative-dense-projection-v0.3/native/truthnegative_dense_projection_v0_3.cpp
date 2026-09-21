#include "truthnegative_dense_projection_v0_3.h"

#include "scientific_master_digest_v0_1.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <limits>
#include <new>
#include <sstream>
#include <utility>
#include <vector>

namespace truthraw::truthnegative_dense_projection::v0_3 {
namespace {

namespace digest = truthraw::scientific_master_digest::v0_1;

struct SourceCell final {
    std::uint32_t x = 0u;
    std::uint32_t y = 0u;
    std::uint32_t width = 0u;
    std::uint32_t height = 0u;
    std::vector<float> rgb;
};

inline std::uint32_t clamp_index(
    std::int64_t value,
    std::uint32_t limit) noexcept {
    if (limit == 0u || value <= 0) return 0u;
    const auto u = static_cast<std::uint64_t>(value);
    return u >= limit ? limit - 1u : static_cast<std::uint32_t>(u);
}

inline void axis_map(
    std::uint32_t targetCoordinate,
    std::uint32_t sourceLimit,
    std::uint32_t& s0,
    std::uint32_t& s1,
    float& fraction) noexcept {
    const std::uint32_t n = targetCoordinate >> 2u;
    int base = 0;
    switch (targetCoordinate & 3u) {
        case 0u: base = static_cast<int>(n) - 1; fraction = 0.625f; break;
        case 1u: base = static_cast<int>(n) - 1; fraction = 0.875f; break;
        case 2u: base = static_cast<int>(n); fraction = 0.125f; break;
        default: base = static_cast<int>(n); fraction = 0.375f; break;
    }
    s0 = clamp_index(base, sourceLimit);
    s1 = clamp_index(base + 1, sourceLimit);
}

inline constexpr std::uint32_t kAcceleratedBlockEdge = 256u;

}  // namespace

struct DenseProjectionTileSource::Impl final {
    Geometry geometry{};
    bool valid = false;
    std::string error;
    std::vector<SourceCell> cells;
    IExactDenseAccelerator* accelerator = nullptr;
    bool acceleratorDisabled = false;
    bool acceleratorUsed = false;
    std::string acceleratorFailure;

    bool acceleratedBlockValid = false;
    std::uint32_t acceleratedBlockX = 0u;
    std::uint32_t acceleratedBlockY = 0u;
    std::uint32_t acceleratedBlockWidth = 0u;
    std::uint32_t acceleratedBlockHeight = 0u;
    std::vector<float> packedPatch;
    std::vector<float> acceleratedBlock;

    const SourceCell* findCell(std::uint32_t sx, std::uint32_t sy) const noexcept {
        for (const auto& cell : cells) {
            if (sx >= cell.x && sy >= cell.y &&
                sx < cell.x + cell.width &&
                sy < cell.y + cell.height) {
                return &cell;
            }
        }
        return nullptr;
    }
};

DenseProjectionTileSource::DenseProjectionTileSource(
    float_dng::IScientificMasterTileSource& scientificMaster,
    std::uint32_t sourceWidth,
    std::uint32_t sourceHeight,
    IExactDenseAccelerator* accelerator) noexcept
    : source_(scientificMaster), impl_(new (std::nothrow) Impl{}) {
    if (!impl_) return;
    if (sourceWidth == 0u || sourceHeight == 0u ||
        sourceWidth > std::numeric_limits<std::uint32_t>::max() / kScale ||
        sourceHeight > std::numeric_limits<std::uint32_t>::max() / kScale) {
        impl_->error = "TruthNegative dense geometry is invalid or overflows";
        return;
    }
    impl_->accelerator = accelerator;
    impl_->geometry.sourceWidth = sourceWidth;
    impl_->geometry.sourceHeight = sourceHeight;
    impl_->geometry.targetWidth = sourceWidth * kScale;
    impl_->geometry.targetHeight = sourceHeight * kScale;
    impl_->valid = true;
}

DenseProjectionTileSource::~DenseProjectionTileSource() = default;

Geometry DenseProjectionTileSource::geometry() const noexcept {
    return impl_ ? impl_->geometry : Geometry{};
}

bool DenseProjectionTileSource::valid() const noexcept {
    return impl_ && impl_->valid;
}

const std::string& DenseProjectionTileSource::error() const noexcept {
    static const std::string kAllocationFailure =
        "TruthNegative dense projection workspace allocation failed";
    return impl_ ? impl_->error : kAllocationFailure;
}

bool DenseProjectionTileSource::acceleratorEligible() const noexcept {
    return impl_ && impl_->accelerator != nullptr &&
           !impl_->acceleratorDisabled &&
           impl_->accelerator->exactScientificEligible();
}

bool DenseProjectionTileSource::acceleratorUsed() const noexcept {
    return impl_ && impl_->acceleratorUsed;
}

const char* DenseProjectionTileSource::acceleratorBackendName() const noexcept {
    if (!impl_ || impl_->accelerator == nullptr) return "CPU_REFERENCE";
    return impl_->accelerator->backendName();
}

std::size_t DenseProjectionTileSource::residentBytesUpperBound() const noexcept {
    std::size_t total = source_.residentBytesUpperBound();
    if (!impl_) return total;
    for (const auto& cell : impl_->cells) {
        const auto bytes = cell.rgb.capacity() * sizeof(float);
        if (total > std::numeric_limits<std::size_t>::max() - bytes) {
            return std::numeric_limits<std::size_t>::max();
        }
        total += bytes;
    }
    const std::size_t extra =
        (impl_->packedPatch.capacity() + impl_->acceleratedBlock.capacity()) *
        sizeof(float);
    if (total > std::numeric_limits<std::size_t>::max() - extra) {
        return std::numeric_limits<std::size_t>::max();
    }
    total += extra;
    if (impl_->accelerator != nullptr) {
        const auto acceleratorBytes=impl_->accelerator->residentBytesUpperBound();
        if (total > std::numeric_limits<std::size_t>::max() - acceleratorBytes) {
            return std::numeric_limits<std::size_t>::max();
        }
        total += acceleratorBytes;
    }
    return total;
}

float_dng::Status DenseProjectionTileSource::readCameraNativeTile(
    std::uint32_t x,
    std::uint32_t y,
    std::uint32_t width,
    std::uint32_t height,
    float* rgb,
    std::size_t floatCount) noexcept {
    try {
        if (!valid()) {
            return float_dng::Status::error(
                float_dng::StatusCode::InvalidArgument, error());
        }
        const auto g = impl_->geometry;
        if (!rgb || width == 0u || height == 0u ||
            x >= g.targetWidth || y >= g.targetHeight ||
            x + width > g.targetWidth || y + height > g.targetHeight) {
            return float_dng::Status::error(
                float_dng::StatusCode::InvalidArgument,
                "TruthNegative dense target tile is outside projection bounds");
        }
        const std::size_t pixels =
            static_cast<std::size_t>(width) * static_cast<std::size_t>(height);
        if (pixels > std::numeric_limits<std::size_t>::max() / 3u ||
            floatCount != pixels * 3u) {
            return float_dng::Status::error(
                float_dng::StatusCode::InvalidArgument,
                "TruthNegative dense target tile size mismatch");
        }

        auto loadCells = [&](
            std::uint32_t tx,
            std::uint32_t ty,
            std::uint32_t tw,
            std::uint32_t th,
            std::uint32_t& minSx,
            std::uint32_t& maxSx,
            std::uint32_t& minSy,
            std::uint32_t& maxSy) -> float_dng::Status {
            std::uint32_t ax0=0u, ax1=0u, bx0=0u, bx1=0u;
            std::uint32_t ay0=0u, ay1=0u, by0=0u, by1=0u;
            float unused=0.0f;
            axis_map(tx, g.sourceWidth, ax0, ax1, unused);
            axis_map(tx + tw - 1u, g.sourceWidth, bx0, bx1, unused);
            axis_map(ty, g.sourceHeight, ay0, ay1, unused);
            axis_map(ty + th - 1u, g.sourceHeight, by0, by1, unused);
            minSx=std::min(std::min(ax0,ax1),std::min(bx0,bx1));
            maxSx=std::max(std::max(ax0,ax1),std::max(bx0,bx1));
            minSy=std::min(std::min(ay0,ay1),std::min(by0,by1));
            maxSy=std::max(std::max(ay0,ay1),std::max(by0,by1));

            const std::uint32_t cellEdge = float_dng::kCanonicalTileEdge;
            const std::uint32_t firstCellX = (minSx / cellEdge) * cellEdge;
            const std::uint32_t lastCellX = (maxSx / cellEdge) * cellEdge;
            const std::uint32_t firstCellY = (minSy / cellEdge) * cellEdge;
            const std::uint32_t lastCellY = (maxSy / cellEdge) * cellEdge;

            impl_->cells.clear();
            for (std::uint32_t cy=firstCellY;; cy+=cellEdge) {
                for (std::uint32_t cx=firstCellX;; cx+=cellEdge) {
                    SourceCell cell{};
                    cell.x=cx; cell.y=cy;
                    cell.width=std::min(cellEdge,g.sourceWidth-cx);
                    cell.height=std::min(cellEdge,g.sourceHeight-cy);
                    cell.rgb.resize(
                        static_cast<std::size_t>(cell.width)*cell.height*3u);
                    const auto read=source_.readCameraNativeTile(
                        cell.x,cell.y,cell.width,cell.height,
                        cell.rgb.data(),cell.rgb.size());
                    if(!read) {
                        return float_dng::Status::error(
                            float_dng::StatusCode::SourceFailed,
                            "TruthNegative source Scientific Master tile failed: "+
                                read.message);
                    }
                    impl_->cells.push_back(std::move(cell));
                    if(cx==lastCellX) break;
                    if(cx>lastCellX-cellEdge) {
                        return float_dng::Status::error(
                            float_dng::StatusCode::InvalidArgument,
                            "TruthNegative source-cell x iteration overflow");
                    }
                }
                if(cy==lastCellY) break;
                if(cy>lastCellY-cellEdge) {
                    return float_dng::Status::error(
                        float_dng::StatusCode::InvalidArgument,
                        "TruthNegative source-cell y iteration overflow");
                }
            }
            return float_dng::Status::ok();
        };

        auto sample = [&](std::uint32_t sx, std::uint32_t sy, int channel,
                          float& value) -> bool {
            const auto* cell=impl_->findCell(sx,sy);
            if(!cell) return false;
            const std::size_t lx=sx-cell->x;
            const std::size_t ly=sy-cell->y;
            const std::size_t index=
                (ly*static_cast<std::size_t>(cell->width)+lx)*3u+
                static_cast<std::size_t>(channel);
            if(index>=cell->rgb.size()) return false;
            value=cell->rgb[index];
            return std::isfinite(value);
        };

        // GPU block cache: one exact dispatch creates up to 256x256 target
        // pixels, which feeds sixteen canonical 64x64 DNG tiles.
        if (acceleratorEligible()) {
            const std::uint32_t blockX=(x/kAcceleratedBlockEdge)*kAcceleratedBlockEdge;
            const std::uint32_t blockY=(y/kAcceleratedBlockEdge)*kAcceleratedBlockEdge;
            const std::uint32_t blockW=std::min(
                kAcceleratedBlockEdge,g.targetWidth-blockX);
            const std::uint32_t blockH=std::min(
                kAcceleratedBlockEdge,g.targetHeight-blockY);
            const bool requestInsideBlock =
                x>=blockX && y>=blockY &&
                x+width<=blockX+blockW && y+height<=blockY+blockH;

            if(requestInsideBlock) {
                const bool cacheHit=
                    impl_->acceleratedBlockValid &&
                    impl_->acceleratedBlockX==blockX &&
                    impl_->acceleratedBlockY==blockY &&
                    impl_->acceleratedBlockWidth==blockW &&
                    impl_->acceleratedBlockHeight==blockH;
                if(!cacheHit) {
                    std::uint32_t minSx=0u,maxSx=0u,minSy=0u,maxSy=0u;
                    const auto loaded=loadCells(
                        blockX,blockY,blockW,blockH,
                        minSx,maxSx,minSy,maxSy);
                    if(!loaded) return loaded;

                    const std::uint32_t patchW=maxSx-minSx+1u;
                    const std::uint32_t patchH=maxSy-minSy+1u;
                    impl_->packedPatch.resize(
                        static_cast<std::size_t>(patchW)*patchH*3u);
                    for(std::uint32_t py=0u;py<patchH;++py) {
                        for(std::uint32_t px=0u;px<patchW;++px) {
                            for(int c=0;c<3;++c) {
                                float value=0.0f;
                                if(!sample(minSx+px,minSy+py,c,value)) {
                                    return float_dng::Status::error(
                                        float_dng::StatusCode::SourceFailed,
                                        "TruthNegative accelerator patch assembly failed");
                                }
                                impl_->packedPatch[
                                    (static_cast<std::size_t>(py)*patchW+px)*3u+
                                    static_cast<std::size_t>(c)]=value;
                            }
                        }
                    }

                    impl_->acceleratedBlock.resize(
                        static_cast<std::size_t>(blockW)*blockH*3u);
                    AcceleratorPatchRequest request{};
                    request.sourceFullWidth=g.sourceWidth;
                    request.sourceFullHeight=g.sourceHeight;
                    request.patchOriginX=minSx;
                    request.patchOriginY=minSy;
                    request.patchWidth=patchW;
                    request.patchHeight=patchH;
                    request.targetOriginX=blockX;
                    request.targetOriginY=blockY;
                    request.targetWidth=blockW;
                    request.targetHeight=blockH;
                    std::string acceleratorError;
                    if(impl_->accelerator->projectPatch(
                            request,
                            impl_->packedPatch.data(),impl_->packedPatch.size(),
                            impl_->acceleratedBlock.data(),impl_->acceleratedBlock.size(),
                            acceleratorError)) {
                        impl_->acceleratedBlockValid=true;
                        impl_->acceleratedBlockX=blockX;
                        impl_->acceleratedBlockY=blockY;
                        impl_->acceleratedBlockWidth=blockW;
                        impl_->acceleratedBlockHeight=blockH;
                        impl_->acceleratorUsed=true;
                    } else {
                        impl_->acceleratorDisabled=true;
                        impl_->acceleratedBlockValid=false;
                        impl_->acceleratorFailure=acceleratorError;
                    }
                }

                if(impl_->acceleratedBlockValid) {
                    const std::uint32_t localX=x-impl_->acceleratedBlockX;
                    const std::uint32_t localY=y-impl_->acceleratedBlockY;
                    for(std::uint32_t oy=0u;oy<height;++oy) {
                        const std::size_t srcOffset=
                            (static_cast<std::size_t>(localY+oy)*
                                 impl_->acceleratedBlockWidth+localX)*3u;
                        const std::size_t dstOffset=
                            static_cast<std::size_t>(oy)*width*3u;
                        std::copy_n(
                            impl_->acceleratedBlock.data()+srcOffset,
                            static_cast<std::size_t>(width)*3u,
                            rgb+dstOffset);
                    }
                    return float_dng::Status::ok();
                }
            }
        }

        // Canonical CPU reference/fallback.
        std::uint32_t minSx=0u,maxSx=0u,minSy=0u,maxSy=0u;
        const auto loaded=loadCells(
            x,y,width,height,minSx,maxSx,minSy,maxSy);
        if(!loaded) return loaded;
        (void)minSx; (void)maxSx; (void)minSy; (void)maxSy;

        for(std::uint32_t oy=0u;oy<height;++oy) {
            std::uint32_t y0=0u,y1=0u;
            float fy=0.0f;
            axis_map(y+oy,g.sourceHeight,y0,y1,fy);
            for(std::uint32_t ox=0u;ox<width;++ox) {
                std::uint32_t x0=0u,x1=0u;
                float fx=0.0f;
                axis_map(x+ox,g.sourceWidth,x0,x1,fx);
                for(int c=0;c<3;++c) {
                    float p00=0.0f,p10=0.0f,p01=0.0f,p11=0.0f;
                    if(!sample(x0,y0,c,p00) || !sample(x1,y0,c,p10) ||
                       !sample(x0,y1,c,p01) || !sample(x1,y1,c,p11)) {
                        return float_dng::Status::error(
                            float_dng::StatusCode::SourceFailed,
                            "TruthNegative dense interpolation footprint incomplete");
                    }
                    const float top=p00+(p10-p00)*fx;
                    const float bottom=p01+(p11-p01)*fx;
                    const float value=top+(bottom-top)*fy;
                    if(!std::isfinite(value)) {
                        return float_dng::Status::error(
                            float_dng::StatusCode::SourceFailed,
                            "TruthNegative dense interpolation produced non-finite value");
                    }
                    rgb[(static_cast<std::size_t>(oy)*width+ox)*3u+
                        static_cast<std::size_t>(c)]=value;
                }
            }
        }
        return float_dng::Status::ok();
    } catch(const std::bad_alloc&) {
        return float_dng::Status::error(
            float_dng::StatusCode::SizeOverflow,
            "TruthNegative dense projection allocation failed");
    } catch(...) {
        return float_dng::Status::error(
            float_dng::StatusCode::SourceFailed,
            "TruthNegative dense projection failed unexpectedly");
    }
}

float_dng::Status compute_projected_raster_identity(
    DenseProjectionTileSource& source,
    Result& out) noexcept {
    try {
        out = {};
        if (!source.valid()) {
            return float_dng::Status::error(
                float_dng::StatusCode::InvalidArgument, source.error());
        }
        const auto g = source.geometry();
        out.geometry = g;
        out.acceleratorEligible = source.acceleratorEligible();
        out.acceleratorBackend =
            source.acceleratorEligible()
                ? source.acceleratorBackendName()
                : "CPU_REFERENCE";
        out.createsNewEvidence = false;
        out.impliesPhysicalSensorGeometry = false;
        out.measuredTargetClaimCount = kMeasuredTargetClaimCount;
        out.physicalFrameCount = 1u;
        out.independentEvidenceCount = 1u;
        out.methodId = kMethodId;
        out.targetAuthority = kAuthority;

        digest::ScientificMasterDigestAccumulator accumulator(
            g.targetWidth, g.targetHeight);
        if (!accumulator.valid()) {
            return float_dng::Status::error(
                float_dng::StatusCode::DigestFailed,
                "TruthNegative projected-raster digest init failed: " +
                    accumulator.error());
        }

        std::vector<float> tile;
        for (std::uint32_t y=0u; y<g.targetHeight;
             y += float_dng::kCanonicalTileEdge) {
            const auto h = std::min(
                float_dng::kCanonicalTileEdge, g.targetHeight - y);
            for (std::uint32_t x=0u; x<g.targetWidth;
                 x += float_dng::kCanonicalTileEdge) {
                const auto w = std::min(
                    float_dng::kCanonicalTileEdge, g.targetWidth - x);
                tile.resize(static_cast<std::size_t>(w) * h * 3u);
                const auto read = source.readCameraNativeTile(
                    x, y, w, h, tile.data(), tile.size());
                if (!read) return read;

                for (const float v : tile) {
                    if (!std::isfinite(v)) {
                        return float_dng::Status::error(
                            float_dng::StatusCode::DigestFailed,
                            "TruthNegative projected raster contains non-finite value");
                    }
                    if (v < 0.0f) ++out.negativeComponentCount;
                    if (v > 1.0f) ++out.overOneComponentCount;
                }
                out.projectedPixels +=
                    static_cast<std::uint64_t>(w) * static_cast<std::uint64_t>(h);

                digest::TileView view{};
                view.x=x; view.y=y; view.width=w; view.height=h;
                view.rgb=tile.data();
                view.rowStrideSamples=static_cast<std::size_t>(w)*3u;
                if (!accumulator.add_tile(view)) {
                    return float_dng::Status::error(
                        float_dng::StatusCode::DigestFailed,
                        "TruthNegative projected-raster digest rejected tile: " +
                            accumulator.error());
                }
                out.logicalResidentUpperBound = std::max(
                    out.logicalResidentUpperBound,
                    source.residentBytesUpperBound() +
                        tile.capacity() * sizeof(float) +
                        accumulator.metrics().residentBytesUpperBound);
            }
        }
        if (!accumulator.finalize(out.projectedRasterSha256)) {
            return float_dng::Status::error(
                float_dng::StatusCode::DigestFailed,
                "TruthNegative projected-raster digest finalization failed: " +
                    accumulator.error());
        }
        out.projectedRasterIdentityAvailable = true;
        out.acceleratorUsed = source.acceleratorUsed();
        if (!out.acceleratorUsed) out.acceleratorBackend = "CPU_REFERENCE";
        return float_dng::Status::ok();
    } catch (const std::bad_alloc&) {
        return float_dng::Status::error(
            float_dng::StatusCode::SizeOverflow,
            "TruthNegative projected-raster digest allocation failed");
    } catch (...) {
        return float_dng::Status::error(
            float_dng::StatusCode::DigestFailed,
            "TruthNegative projected-raster digest failed unexpectedly");
    }
}

std::string authority_manifest(const Result& result) {
    std::ostringstream out;
    out << "schema=TruthNegativeDenseProjection/0.3\n"
        << "method=" << result.methodId << "\n"
        << "source_width=" << result.geometry.sourceWidth << "\n"
        << "source_height=" << result.geometry.sourceHeight << "\n"
        << "target_width=" << result.geometry.targetWidth << "\n"
        << "target_height=" << result.geometry.targetHeight << "\n"
        << "target_authority=" << result.targetAuthority << "\n"
        << "compute_backend=" << result.acceleratorBackend << "\n"
        << "accelerator_eligible=" << (result.acceleratorEligible ? 1 : 0) << "\n"
        << "accelerator_used=" << (result.acceleratorUsed ? 1 : 0) << "\n"
        << "backend_changes_authority=0\n"
        << "creates_new_evidence=0\n"
        << "implies_physical_sensor_geometry=0\n"
        << "measured_target_claim_count=0\n"
        << "physical_frame_count=1\n"
        << "independent_evidence_count=1\n"
        << "reconstructed_cfa_is_original_evidence=0\n"
        << "appearance_applied=0\n"
        << "scientific_master_replaced=0\n";
    return out.str();
}

}  // namespace truthraw::truthnegative_dense_projection::v0_3
